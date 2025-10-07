#pragma once

#include "jetbus/commands.hpp"
#include "jetbus/types.hpp"

#include <chrono>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <future>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace jetbus {

class JetBusClient {
public:
    struct Options {
        std::string url;
        bool enable_reconnect{true};
        std::chrono::milliseconds reconnect_initial_delay{std::chrono::milliseconds{500}};
        std::chrono::milliseconds reconnect_max_delay{std::chrono::seconds{30}};
        std::chrono::milliseconds request_timeout{std::chrono::seconds{5}};
    };

    using DataCallback = std::function<void(const std::string& path, const std::string& value, JetEventType event)>;
    using FetchCallback = std::function<void(bool success, const std::string& token)>;

    explicit JetBusClient(Options options);
    ~JetBusClient();

    JetBusClient(const JetBusClient&) = delete;
    JetBusClient& operator=(const JetBusClient&) = delete;

    void set_data_callback(DataCallback callback);
    void set_fetch_callback(FetchCallback callback);

    void connect();
    void disconnect();

    std::string fetch(const std::string& path);
    void unfetch(const std::string& token);
    void set(const std::string& path, const nlohmann::json& value);

    std::optional<std::string> read_cached(const std::string& path) const;
    std::unordered_map<std::string, std::string> snapshot() const;

    bool wait_for(const std::string& path,
                  const std::function<bool(const std::string&)>& predicate,
                  std::chrono::milliseconds timeout);

private:
    struct PendingMessage {
        std::string payload;
        std::optional<std::string> token;
        std::shared_ptr<std::promise<nlohmann::json>> promise;
    };

    void run();
    void enqueue(PendingMessage message);
    void handle_message(const std::string& text);
    void notify_fetch(const std::string& token, bool success);

    Options options_;

    mutable std::mutex state_mutex_;
    std::unordered_map<std::string, std::string> cache_;
    DataCallback data_callback_;
    FetchCallback fetch_callback_;

    mutable std::condition_variable cache_cv_;

    std::thread worker_;
    std::atomic<bool> running_{false};

    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::deque<PendingMessage> queue_;

    std::mutex pending_mutex_;
    std::unordered_map<int, std::string> pending_tokens_;
    std::unordered_map<int, std::shared_ptr<std::promise<nlohmann::json>>> pending_promises_;

    std::atomic<int> next_id_{1};
};

} // namespace jetbus
