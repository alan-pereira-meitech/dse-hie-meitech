#include "jetbus/client.hpp"

#include "jetbus/measurement_utils.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <algorithm>
#include <deque>
#include <future>
#include <thread>
#include <limits>
#include <mutex>
#include <regex>
#include <sstream>

namespace jetbus {
namespace {

struct UrlParts {
    std::string scheme;
    std::string host;
    std::string port;
    std::string target;
};

UrlParts parse_url(const std::string& url) {
    static const std::regex pattern(R"((ws|wss)://([^/:]+)(:([0-9]+))?(/.*)?$)");
    std::smatch match;
    if (!std::regex_match(url, match, pattern)) {
        throw JetBusError("Invalid JetBus URL: " + url);
    }
    UrlParts parts;
    parts.scheme = match[1];
    parts.host = match[2];
    parts.port = match[4].matched ? match[4].str() : (parts.scheme == "wss" ? "443" : "80");
    parts.target = match[5].matched ? match[5].str() : "/";
    if (parts.target.empty()) {
        parts.target = "/";
    }
    return parts;
}

std::string build_fetch_payload(int id, const std::string& path) {
    nlohmann::json request{
        {"id", id},
        {"method", "fetch"},
        {"params", {
             {"path", path},
             {"matcher", nlohmann::json{{"equals", path}}}
        }}
    };
    return request.dump();
}

std::string build_unfetch_payload(int id, const std::string& token) {
    nlohmann::json request{
        {"id", id},
        {"method", "unfetch"},
        {"params", {
             {"token", token}
        }}
    };
    return request.dump();
}

std::string build_set_payload(int id, const std::string& path, const nlohmann::json& value) {
    nlohmann::json request{
        {"id", id},
        {"method", "set"},
        {"params", {
             {"path", path},
             {"value", value}
        }}
    };
    return request.dump();
}

} // namespace

JetBusClient::JetBusClient(Options options)
    : options_(std::move(options)) {
    if (options_.url.empty()) {
        throw JetBusError("JetBusClient requires a non-empty URL");
    }
}

JetBusClient::~JetBusClient() {
    disconnect();
}

void JetBusClient::set_data_callback(DataCallback callback) {
    std::scoped_lock lock(state_mutex_);
    data_callback_ = std::move(callback);
}

void JetBusClient::set_fetch_callback(FetchCallback callback) {
    std::scoped_lock lock(state_mutex_);
    fetch_callback_ = std::move(callback);
}

void JetBusClient::connect() {
    if (running_.exchange(true)) {
        return;
    }
    worker_ = std::thread([this] { run(); });
}

void JetBusClient::disconnect() {
    if (!running_.exchange(false)) {
        return;
    }
    queue_cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

std::string JetBusClient::fetch(const std::string& path) {
    const int id = next_id_.fetch_add(1);
    const std::string token = std::to_string(id);
    {
        std::lock_guard lock(pending_mutex_);
        pending_tokens_[id] = token;
    }
    PendingMessage message;
    message.payload = build_fetch_payload(id, path);
    message.token = token;
    message.promise = std::make_shared<std::promise<nlohmann::json>>();
    {
        std::lock_guard lock(pending_mutex_);
        pending_promises_[id] = message.promise;
    }
    enqueue(std::move(message));
    auto future = message.promise->get_future();
    if (future.wait_for(options_.request_timeout) == std::future_status::timeout) {
        {
            std::lock_guard lock(pending_mutex_);
            pending_tokens_.erase(id);
            pending_promises_.erase(id);
        }
        throw JetBusError("Fetch request timed out for path: " + path);
    }
    auto response = future.get();
    if (response.contains("error")) {
        std::lock_guard lock(pending_mutex_);
        pending_tokens_.erase(id);
        pending_promises_.erase(id);
        throw JetBusError("Fetch request failed for path: " + path);
    }
    return token;
}

void JetBusClient::unfetch(const std::string& token) {
    const int id = next_id_.fetch_add(1);
    PendingMessage message;
    message.payload = build_unfetch_payload(id, token);
    enqueue(std::move(message));
}

void JetBusClient::set(const std::string& path, const nlohmann::json& value) {
    const int id = next_id_.fetch_add(1);
    PendingMessage message;
    message.payload = build_set_payload(id, path, value);
    message.promise = std::make_shared<std::promise<nlohmann::json>>();
    {
        std::lock_guard lock(pending_mutex_);
        pending_promises_[id] = message.promise;
    }
    enqueue(std::move(message));
    auto future = message.promise->get_future();
    if (future.wait_for(options_.request_timeout) == std::future_status::timeout) {
        std::lock_guard lock(pending_mutex_);
        pending_promises_.erase(id);
        throw JetBusError("Set request timed out for path: " + path);
    }
    auto response = future.get();
    if (response.contains("error")) {
        std::lock_guard lock(pending_mutex_);
        pending_promises_.erase(id);
        throw JetBusError("Set request failed for path: " + path);
    }
}

std::optional<std::string> JetBusClient::read_cached(const std::string& path) const {
    std::scoped_lock lock(state_mutex_);
    auto it = cache_.find(path);
    if (it == cache_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::unordered_map<std::string, std::string> JetBusClient::snapshot() const {
    std::scoped_lock lock(state_mutex_);
    return cache_;
}

bool JetBusClient::wait_for(const std::string& path,
                            const std::function<bool(const std::string&)>& predicate,
                            std::chrono::milliseconds timeout) {
    std::unique_lock lock(state_mutex_);
    return cache_cv_.wait_for(lock, timeout, [&] {
        auto it = cache_.find(path);
        if (it == cache_.end()) {
            return false;
        }
        return predicate(it->second);
    });
}

void JetBusClient::enqueue(PendingMessage message) {
    {
        std::lock_guard lock(queue_mutex_);
        queue_.push_back(std::move(message));
    }
    queue_cv_.notify_all();
}

void JetBusClient::notify_fetch(const std::string& token, bool success) {
    FetchCallback callback;
    {
        std::scoped_lock lock(state_mutex_);
        callback = fetch_callback_;
    }
    if (callback) {
        callback(success, token);
    }
}

void JetBusClient::handle_message(const std::string& text) {
    auto message = nlohmann::json::parse(text, nullptr, false);
    if (message.is_discarded()) {
        return;
    }

    if (message.contains("event")) {
        const std::string path = message.value("path", "");
        std::string raw_value;
        if (message.contains("value")) {
            const auto& json_value = message["value"];
            if (json_value.is_string()) {
                raw_value = json_value.get<std::string>();
            } else {
                raw_value = json_value.dump();
            }
        }
        {
            std::scoped_lock lock(state_mutex_);
            cache_[path] = raw_value;
        }
        cache_cv_.notify_all();
        DataCallback callback;
        {
            std::scoped_lock lock(state_mutex_);
            callback = data_callback_;
        }
        if (callback) {
            callback(path, raw_value, event_type_from_string(message.value("event", "")));
        }
        return;
    }

    if (message.contains("id")) {
        const int id = message["id"].get<int>();
        std::shared_ptr<std::promise<nlohmann::json>> promise;
        std::optional<std::string> token;
        {
            std::lock_guard lock(pending_mutex_);
            auto pit = pending_promises_.find(id);
            if (pit != pending_promises_.end()) {
                promise = pit->second;
                pending_promises_.erase(pit);
            }
            auto tit = pending_tokens_.find(id);
            if (tit != pending_tokens_.end()) {
                token = tit->second;
                pending_tokens_.erase(tit);
            }
        }
        if (promise) {
            promise->set_value(message);
        }
        if (token) {
            notify_fetch(*token, !message.contains("error"));
        }
    }
}

void JetBusClient::run() {
    using tcp = boost::asio::ip::tcp;
    boost::asio::io_context ioc;
    boost::beast::flat_buffer buffer;
    auto url = parse_url(options_.url);
    auto delay = options_.reconnect_initial_delay;

    while (running_.load()) {
        boost::beast::websocket::stream<boost::beast::tcp_stream> ws{ioc};
        boost::system::error_code ec;

        tcp::resolver resolver{ioc};
        auto const results = resolver.resolve(url.host, url.port, ec);
        if (ec) {
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, options_.reconnect_max_delay);
            continue;
        }

        boost::asio::connect(ws.next_layer().socket(), results.begin(), results.end(), ec);
        if (ec) {
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, options_.reconnect_max_delay);
            continue;
        }

        ws.handshake(url.host, url.target, ec);
        if (ec) {
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, options_.reconnect_max_delay);
            continue;
        }

        delay = options_.reconnect_initial_delay;

        while (running_.load()) {
            PendingMessage message;
            {
                std::unique_lock lock(queue_mutex_);
                queue_cv_.wait_for(lock, std::chrono::milliseconds(50), [&] {
                    return !queue_.empty() || !running_.load();
                });
                if (!running_.load()) {
                    break;
                }
                if (!queue_.empty()) {
                    message = std::move(queue_.front());
                    queue_.pop_front();
                }
            }

            if (!message.payload.empty()) {
                ws.write(boost::asio::buffer(message.payload), ec);
                if (ec) {
                    break;
                }
            }

            ws.next_layer().expires_after(std::chrono::milliseconds(200));
            buffer.consume(buffer.size());
            ws.read(buffer, ec);
            if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::timed_out) {
                ec.clear();
            } else if (ec) {
                break;
            } else {
                auto data = boost::beast::buffers_to_string(buffer.data());
                handle_message(data);
            }
        }

        boost::system::error_code close_ec;
        ws.close(boost::beast::websocket::close_code::normal, close_ec);
        if (!options_.enable_reconnect || !running_.load()) {
            break;
        }
        std::this_thread::sleep_for(delay);
        delay = std::min(delay * 2, options_.reconnect_max_delay);
    }
}

} // namespace jetbus
