#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "hbm/automation/api/net_connection.hpp"

namespace hbm::automation::api::weighing::wtx::jet {

class JetBusConnection : public INetConnection {
public:
    using FetchFunction = std::function<std::map<std::string, std::string>()>;
    using WriteHook = std::function<void(const JetBusCommand&, const std::string&)>;

    JetBusConnection(std::string ip_address,
                     std::string user = "Administrator",
                     std::string password = "wtx");

    ConnectionType connection_type() const override;
    bool is_connected() const override;
    const std::string& ip_address() const override;
    void set_ip_address(std::string address) override;
    void connect(int timeout_ms = 20000) override;
    void disconnect() override;

    std::string read_from_buffer(const JetBusCommand& command) const override;
    bool write(const JetBusCommand& command, const std::string& value) override;
    bool write_integer(const JetBusCommand& command, int value) override;

    void register_update_handler(UpdateCallback callback) override;
    void clear_update_handlers() override;
    void fetch_all() override;

    void set_fetch_function(FetchFunction function);
    void set_write_hook(WriteHook hook);

private:
    void notify_update_handlers();

    std::string ip_address_;
    std::string user_;
    std::string password_;
    bool connected_{false};

    mutable std::mutex mutex_;
    std::map<std::string, std::string> data_;
    std::vector<UpdateCallback> update_handlers_;
    FetchFunction fetch_function_;
    WriteHook write_hook_;
};

} // namespace hbm::automation::api::weighing::wtx::jet
