#pragma once

#include <functional>
#include <string>

#include "hbm/automation/api/enums.hpp"
#include "hbm/automation/api/weighing/wtx/jet/jet_bus_command.hpp"

namespace hbm::automation::api {

class INetConnection {
public:
    using UpdateCallback = std::function<void()>;

    virtual ~INetConnection() = default;

    virtual ConnectionType connection_type() const = 0;
    virtual bool is_connected() const = 0;
    virtual const std::string& ip_address() const = 0;
    virtual void set_ip_address(std::string address) = 0;
    virtual void connect(int timeout_ms = 20000) = 0;
    virtual void disconnect() = 0;
    virtual std::string read_from_buffer(const weighing::wtx::jet::JetBusCommand& command) const = 0;

    virtual int read_integer_from_buffer(const weighing::wtx::jet::JetBusCommand& command) const
    {
        return command.to_int(read_from_buffer(command));
    }

    virtual bool write(const weighing::wtx::jet::JetBusCommand& command, const std::string& value) = 0;

    virtual bool write_integer(const weighing::wtx::jet::JetBusCommand& command, int value)
    {
        return write(command, command.format_value(value));
    }

    virtual void register_update_handler(UpdateCallback callback) = 0;
    virtual void clear_update_handlers() = 0;
    virtual void fetch_all() = 0;
};

} // namespace hbm::automation::api
