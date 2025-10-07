#include "hbm/automation/api/weighing/wtx/jet/jet_bus_connection.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace hbm::automation::api::weighing::wtx::jet {

JetBusConnection::JetBusConnection(std::string ip_address,
                                   std::string user,
                                   std::string password)
    : ip_address_(std::move(ip_address)), user_(std::move(user)), password_(std::move(password))
{
}

ConnectionType JetBusConnection::connection_type() const
{
    return ConnectionType::Jetbus;
}

bool JetBusConnection::is_connected() const
{
    return connected_;
}

const std::string& JetBusConnection::ip_address() const
{
    return ip_address_;
}

void JetBusConnection::set_ip_address(std::string address)
{
    ip_address_ = std::move(address);
}

void JetBusConnection::connect(int /*timeout_ms*/)
{
    connected_ = true;
}

void JetBusConnection::disconnect()
{
    connected_ = false;
}

std::string JetBusConnection::read_from_buffer(const JetBusCommand& command) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = data_.find(command.path());
    if (iterator == data_.end()) {
        return "0";
    }

    return command.to_string(iterator->second);
}

bool JetBusConnection::write(const JetBusCommand& command, const std::string& value)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[command.path()] = value;
    }

    if (write_hook_) {
        write_hook_(command, value);
    }

    notify_update_handlers();
    return true;
}

bool JetBusConnection::write_integer(const JetBusCommand& command, int value)
{
    const std::string formatted = command.format_value(value);
    return write(command, formatted);
}

void JetBusConnection::register_update_handler(UpdateCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    update_handlers_.push_back(std::move(callback));
}

void JetBusConnection::clear_update_handlers()
{
    std::lock_guard<std::mutex> lock(mutex_);
    update_handlers_.clear();
}

void JetBusConnection::fetch_all()
{
    if (!fetch_function_) {
        return;
    }

    std::map<std::string, std::string> fetched = fetch_function_();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.insert(fetched.begin(), fetched.end());
        for (auto&& [key, value] : fetched) {
            data_[key] = value;
        }
    }

    notify_update_handlers();
}

void JetBusConnection::set_fetch_function(FetchFunction function)
{
    fetch_function_ = std::move(function);
}

void JetBusConnection::set_write_hook(WriteHook hook)
{
    write_hook_ = std::move(hook);
}

void JetBusConnection::notify_update_handlers()
{
    std::vector<UpdateCallback> handlers_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_copy = update_handlers_;
    }

    for (auto& handler : handlers_copy) {
        if (handler) {
            handler();
        }
    }
}

} // namespace hbm::automation::api::weighing::wtx::jet
