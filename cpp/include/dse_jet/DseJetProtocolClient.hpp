#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>

#include "json.hpp"

namespace dse::jet
{
    /**
     * @brief C++ client that encapsulates the communication flow with a DSE Jet device.
     *
     * The class hides the underlying WebSocket interaction and mirrors the behaviour of the
     * former .NET implementation. All interactions are synchronous from the callers perspective
     * while internally a dedicated reader thread is used to keep subscription data up to date.
     */
    class DseJetProtocolClient
    {
    public:
        using LogHandler = std::function<void(const std::string&)>;
        using UpdateHandler = std::function<void(const std::string&, const Json&)>;

        DseJetProtocolClient(std::string ipAddress,
                             std::vector<std::string> defaultPaths = {},
                             unsigned short port = 80,
                             std::string endpoint = "/jet/canopen");
        ~DseJetProtocolClient();

        DseJetProtocolClient(const DseJetProtocolClient&) = delete;
        DseJetProtocolClient& operator=(const DseJetProtocolClient&) = delete;

        void setLogHandler(LogHandler handler);
        void setUpdateHandler(UpdateHandler handler);

        void connect(std::chrono::milliseconds timeout = std::chrono::milliseconds{20000});
        void disconnect();

        [[nodiscard]] bool isConnected() const noexcept;

        [[nodiscard]] std::optional<std::string> getCachedValue(const std::string& path) const;
        [[nodiscard]] bool isSubscribedPath(const std::string& path) const;

        Json fetchOnce(const std::string& path);
        void writeValue(const std::string& path, const Json& value);

        void subscribe(const std::string& path);
        void unsubscribe(const std::string& path);

        [[nodiscard]] const std::unordered_map<std::string, std::string>& cachedValues() const noexcept;

    private:
        struct PendingResponse
        {
            std::mutex mutex;
            std::condition_variable cv;
            bool ready{false};
            Json payload;
            std::optional<std::string> error;
        };

        void ensureConnected() const;
        void startReadLoop();
        void stopReadLoop();
        void readLoop();
        void handleIncomingMessage(const std::string& payload);
        Json sendRequest(Json request);
        void rejectAllPending(const std::string& message);

        std::string ipAddress_;
        unsigned short port_;
        std::string endpoint_;
        std::vector<std::string> defaultSubscriptions_;

        mutable std::mutex connectionMutex_;
        bool connected_{false};

        std::chrono::milliseconds timeout_{20000};

        mutable std::mutex websocketMutex_;
        boost::asio::io_context ioContext_;
        std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> websocket_;

        std::atomic<bool> stopReader_{false};
        std::thread readerThread_;

        mutable std::mutex cacheMutex_;
        std::unordered_map<std::string, std::string> cache_;
        std::unordered_set<std::string> subscriptions_;

        std::mutex pendingMutex_;
        std::unordered_map<int, std::shared_ptr<PendingResponse>> pending_;
        int nextRequestId_{1};

        LogHandler logHandler_;
        UpdateHandler updateHandler_;
    };
}

