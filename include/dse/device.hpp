#pragma once

#include "dse/types.hpp"
#include "jetbus/client.hpp"
#include "jetbus/process_data.hpp"

#include <chrono>
#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace dse {

class Device {
public:
    struct Options {
        jetbus::JetBusClient::Options client_options;
        std::chrono::milliseconds process_data_interval{std::chrono::milliseconds{200}};
        bool auto_fetch_process_data{true};
    };

    explicit Device(Options options);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    void connect();
    void disconnect();

    bool is_connected() const noexcept { return connected_; }

    void set_process_data_callback(std::function<void(const jetbus::ProcessData&)> callback);

    const jetbus::ProcessData& process_data() const;

    double net_weight() const;
    double gross_weight() const;
    double tare_weight() const;
    std::string unit() const;
    int decimals() const;
    dse::TareMode tare_mode() const;
    bool weight_stable() const;
    bool zero_required() const;
    bool center_of_zero() const;
    bool inside_zero() const;
    bool legal_for_trade() const;
    bool underload() const;
    bool overload() const;
    bool higher_safe_load_limit() const;
    bool general_scale_error() const;
    bool scale_alarm() const;

    std::int32_t weight_step() const;
    std::int32_t scale_range() const;
    std::int32_t maximum_capacity() const;
    std::int32_t zero_value() const;
    std::int32_t zero_signal() const;
    std::int32_t nominal_signal() const;
    std::string identification();
    std::string firmware_version();
    std::uint32_t serial_number();

    void set_unit(const std::string& unit_code);
    void set_manual_tare(double value);
    void set_maximum_capacity(std::int32_t value);
    void set_zero_signal(std::int32_t value);
    void set_nominal_signal(std::int32_t value);

    void save_all_parameters();
    void restore_default_parameters();

    void zero();
    void tare();
    void set_gross();
    void record_weight();

    bool adjust_zero_signal();
    bool adjust_nominal_signal();
    bool adjust_nominal_signal_with_calibration_weight(double weight);
    void calculate_adjustment(double scale_zero_mvv, double capacity_mvv);

private:
    enum class CommandStatus : std::uint32_t {
        Ongoing = 1634168417,
        Ok = 1801543519,
        ErrorE1 = 826629983,
        ErrorE2 = 843407199,
        ErrorE3 = 860184415
    };

    Options options_;
    mutable jetbus::JetBusClient client_;
    std::function<void(const jetbus::ProcessData&)> process_callback_;

    mutable std::mutex process_mutex_;
    jetbus::ProcessData process_data_{};

    std::atomic<bool> connected_{false};
    std::unordered_map<std::string, std::string> subscriptions_;

    std::string ensure_value(const jetbus::Command& command, std::chrono::milliseconds timeout) const;
    std::int32_t read_int(const jetbus::Command& command) const;
    void write_int(const jetbus::Command& command, std::int32_t value);
    void send_scale_command(std::uint32_t command_value);
    bool wait_for_status(CommandStatus desired, std::chrono::milliseconds timeout);
    CommandStatus read_status() const;
    void refresh_process_data();
    static std::uint32_t unit_code_from_string(const std::string& unit);
    static constexpr double conversion_factor_mvv_to_d = 1000000.0;
};

} // namespace dse
