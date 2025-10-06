#include "dse_jet/DseJetConnection.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace dse::jet
{
namespace
{
    std::string jsonValueToString(const Json& value)
    {
        if (value.is_string())
        {
            return value.get<std::string>();
        }

        return value.dump();
    }
}

DseJetConnection::DseJetConnection(std::string ipAddress)
    : ipAddress_(std::move(ipAddress))
    , protocolClient_(ipAddress_, defaultFetchTargets())
{
    if (ipAddress_.empty())
    {
        throw std::invalid_argument("ipAddress must not be empty");
    }

    protocolClient_.setLogHandler([this](const std::string& message) {
        if (logHandler_)
        {
            logHandler_(message);
        }
    });

    protocolClient_.setUpdateHandler([this](const std::string&, const Json&) {
        isConnected_.store(protocolClient_.isConnected());
        if (updateHandler_)
        {
            updateHandler_();
        }
    });
}

void DseJetConnection::setLogHandler(LogHandler handler)
{
    logHandler_ = std::move(handler);
}

void DseJetConnection::setUpdateHandler(UpdateHandler handler)
{
    updateHandler_ = std::move(handler);
}

void DseJetConnection::connect(std::chrono::milliseconds timeout)
{
    protocolClient_.connect(timeout);
    isConnected_.store(protocolClient_.isConnected());
}

void DseJetConnection::disconnect()
{
    protocolClient_.disconnect();
    isConnected_.store(false);
}

bool DseJetConnection::isConnected() const noexcept
{
    return isConnected_.load();
}

const std::string& DseJetConnection::ipAddress() const noexcept
{
    return ipAddress_;
}

std::optional<std::string> DseJetConnection::readFromCache(const std::string& path) const
{
    return protocolClient_.getCachedValue(path);
}

std::string DseJetConnection::readFromDevice(const std::string& path)
{
    if (protocolClient_.isSubscribedPath(path))
    {
        auto cached = protocolClient_.getCachedValue(path);
        if (cached)
        {
            return *cached;
        }
    }

    auto value = protocolClient_.fetchOnce(path);
    return jsonValueToString(value);
}

void DseJetConnection::write(const std::string& path, const std::string& value)
{
    protocolClient_.writeValue(path, Json(value));
}

void DseJetConnection::write(const std::string& path, int value)
{
    protocolClient_.writeValue(path, Json(value));
}

void DseJetConnection::write(const std::string& path, double value)
{
    protocolClient_.writeValue(path, Json(value));
}

const std::unordered_map<std::string, std::string>& DseJetConnection::allData() const noexcept
{
    return protocolClient_.cachedValues();
}

std::vector<std::string> DseJetConnection::defaultFetchTargets()
{
    return {
        "6002/02",
        "6012/01",
        "6013/01",
        "6015/01",
        "6016/01",
        "601A/01",
        "6113/01",
        "611C/01",
        "611C/02",
        "611C/03",
        "6141/02",
        "6143/00",
        "6144/00",
        "6153/00",
        "6002/00",
    };
}

}

