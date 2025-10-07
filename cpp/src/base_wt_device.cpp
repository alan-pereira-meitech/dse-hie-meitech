#include "hbm/automation/api/weighing/base_wt_device.hpp"

#include <algorithm>

namespace hbm::automation::api::weighing {

BaseWTDevice::BaseWTDevice(std::shared_ptr<INetConnection> connection, int timer_interval_ms)
    : connection_(std::move(connection)), interval_(std::chrono::milliseconds(timer_interval_ms))
{
}

BaseWTDevice::~BaseWTDevice()
{
    stop();
}

void BaseWTDevice::start()
{
    if (running_.exchange(true)) {
        return;
    }

    polling_thread_ = std::thread([this]() { polling_loop(); });
}

void BaseWTDevice::stop()
{
    if (!running_.exchange(false)) {
        return;
    }

    if (polling_thread_.joinable()) {
        polling_thread_.join();
    }
}

void BaseWTDevice::restart()
{
    stop();
    start();
}

void BaseWTDevice::add_process_data_handler(ProcessDataHandler handler)
{
    std::lock_guard<std::mutex> lock(handler_mutex_);
    handlers_.push_back(std::move(handler));
}

void BaseWTDevice::clear_process_data_handlers()
{
    std::lock_guard<std::mutex> lock(handler_mutex_);
    handlers_.clear();
}

INetConnection& BaseWTDevice::connection()
{
    return *connection_;
}

const INetConnection& BaseWTDevice::connection() const
{
    return *connection_;
}

void BaseWTDevice::set_process_data(std::unique_ptr<data::IProcessData> process_data)
{
    process_data_ = std::move(process_data);
}

data::IProcessData& BaseWTDevice::process_data()
{
    return *process_data_;
}

const data::IProcessData& BaseWTDevice::process_data() const
{
    return *process_data_;
}

void BaseWTDevice::notify_process_data_received()
{
    ProcessDataReceivedEventArgs args(*process_data_);

    std::vector<ProcessDataHandler> handlers_copy;
    {
        std::lock_guard<std::mutex> lock(handler_mutex_);
        handlers_copy = handlers_;
    }

    for (auto& handler : handlers_copy) {
        if (handler) {
            handler(args);
        }
    }
}

void BaseWTDevice::polling_loop()
{
    while (running_.load()) {
        connection_->fetch_all();
        std::this_thread::sleep_for(interval_);
    }
}

} // namespace hbm::automation::api::weighing
