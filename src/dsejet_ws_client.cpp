#include "dsejet_ws_client.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include <boost/asio/connect.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>

#include <openssl/err.h>
#include <openssl/ssl.h>

namespace dsejet {

namespace {
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

const char *kResultType = "result";
const char *kErrorType = "error";
const char *kEventType = "event";
const char *kCallType = "call";

class PlainWebSocketAdapter;
class TlsWebSocketAdapter;

} // namespace

namespace {

class PlainWebSocketAdapter : public DSEJetWSClient::WebSocketAdapter {
public:
    explicit PlainWebSocketAdapter(net::io_context &io) : resolver_(io), ws_(io) {}

    void async_connect(const std::string &host, std::uint16_t port, const std::string &target,
                       std::function<void(const boost::system::error_code &)> handler) override {
        host_ = host;
        target_ = target;
        resolver_.async_resolve(host, std::to_string(port),
                                [this, handler](const boost::system::error_code &ec, tcp::resolver::results_type results) {
            if (ec) {
                handler(ec);
                return;
            }
            net::async_connect(ws_.next_layer(), results,
                               [this, handler](const boost::system::error_code &connect_ec, const tcp::endpoint &) {
                if (connect_ec) {
                    handler(connect_ec);
                    return;
                }
                ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
                ws_.async_handshake(host_, target_, handler);
            });
        });
    }

    void async_write(const std::shared_ptr<std::string> &data,
                      std::function<void(const boost::system::error_code &, std::size_t)> handler) override {
        ws_.async_write(net::buffer(*data), handler);
    }

    void async_read(beast::flat_buffer &buffer,
                     std::function<void(const boost::system::error_code &, std::size_t)> handler) override {
        ws_.async_read(buffer, handler);
    }

    void async_ping(std::function<void(const boost::system::error_code &)> handler) override {
        ws_.async_ping({}, handler);
    }

    void async_close(std::function<void(const boost::system::error_code &)> handler) override {
        if (ws_.is_open()) {
            ws_.async_close(websocket::close_code::normal, handler);
        } else {
            handler({});
        }
    }

    bool is_open() const override {
        return ws_.is_open();
    }

private:
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    std::string host_;
    std::string target_;
};

class TlsWebSocketAdapter : public DSEJetWSClient::WebSocketAdapter {
public:
    TlsWebSocketAdapter(net::io_context &io, ssl::context &ssl_ctx)
        : resolver_(io), ws_(io, ssl_ctx) {}

    void async_connect(const std::string &host, std::uint16_t port, const std::string &target,
                       std::function<void(const boost::system::error_code &)> handler) override {
        host_ = host;
        target_ = target;
        resolver_.async_resolve(host, std::to_string(port),
                                [this, handler](const boost::system::error_code &ec, tcp::resolver::results_type results) {
            if (ec) {
                handler(ec);
                return;
            }
            net::async_connect(ws_.next_layer().next_layer(), results,
                               [this, handler](const boost::system::error_code &connect_ec, const tcp::endpoint &) {
                if (connect_ec) {
                    handler(connect_ec);
                    return;
                }
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
                if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str())) {
                    boost::system::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                    handler(ec);
                    return;
                }
#endif
                ws_.next_layer().async_handshake(ssl::stream_base::client,
                                                 [this, handler](const boost::system::error_code &ssl_ec) {
                    if (ssl_ec) {
                        handler(ssl_ec);
                        return;
                    }
                    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
                    ws_.async_handshake(host_, target_, handler);
                });
            });
        });
    }

    void async_write(const std::shared_ptr<std::string> &data,
                      std::function<void(const boost::system::error_code &, std::size_t)> handler) override {
        ws_.async_write(net::buffer(*data), handler);
    }

    void async_read(beast::flat_buffer &buffer,
                     std::function<void(const boost::system::error_code &, std::size_t)> handler) override {
        ws_.async_read(buffer, handler);
    }

    void async_ping(std::function<void(const boost::system::error_code &)> handler) override {
        ws_.async_ping({}, handler);
    }

    void async_close(std::function<void(const boost::system::error_code &)> handler) override {
        if (ws_.is_open()) {
            ws_.async_close(websocket::close_code::normal, handler);
        } else {
            handler({});
        }
    }

    bool is_open() const override {
        return ws_.is_open();
    }

private:
    tcp::resolver resolver_;
    websocket::stream<ssl::stream<tcp::socket>> ws_;
    std::string host_;
    std::string target_;
};

} // namespace

nlohmann::json MakeAuthenticateRequest(std::uint64_t id, const std::string &user, const std::string &password) {
    nlohmann::json payload = {
        {"type", kCallType},
        {"id", id},
        {"method", "authenticate"},
        {"params", {{"user", user}, {"password", password}}},
    };
    return payload;
}

nlohmann::json MakeFetchRequest(std::uint64_t id, const std::string &path, bool subscribe) {
    nlohmann::json payload = {
        {"type", kCallType},
        {"id", id},
        {"method", "fetch"},
        {"path", path},
        {"params", {{"subscribe", subscribe}}},
    };
    return payload;
}

nlohmann::json MakeSetRequest(std::uint64_t id, const std::string &path, const nlohmann::json &value) {
    nlohmann::json payload = {
        {"type", kCallType},
        {"id", id},
        {"method", "set"},
        {"path", path},
        {"value", value},
    };
    return payload;
}

nlohmann::json MakePingFrame() {
    return nlohmann::json{{"type", "ping"}};
}

bool ParseJetEvent(const nlohmann::json &message, JetEvent &event) {
    if (!message.is_object()) {
        return false;
    }
    if (!message.contains("type") || message.at("type") != kEventType) {
        return false;
    }
    if (!message.contains("path") || !message.contains("event")) {
        return false;
    }
    event.path = message.at("path").get<std::string>();
    event.event = message.at("event").get<std::string>();
    if (message.contains("value")) {
        event.value = message.at("value");
    } else {
        event.value = nlohmann::json();
    }
    return true;
}

DSEJetWSClient::DSEJetWSClient(const Config &config, std::shared_ptr<Logger> logger)
    : config_(config),
      logger_(std::move(logger)),
      work_guard_(std::in_place, io_context_.get_executor()),
      ping_timer_(io_context_),
      reconnect_timer_(io_context_),
      timeout_timer_(io_context_),
      backoff_(config.reconnect_initial),
      max_backoff_(config.reconnect_max) {
    if (logger_) {
        logger_->set_level(static_cast<LogLevel>(config_.log_level));
    }

    io_thread_ = std::thread([this]() {
        io_context_.run();
    });
}

DSEJetWSClient::~DSEJetWSClient() {
    close();
}

std::future<void> DSEJetWSClient::connect() {
    auto promise = std::make_shared<std::promise<void>>();
    {
        std::lock_guard<std::mutex> lock(promise_mutex_);
        initial_connect_promise_ = promise;
    }
    net::post(io_context_, [this]() {
        do_connect();
    });
    return promise->get_future();
}

void DSEJetWSClient::close() {
    bool expected = false;
    if (!stop_.compare_exchange_strong(expected, true)) {
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
        return;
    }

    net::post(io_context_, [this]() {
        ping_timer_.cancel();
        reconnect_timer_.cancel();
        timeout_timer_.cancel();
        if (ws_) {
            ws_->async_close([](const boost::system::error_code &) {});
        }
        resolve_pending_with_error("Client stopped");
        fail_initial("Client stopped");
        if (work_guard_) {
            work_guard_->reset();
            work_guard_.reset();
        }
    });

    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

std::future<nlohmann::json> DSEJetWSClient::fetch(const std::string &path, bool subscribe, std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) {
        timeout = config_.request_timeout;
    }
    auto id = next_request_id_++;
    auto payload = MakeFetchRequest(id, path, subscribe);
    return send_request(payload, JetRequestType::Fetch, timeout);
}

std::future<nlohmann::json> DSEJetWSClient::fetch_once(const std::string &path, std::chrono::milliseconds timeout) {
    return fetch(path, false, timeout);
}

std::future<nlohmann::json> DSEJetWSClient::set_value(const std::string &path, const nlohmann::json &value, std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) {
        timeout = config_.request_timeout;
    }
    auto id = next_request_id_++;
    auto payload = MakeSetRequest(id, path, value);
    return send_request(payload, JetRequestType::Set, timeout);
}

void DSEJetWSClient::subscribe(const std::vector<std::string> &paths, const EventHandler &handler) {
    net::post(io_context_, [this, paths, handler]() {
        for (const auto &path : paths) {
            subscriptions_[path].push_back(handler);
            auto future = fetch(path, true, config_.request_timeout);
            std::thread([future = std::move(future), logger = logger_]() mutable {
                try {
                    future.get();
                } catch (const std::exception &ex) {
                    if (logger) {
                        logger->warn(std::string("Initial fetch failed: ") + ex.what());
                    }
                }
            }).detach();
        }
    });
}

void DSEJetWSClient::set_error_handler(ErrorHandler handler) {
    net::post(io_context_, [this, handler]() {
        error_handler_ = handler;
    });
}

void DSEJetWSClient::do_connect() {
    if (stop_) {
        return;
    }
    if (connecting_) {
        return;
    }
    connecting_ = true;
    read_buffer_.consume(read_buffer_.size());
    if (config_.tls) {
        if (!ssl_context_) {
            ssl_context_ = std::make_unique<ssl::context>(ssl::context::tlsv12_client);
            ssl_context_->set_default_verify_paths();
            ssl_context_->set_verify_mode(ssl::verify_none); // TODO: configure verification according to deployment
        }
        ws_ = std::make_unique<TlsWebSocketAdapter>(io_context_, *ssl_context_);
    } else {
        ws_ = std::make_unique<PlainWebSocketAdapter>(io_context_);
    }
    ws_->async_connect(config_.host, config_.port, config_.jet_path,
                       [this](const boost::system::error_code &ec) {
        handle_connect_result(ec);
    });
}

void DSEJetWSClient::handle_connect_result(const boost::system::error_code &ec) {
    connecting_ = false;
    if (ec) {
        connected_ = false;
        handle_error("connect", ec);
        fail_initial(ec.message());
        schedule_reconnect();
        return;
    }
    on_connected();
}

void DSEJetWSClient::on_connected() {
    connected_ = true;
    backoff_ = config_.reconnect_initial;
    if (logger_) {
        logger_->info("WebSocket connected");
    }
    start_read();
    schedule_ping();

    if (!config_.user.empty()) {
        auto id = next_request_id_++;
        auto payload = MakeAuthenticateRequest(id, config_.user, config_.password);
        auto future = send_request(payload, JetRequestType::Authenticate, config_.request_timeout);
        std::thread([this, future = std::move(future)]() mutable {
            try {
                future.get();
                mark_ready();
            } catch (const std::exception &ex) {
                handle_error(std::string("Authentication failed: ") + ex.what());
                fail_initial(ex.what());
                reset_connection();
                schedule_reconnect();
            }
        }).detach();
    } else {
        mark_ready();
    }
}

void DSEJetWSClient::start_read() {
    if (!ws_) {
        return;
    }
    ws_->async_read(read_buffer_, [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
        if (ec) {
            connected_ = false;
            handle_error("read", ec);
            schedule_reconnect();
            return;
        }
        std::string payload = beast::buffers_to_string(read_buffer_.data());
        read_buffer_.consume(bytes_transferred);
        handle_message(payload);
        start_read();
    });
}

void DSEJetWSClient::schedule_ping() {
    if (stop_) {
        return;
    }
    ping_timer_.expires_after(config_.ping_interval);
    ping_timer_.async_wait([this](const boost::system::error_code &ec) {
        if (ec || stop_) {
            return;
        }
        if (ws_) {
            ws_->async_ping([this](const boost::system::error_code &ping_ec) {
                if (ping_ec) {
                    handle_error("ping", ping_ec);
                    schedule_reconnect();
                }
            });
        }
        schedule_ping();
    });
}

void DSEJetWSClient::schedule_reconnect() {
    if (stop_) {
        return;
    }
    reconnect_timer_.expires_after(backoff_);
    reconnect_timer_.async_wait([this](const boost::system::error_code &ec) {
        if (ec || stop_) {
            return;
        }
        do_connect();
    });
    backoff_ = std::min(backoff_ * 2, max_backoff_);
}

void DSEJetWSClient::handle_error(const std::string &context, const boost::system::error_code &ec) {
    std::string message = context + ": " + ec.message();
    handle_error(message);
}

void DSEJetWSClient::handle_error(const std::string &message) {
    if (logger_) {
        logger_->warn(message);
    }
    if (error_handler_) {
        (*error_handler_)(message);
    }
}

void DSEJetWSClient::mark_ready() {
    std::shared_ptr<std::promise<void>> promise;
    {
        std::lock_guard<std::mutex> lock(promise_mutex_);
        promise.swap(initial_connect_promise_);
    }
    if (promise) {
        promise->set_value();
    }
}

void DSEJetWSClient::fail_initial(const std::string &message) {
    std::shared_ptr<std::promise<void>> promise;
    {
        std::lock_guard<std::mutex> lock(promise_mutex_);
        promise.swap(initial_connect_promise_);
    }
    if (promise) {
        promise->set_exception(std::make_exception_ptr(std::runtime_error(message)));
    }
}

void DSEJetWSClient::reset_connection() {
    if (ws_) {
        ws_->async_close([](const boost::system::error_code &) {});
        ws_.reset();
    }
}

void DSEJetWSClient::enqueue_message(const std::string &message) {
    auto data = std::make_shared<std::string>(message);
    net::post(io_context_, [this, data]() {
        write_queue_.push_back(data);
        if (!write_in_progress_) {
            do_write();
        }
    });
}

void DSEJetWSClient::do_write() {
    if (write_queue_.empty() || !ws_) {
        return;
    }
    write_in_progress_ = true;
    auto data = write_queue_.front();
    ws_->async_write(data, [this, data](const boost::system::error_code &ec, std::size_t) {
        if (ec) {
            write_in_progress_ = false;
            handle_error("write", ec);
            schedule_reconnect();
            return;
        }
        write_queue_.pop_front();
        write_in_progress_ = false;
        if (!write_queue_.empty()) {
            do_write();
        }
    });
}

void DSEJetWSClient::handle_message(const std::string &payload) {
    try {
        auto message = nlohmann::json::parse(payload);
        if (!message.contains("type")) {
            return;
        }
        auto type = message.at("type").get<std::string>();
        if (type == kResultType) {
            handle_response(message);
        } else if (type == kErrorType) {
            handle_response(message);
        } else if (type == kEventType) {
            handle_event(message);
        } else {
            // TODO: handle other message types when documented
        }
    } catch (const std::exception &ex) {
        if (logger_) {
            logger_->warn(std::string("Failed to parse message: ") + ex.what());
        }
    }
}

void DSEJetWSClient::handle_response(const nlohmann::json &message) {
    if (!message.contains("id")) {
        return;
    }
    auto id = message.at("id").get<std::uint64_t>();
    auto it = pending_requests_.find(id);
    if (it == pending_requests_.end()) {
        return;
    }
    auto pending = it->second;
    pending_requests_.erase(it);

    bool success = true;
    std::string error_message;
    if (message.contains("type") && message.at("type") == kErrorType) {
        success = false;
        if (message.contains("error")) {
            if (message.at("error").is_string()) {
                error_message = message.at("error").get<std::string>();
            } else if (message.at("error").is_object() && message.at("error").contains("message")) {
                error_message = message.at("error").at("message").get<std::string>();
            }
        }
    } else if (message.contains("success")) {
        success = message.at("success").get<bool>();
    }

    if (success) {
        if (message.contains("value")) {
            pending.promise->set_value(message.at("value"));
        } else {
            pending.promise->set_value(nlohmann::json());
        }
    } else {
        if (error_message.empty()) {
            error_message = "Jet request failed";
        }
        pending.promise->set_exception(std::make_exception_ptr(std::runtime_error(error_message)));
    }

    if (pending.type == JetRequestType::Authenticate) {
        if (success) {
            mark_ready();
        } else {
            fail_initial(error_message);
        }
    }

    schedule_timeout_watchdog();
}

void DSEJetWSClient::handle_event(const nlohmann::json &message) {
    JetEvent event;
    if (!ParseJetEvent(message, event)) {
        return;
    }
    auto it = subscriptions_.find(event.path);
    if (it == subscriptions_.end()) {
        return;
    }
    for (const auto &handler : it->second) {
        handler(event.path, event.value);
    }
}

void DSEJetWSClient::check_timeouts() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = pending_requests_.begin(); it != pending_requests_.end();) {
        if (now >= it->second.deadline) {
            it->second.promise->set_exception(std::make_exception_ptr(std::runtime_error("Jet request timeout")));
            it = pending_requests_.erase(it);
        } else {
            ++it;
        }
    }
    if (pending_requests_.empty()) {
        timeout_timer_.cancel();
    } else {
        schedule_timeout_watchdog();
    }
}

void DSEJetWSClient::schedule_timeout_watchdog() {
    if (pending_requests_.empty()) {
        timeout_timer_.cancel();
        return;
    }
    auto next_deadline = pending_requests_.begin()->second.deadline;
    for (const auto &entry : pending_requests_) {
        if (entry.second.deadline < next_deadline) {
            next_deadline = entry.second.deadline;
        }
    }
    timeout_timer_.expires_at(next_deadline);
    timeout_timer_.async_wait([this](const boost::system::error_code &ec) {
        if (ec) {
            return;
        }
        check_timeouts();
    });
}

void DSEJetWSClient::resolve_pending_with_error(const std::string &message) {
    for (auto &entry : pending_requests_) {
        entry.second.promise->set_exception(std::make_exception_ptr(std::runtime_error(message)));
    }
    pending_requests_.clear();
    timeout_timer_.cancel();
}

std::future<nlohmann::json> DSEJetWSClient::send_request(const nlohmann::json &payload, JetRequestType type, std::chrono::milliseconds timeout) {
    auto promise = std::make_shared<std::promise<nlohmann::json>>();
    auto future = promise->get_future();
    auto deadline = std::chrono::steady_clock::now() + timeout;
    net::post(io_context_, [this, payload, promise, type, deadline]() {
        if (stop_) {
            promise->set_exception(std::make_exception_ptr(std::runtime_error("Client stopped")));
            return;
        }
        pending_requests_.emplace(payload.at("id").get<std::uint64_t>(), PendingRequest{promise, type, deadline});
        enqueue_message(payload.dump());
        schedule_timeout_watchdog();
    });
    return future;
}

} // namespace dsejet

