#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "DseJetProtocolClient.hpp"

namespace dse::jet
{
    /**
     * @brief High level connection wrapper used to communicate with a DSE Jet device.
     */
    class DseJetConnection
    {
    public:
        using LogHandler = DseJetProtocolClient::LogHandler;
        using UpdateHandler = std::function<void()>;

        explicit DseJetConnection(std::string ipAddress);

        void setLogHandler(LogHandler handler);
        void setUpdateHandler(UpdateHandler handler);

        void connect(std::chrono::milliseconds timeout = std::chrono::milliseconds{20000});
        void disconnect();

        [[nodiscard]] bool isConnected() const noexcept;
        [[nodiscard]] const std::string& ipAddress() const noexcept;

        [[nodiscard]] std::optional<std::string> readFromCache(const std::string& path) const;
        std::string readFromDevice(const std::string& path);

        void write(const std::string& path, const std::string& value);
        void write(const std::string& path, int value);
        void write(const std::string& path, double value);

        [[nodiscard]] const std::unordered_map<std::string, std::string>& allData() const noexcept;

    private:
        static std::vector<std::string> defaultFetchTargets();

        std::string ipAddress_;
        DseJetProtocolClient protocolClient_;
        std::atomic<bool> isConnected_{false};
        UpdateHandler updateHandler_;
        LogHandler logHandler_;
    };
}

