#include "dse_jet/DseJetProtocolClient.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

namespace dse::jet
{
namespace
{
    std::string makeHostHeader(const std::string& host, unsigned short port)
    {
        std::ostringstream oss;
        oss << host << ':' << port;
        return oss.str();
    }

    std::string dumpJson(const Json& json)
    {
        return json.dump();
    }

    std::string valueToString(const Json& value)
    {
        if (value.is_string())
        {
            return value.get<std::string>();
        }

        return value.dump();
    }
}

DseJetProtocolClient::DseJetProtocolClient(std::string ipAddress,
                                           std::vector<std::string> defaultPaths,
                                           unsigned short port,
                                           std::string endpoint)
    : ipAddress_(std::move(ipAddress))
    , port_(port)
    , endpoint_(std::move(endpoint))
    , defaultSubscriptions_(std::move(defaultPaths))
{
    if (ipAddress_.empty())
    {
        throw std::invalid_argument("ipAddress must not be empty");
    }
}

DseJetProtocolClient::~DseJetProtocolClient()
{
    try
    {
        disconnect();
    }
    catch (...)
    {
        // Destructor must not throw.
    }
}

void DseJetProtocolClient::setLogHandler(LogHandler handler)
{
    logHandler_ = std::move(handler);
}

void DseJetProtocolClient::setUpdateHandler(UpdateHandler handler)
{
    updateHandler_ = std::move(handler);
}

void DseJetProtocolClient::connect(std::chrono::milliseconds timeout)
{
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        if (connected_)
        {
            return;
        }
        timeout_ = timeout;
    }

    auto websocket = std::make_shared<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>(ioContext_);
    boost::asio::ip::tcp::resolver resolver(ioContext_);
    const auto portString = std::to_string(port_);
    auto const results = resolver.resolve(ipAddress_, portString);
    boost::system::error_code ec;
    auto const endpoint = boost::asio::connect(websocket->next_layer(), results, ec);
    if (ec)
    {
        throw std::runtime_error("Failed to resolve or connect to device: " + ec.message());
    }

    const auto hostHeader = makeHostHeader(ipAddress_, port_);
    websocket->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
    websocket->next_layer().expires_after(timeout_);
    websocket->handshake(hostHeader, endpoint_);
    websocket->text(true);

    {
        std::lock_guard<std::mutex> socketLock(websocketMutex_);
        websocket_ = std::move(websocket);
    }

    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        connected_ = true;
    }

    stopReader_.store(false);
    startReadLoop();

    for (const auto& path : defaultSubscriptions_)
    {
        try
        {
            subscribe(path);
        }
        catch (const std::exception& ex)
        {
            if (logHandler_)
            {
                logHandler_(std::string{"Failed to subscribe to path "} + path + ": " + ex.what());
            }
        }
    }
}

void DseJetProtocolClient::disconnect()
{
    bool wasConnected = false;
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        wasConnected = connected_;
        connected_ = false;
    }

    if (!wasConnected)
    {
        return;
    }

    stopReadLoop();

    std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;
    {
        std::lock_guard<std::mutex> lock(websocketMutex_);
        socket = std::move(websocket_);
    }

    if (socket)
    {
        boost::system::error_code ec;
        socket->close(boost::beast::websocket::close_code::normal, ec);
    }

    rejectAllPending("Connection closed");

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cache_.clear();
        subscriptions_.clear();
    }
}

bool DseJetProtocolClient::isConnected() const noexcept
{
    std::lock_guard<std::mutex> lock(connectionMutex_);
    return connected_;
}

std::optional<std::string> DseJetProtocolClient::getCachedValue(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = cache_.find(path);
    if (it == cache_.end())
    {
        return std::nullopt;
    }
    return it->second;
}

bool DseJetProtocolClient::isSubscribedPath(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    return subscriptions_.find(path) != subscriptions_.end();
}

Json DseJetProtocolClient::fetchOnce(const std::string& path)
{
    Json request = {
        {"type", "fetch"},
        {"path", path}
    };

    auto response = sendRequest(std::move(request));
    if (!response.contains("value"))
    {
        throw std::runtime_error("Fetch response does not contain a value field");
    }

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        cache_[path] = valueToString(response["value"]);
    }

    if (updateHandler_)
    {
        updateHandler_(path, response["value"]);
    }

    return response["value"];
}

void DseJetProtocolClient::writeValue(const std::string& path, const Json& value)
{
    Json request = {
        {"type", "set"},
        {"path", path},
        {"value", value}
    };

    sendRequest(std::move(request));
}

void DseJetProtocolClient::subscribe(const std::string& path)
{
    if (path.empty())
    {
        throw std::invalid_argument("path must not be empty");
    }

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        if (subscriptions_.find(path) != subscriptions_.end())
        {
            return;
        }
    }

    Json request = {
        {"type", "subscribe"},
        {"path", path}
    };

    sendRequest(std::move(request));

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        subscriptions_.insert(path);
    }
}

void DseJetProtocolClient::unsubscribe(const std::string& path)
{
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        if (subscriptions_.find(path) == subscriptions_.end())
        {
            return;
        }
    }

    Json request = {
        {"type", "unsubscribe"},
        {"path", path}
    };

    sendRequest(std::move(request));

    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        subscriptions_.erase(path);
        cache_.erase(path);
    }
}

const std::unordered_map<std::string, std::string>& DseJetProtocolClient::cachedValues() const noexcept
{
    return cache_;
}

void DseJetProtocolClient::ensureConnected() const
{
    if (!isConnected())
    {
        throw std::runtime_error("Jet connection is not established");
    }
}

void DseJetProtocolClient::startReadLoop()
{
    readerThread_ = std::thread([this]() { readLoop(); });
}

void DseJetProtocolClient::stopReadLoop()
{
    stopReader_.store(true);

    std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;
    {
        std::lock_guard<std::mutex> lock(websocketMutex_);
        socket = websocket_;
    }

    if (socket)
    {
        boost::system::error_code ec;
        socket->close(boost::beast::websocket::close_code::normal, ec);
    }

    if (readerThread_.joinable())
    {
        readerThread_.join();
    }
}

void DseJetProtocolClient::readLoop()
{
    std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;
    {
        std::lock_guard<std::mutex> lock(websocketMutex_);
        socket = websocket_;
    }

    if (!socket)
    {
        return;
    }

    boost::beast::flat_buffer buffer;

    while (!stopReader_.load())
    {
        buffer.consume(buffer.size());
        boost::system::error_code ec;
        socket->read(buffer, ec);
        if (ec)
        {
            if (logHandler_)
            {
                logHandler_(std::string{"Jet reader loop terminated: "} + ec.message());
            }

            rejectAllPending(ec.message());
            {
                std::lock_guard<std::mutex> lock(connectionMutex_);
                connected_ = false;
            }
            break;
        }

        const auto payload = boost::beast::buffers_to_string(buffer.data());
        handleIncomingMessage(payload);
    }
}

void DseJetProtocolClient::handleIncomingMessage(const std::string& payload)
{
    if (payload.empty())
    {
        return;
    }

    Json message;
    try
    {
        message = Json::parse(payload);
    }
    catch (const std::exception& ex)
    {
        if (logHandler_)
        {
            logHandler_(std::string{"Failed to parse Jet payload: "} + ex.what());
        }
        return;
    }

    if (message.contains("id"))
    {
        const auto id = message["id"].get<int>();
        std::shared_ptr<PendingResponse> pending;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            auto it = pending_.find(id);
            if (it != pending_.end())
            {
                pending = it->second;
                pending_.erase(it);
            }
        }

        if (pending)
        {
            std::unique_lock<std::mutex> lock(pending->mutex);
            if (message.contains("error"))
            {
                pending->error = message["error"].dump();
            }
            else
            {
                pending->payload = message;
            }
            pending->ready = true;
            lock.unlock();
            pending->cv.notify_all();
        }
        return;
    }

    if (message.contains("path") && message.contains("value"))
    {
        const auto path = message["path"].get<std::string>();
        const auto value = message["value"];

        {
            std::lock_guard<std::mutex> lock(cacheMutex_);
            cache_[path] = valueToString(value);
        }

        if (updateHandler_)
        {
            updateHandler_(path, value);
        }

        if (logHandler_)
        {
            logHandler_(std::string{"Received update for path "} + path + ": " + value.dump());
        }
    }
    else if (logHandler_)
    {
        logHandler_(std::string{"Received unhandled Jet payload: "} + payload);
    }
}

Json DseJetProtocolClient::sendRequest(Json request)
{
    ensureConnected();

    auto pending = std::make_shared<PendingResponse>();
    int id = 0;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        id = nextRequestId_++;
        request["id"] = id;
        pending_[id] = pending;
    }

    const auto payload = dumpJson(request);
    {
        std::lock_guard<std::mutex> lock(websocketMutex_);
        if (!websocket_)
        {
            std::lock_guard<std::mutex> pendingLock(pendingMutex_);
            pending_.erase(id);
            throw std::runtime_error("Jet WebSocket connection is not available");
        }

        boost::system::error_code ec;
        websocket_->write(boost::asio::buffer(payload), ec);
        if (ec)
        {
            std::lock_guard<std::mutex> pendingLock(pendingMutex_);
            pending_.erase(id);
            throw std::runtime_error("Failed to send Jet request: " + ec.message());
        }
    }

    std::unique_lock<std::mutex> lock(pending->mutex);
    if (!pending->cv.wait_for(lock, timeout_, [&pending]() { return pending->ready; }))
    {
        lock.unlock();
        std::lock_guard<std::mutex> pendingLock(pendingMutex_);
        pending_.erase(id);
        throw std::runtime_error("Timeout while waiting for Jet response");
    }

    if (pending->error)
    {
        throw std::runtime_error(*pending->error);
    }

    return pending->payload;
}

void DseJetProtocolClient::rejectAllPending(const std::string& message)
{
    std::vector<std::shared_ptr<PendingResponse>> pendings;
    {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        for (auto& entry : pending_)
        {
            pendings.push_back(entry.second);
        }
        pending_.clear();
    }

    for (auto& pending : pendings)
    {
        if (!pending)
        {
            continue;
        }

        std::unique_lock<std::mutex> lock(pending->mutex);
        pending->error = message;
        pending->ready = true;
        lock.unlock();
        pending->cv.notify_all();
    }
}

*** End Patch
