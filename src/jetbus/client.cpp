#include "jetbus/client.hpp"

#include "jetbus/measurement_utils.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
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
    // Align with SharpJet: params.path is an object with equals, include caseInsensitive and id
    nlohmann::json request{
        {"id", id},
        {"method", "fetch"},
        {"params", {
             {"path", nlohmann::json{{"equals", path}}},
             {"caseInsensitive", false},
             {"id", id}
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
    if (options_.enable_debug_logs) printf("[DEBUG] Entering fetch for path: %s\n", path.c_str());
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
    if (!message.promise) {
        printf("[ERROR] message.promise is null in fetch()!\n");
        throw JetBusError("Promise pointer is null in fetch()");
    }
    {
        std::lock_guard lock(pending_mutex_);
        pending_promises_[id] = message.promise;
    }
    if (options_.enable_debug_logs) printf("[DEBUG] About to call get_future() in fetch()\n");
    auto future = message.promise->get_future();
    if (options_.enable_debug_logs) printf("[DEBUG] Future obtained, enqueueing message...\n");
    enqueue(std::move(message));
    if (options_.enable_debug_logs) printf("[DEBUG] get_future() called, waiting for response...\n");
    if (future.wait_for(options_.request_timeout) == std::future_status::timeout) {
        {
            std::lock_guard lock(pending_mutex_);
            pending_tokens_.erase(id);
            pending_promises_.erase(id);
        }
    printf("[ERROR] Fetch request timed out for path: %s\n", path.c_str());
        throw JetBusError("Fetch request timed out for path: " + path);
    }
    auto response = future.get();
    if (response.contains("error")) {
        std::lock_guard lock(pending_mutex_);
        pending_tokens_.erase(id);
        pending_promises_.erase(id);
        printf("[ERROR] Fetch request failed for path: %s\n", path.c_str());
        throw JetBusError("Fetch request failed for path: " + path);
    }
    if (options_.enable_debug_logs) printf("[DEBUG] Fetch for path %s succeeded, token: %s\n", path.c_str(), token.c_str());
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

void JetBusClient::handle_message(std::string_view text) {
    auto message = nlohmann::json::parse(text, nullptr, false);
    if (message.is_discarded()) {
        return;
    }

    // Eventos chegam no formato: { "method": <num>, "params": { "path": "...", "event": "add|change|fetch", "value": ... } }
    if (message.contains("params") && message["params"].is_object()) {
        const auto& params = message["params"];
        const std::string path = params.value("path", "");
        const std::string event_str = params.value("event", "");
        std::string raw_value;
        if (params.contains("value")) {
            const auto& json_value = params["value"];
            if (json_value.is_string()) {
                raw_value = json_value.get<std::string>();
            } else {
                raw_value = json_value.dump();
            }
        }
        if (!path.empty()) {
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
                callback(path, raw_value, event_type_from_string(event_str));
            }
            return;
        }
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
        // Desativar permessage-deflate no lado do cliente (não anunciar extensões)
        {
            boost::beast::websocket::permessage_deflate pmd;
            pmd.client_enable = false;
            ws.set_option(pmd);
        }
        boost::system::error_code ec;

        tcp::resolver resolver{ioc};
        if (options_.enable_debug_logs) printf("[DEBUG] Resolving %s:%s...\n", url.host.c_str(), url.port.c_str());
        auto const results = resolver.resolve(url.host, url.port, ec);
        if (ec) {
            printf("[ERROR] resolve failed: %s\n", ec.message().c_str());
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, options_.reconnect_max_delay);
            continue;
        }

        if (options_.enable_debug_logs) printf("[DEBUG] Connecting TCP...\n");
        boost::asio::connect(ws.next_layer().socket(), results.begin(), results.end(), ec);
        if (ec) {
            printf("[ERROR] connect failed: %s\n", ec.message().c_str());
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, options_.reconnect_max_delay);
            continue;
        }

    // Host para handshake; mantenha como estava (host puro para 80/443, host:port caso contrário)
    std::string host_header = (url.port == "80" || url.port == "443") ? url.host : (url.host + ":" + url.port);
        std::string target = url.target;
        if (options_.enable_debug_logs) printf("[DEBUG] Performing websocket handshake to %s%s...\n", host_header.c_str(), target.c_str());
        ws.set_option(boost::beast::websocket::stream_base::decorator(
            [&](boost::beast::websocket::request_type& req) {
                namespace http = boost::beast::http;

                // Zere campos que podem atrapalhar
                req.erase(http::field::user_agent);
                req.erase(http::field::origin);
                req.erase(http::field::sec_websocket_extensions);
                req.erase(http::field::cache_control);
                req.erase(http::field::pragma);

                // Replicar exatamente o que funcionou no nc
                req.set(http::field::host, host_header);            // sem :80
                req.set(http::field::upgrade, "websocket");
                req.set(http::field::connection, "Upgrade");       // U maiúsculo
                req.set(http::field::sec_websocket_version, "13");
                req.set(http::field::sec_websocket_protocol, "jet");

                // Log do request
                if (options_.enable_debug_logs) {
                    std::stringstream ss; ss << req;
                    printf("[DEBUG] Outgoing WS handshake request:\n%s\n", ss.str().c_str());
                }
            }
        ));

        // handshake (sem tentar barra final imediatamente)
        ws.handshake(host_header, target, ec);
        if (!ec) {
            if (options_.enable_debug_logs) printf("[DEBUG] Handshake successful (response not directly accessible via ws.response()).\n");
        } else {
            if (options_.enable_debug_logs) printf("[DEBUG] Handshake error immediately after call: %s\n", ec.message().c_str());
        }
        // não retentar automaticamente com barra final aqui
        if (ec) {
            printf("[ERROR] handshake failed: %s\n", ec.message().c_str());
            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, options_.reconnect_max_delay);
            continue;
        }

    if (options_.enable_debug_logs) printf("[DEBUG] Handshake successful. Entering I/O loop.\n");
        delay = options_.reconnect_initial_delay;

        while (running_.load()) {
            // Drain entire queue before blocking on read
            std::vector<PendingMessage> messages_to_send;
            {
                std::unique_lock lock(queue_mutex_);
                // Use wait_for with short timeout to allow periodic reading even without messages
                queue_cv_.wait_for(lock, std::chrono::milliseconds(10), [&] {
                    return !queue_.empty() || !running_.load();
                });
                if (!running_.load()) {
                    break;
                }
                // Move all pending messages to avoid multiple lock/unlock cycles
                while (!queue_.empty()) {
                    messages_to_send.emplace_back(std::move(queue_.front()));
                    queue_.pop_front();
                }
            }

            // Send all queued messages
            bool write_error = false;
            for (const auto& message : messages_to_send) {
                if (!message.payload.empty()) {
                    if (options_.enable_debug_logs) printf("[DEBUG] Writing payload: %s\n", message.payload.c_str());
                    ws.write(boost::asio::buffer(message.payload), ec);
                    if (ec) {
                        printf("[ERROR] write failed: %s\n", ec.message().c_str());
                        write_error = true;
                        break;
                    }
                }
            }
            
            if (write_error) {
                break;
            }

            // Read and process incoming frames immediately
            ws.next_layer().expires_after(std::chrono::milliseconds(200));
            buffer.consume(buffer.size());
            ws.read(buffer, ec);
            if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::timed_out) {
                if (options_.enable_debug_logs) printf("[DEBUG] read timeout or aborted, continuing...\n");
                ec.clear();
            } else if (ec) {
                printf("[ERROR] read failed: %s\n", ec.message().c_str());
                break;
            } else {
                auto data = boost::beast::buffers_to_string(buffer.data());
                if (options_.enable_debug_logs) printf("[DEBUG] Received message: %s\n", data.c_str());
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
