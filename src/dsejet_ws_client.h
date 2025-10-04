#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>

#include "dsejet/config.h"
#include "logging.h"

namespace dsejet {

enum class JetRequestType {
    Authenticate,
    Fetch,
    Set,
    Ping,
};

struct JetEvent {
    std::string path;
    std::string event;
    nlohmann::json value;
};

nlohmann::json MakeAuthenticateRequest(std::uint64_t id, const std::string &user, const std::string &password);
nlohmann::json MakeFetchRequest(std::uint64_t id, const std::string &path, bool subscribe);
nlohmann::json MakeSetRequest(std::uint64_t id, const std::string &path, const nlohmann::json &value);
nlohmann::json MakePingFrame();
bool ParseJetEvent(const nlohmann::json &message, JetEvent &event);

class DSEJetWSClient : public std::enable_shared_from_this<DSEJetWSClient> {
public:
    using EventHandler = std::function<void(const std::string &, const nlohmann::json &)>;
    using ErrorHandler = std::function<void(const std::string &)>;

    DSEJetWSClient(const Config &config, std::shared_ptr<Logger> logger);
    ~DSEJetWSClient();

    std::future<void> connect();
    void close();

    bool is_connected() const { return connected_; }

    std::future<nlohmann::json> fetch(const std::string &path, bool subscribe, std::chrono::milliseconds timeout);
    std::future<nlohmann::json> fetch_once(const std::string &path, std::chrono::milliseconds timeout);
    std::future<nlohmann::json> set_value(const std::string &path, const nlohmann::json &value, std::chrono::milliseconds timeout);

    void subscribe(const std::vector<std::string> &paths, const EventHandler &handler);

    void set_error_handler(ErrorHandler handler);

    std::chrono::milliseconds current_backoff() const { return backoff_; }

    class WebSocketAdapter {
    public:
        virtual ~WebSocketAdapter() = default;
        virtual void async_connect(const std::string &host, std::uint16_t port, const std::string &target,
                                   std::function<void(const boost::system::error_code &)> handler) = 0;
        virtual void async_write(const std::shared_ptr<std::string> &data,
                                 std::function<void(const boost::system::error_code &, std::size_t)> handler) = 0;
        virtual void async_read(boost::beast::flat_buffer &buffer,
                                std::function<void(const boost::system::error_code &, std::size_t)> handler) = 0;
        virtual void async_ping(std::function<void(const boost::system::error_code &)> handler) = 0;
        virtual void async_close(std::function<void(const boost::system::error_code &)> handler) = 0;
        virtual bool is_open() const = 0;
    };

private:
    struct PendingRequest {
        std::shared_ptr<std::promise<nlohmann::json>> promise;
        JetRequestType type;
        std::chrono::steady_clock::time_point deadline;
    };

    void do_connect();
    void handle_connect_result(const boost::system::error_code &ec);
    void on_connected();
    void start_read();
    void schedule_ping();
    void schedule_reconnect();
    void handle_error(const std::string &context, const boost::system::error_code &ec);
    void handle_error(const std::string &message);
    void mark_ready();
    void fail_initial(const std::string &message);
    void reset_connection();

    void enqueue_message(const std::string &message);
    void do_write();
    void handle_message(const std::string &payload);
    void handle_response(const nlohmann::json &message);
    void handle_event(const nlohmann::json &message);

    void check_timeouts();
    void schedule_timeout_watchdog();
    void resolve_pending_with_error(const std::string &message);

    std::future<nlohmann::json> send_request(const nlohmann::json &payload, JetRequestType type, std::chrono::milliseconds timeout);

    Config config_;
    std::shared_ptr<Logger> logger_;

    boost::asio::io_context io_context_;
    std::optional<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;
    std::thread io_thread_;

    std::unique_ptr<WebSocketAdapter> ws_;
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;

    boost::asio::steady_timer ping_timer_;
    boost::asio::steady_timer reconnect_timer_;
    boost::asio::steady_timer timeout_timer_;
    boost::beast::flat_buffer read_buffer_;

    std::deque<std::shared_ptr<std::string>> write_queue_;
    bool write_in_progress_ = false;

    std::unordered_map<std::uint64_t, PendingRequest> pending_requests_;
    std::unordered_map<std::string, std::vector<EventHandler>> subscriptions_;

    std::mutex promise_mutex_;
    std::shared_ptr<std::promise<void>> initial_connect_promise_;

    std::optional<ErrorHandler> error_handler_;

    std::atomic<bool> stop_{false};
    std::atomic<bool> connected_{false};
    bool connecting_ = false;

    std::chrono::milliseconds backoff_;
    std::chrono::milliseconds max_backoff_;

    std::uint64_t next_request_id_ = 1;
};

} // namespace dsejet

