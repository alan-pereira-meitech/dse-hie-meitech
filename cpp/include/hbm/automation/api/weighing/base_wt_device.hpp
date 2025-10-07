#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "hbm/automation/api/data/process_data.hpp"
#include "hbm/automation/api/net_connection.hpp"
#include "hbm/automation/api/weighing/process_data_event.hpp"

namespace hbm::automation::api::weighing {

class BaseWTDevice {
public:
    BaseWTDevice(std::shared_ptr<INetConnection> connection, int timer_interval_ms);
    virtual ~BaseWTDevice();

    BaseWTDevice(const BaseWTDevice&) = delete;
    BaseWTDevice& operator=(const BaseWTDevice&) = delete;

    void start();
    void stop();
    void restart();

    void add_process_data_handler(ProcessDataHandler handler);
    void clear_process_data_handlers();

    [[nodiscard]] INetConnection& connection();
    [[nodiscard]] const INetConnection& connection() const;

protected:
    void set_process_data(std::unique_ptr<data::IProcessData> process_data);
    [[nodiscard]] data::IProcessData& process_data();
    [[nodiscard]] const data::IProcessData& process_data() const;
    void notify_process_data_received();

private:
    void polling_loop();

    std::shared_ptr<INetConnection> connection_;
    std::unique_ptr<data::IProcessData> process_data_;
    std::chrono::milliseconds interval_;
    std::atomic<bool> running_{false};
    std::thread polling_thread_;

    std::mutex handler_mutex_;
    std::vector<ProcessDataHandler> handlers_;
};

} // namespace hbm::automation::api::weighing
