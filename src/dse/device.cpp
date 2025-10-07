#include "dse/device.hpp"

#include "jetbus/commands.hpp"
#include "jetbus/measurement_utils.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace dse {
namespace {

constexpr std::uint32_t kScaleCommandCalibrateZero = 2053923171U;
constexpr std::uint32_t kScaleCommandCalibrateNominal = 1852596579U;
constexpr std::uint32_t kScaleCommandExitCalibrate = 1953069157U;
constexpr std::uint32_t kScaleCommandTare = 1701994868U;
constexpr std::uint32_t kScaleCommandZero = 1869768058U;
constexpr std::uint32_t kScaleCommandSetGross = 1936683623U;
constexpr std::chrono::milliseconds kDefaultCommandTimeout{10000};

const std::vector<std::string> kDefaultFetchPaths = {
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
    "6142/00",
    "6143/00",
    "6144/00",
    "6153/00"
};

} // namespace

Device::Device(Options options)
    : options_(std::move(options)),
      client_(options_.client_options) {
    client_.set_data_callback([this](const std::string&, const std::string&, jetbus::JetEventType) {
        refresh_process_data();
    });
}

Device::~Device() {
    disconnect();
}

void Device::set_process_data_callback(std::function<void(const jetbus::ProcessData&)> callback) {
    std::scoped_lock lock(process_mutex_);
    process_callback_ = std::move(callback);
}

void Device::connect() {
    if (connected_) {
        return;
    }
    client_.connect();

    for (const auto& path : kDefaultFetchPaths) {
        auto token = client_.fetch(path);
        subscriptions_.emplace(path, token);
    }

    // antes do wait_for: garanta a inscrição (fetch) do path de process data
    const auto pd_path = jetbus::commands::cia461_net_value().path;
    client_.fetch(pd_path);

    // aumente o prazo para a 1ª amostra chegar
    auto tmo = std::chrono::seconds(5);
    fprintf(stderr,
            "[DEBUG] Waiting up to %lld ms for process data at path %s\n",
            (long long)std::chrono::duration_cast<std::chrono::milliseconds>(tmo).count(),
            pd_path.c_str());

    // a condição continua “valor não vazio”
    const bool ready = client_.wait_for(pd_path,
                                        [](const std::string& value) { return !value.empty(); },
                                        tmo);
    if (!ready) {
        // detalhe útil no erro
        throw jetbus::JetBusError(std::string("Timeout waiting for process data at path: ") + pd_path);
    }

    refresh_process_data();
    connected_ = true;
}

void Device::disconnect() {
    if (!connected_) {
        client_.disconnect();
        return;
    }
    for (const auto& [path, token] : subscriptions_) {
        (void)path;
        client_.unfetch(token);
    }
    subscriptions_.clear();
    client_.disconnect();
    connected_ = false;
}

const jetbus::ProcessData& Device::process_data() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_;
}

double Device::net_weight() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.weight().net;
}

double Device::gross_weight() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.weight().gross;
}

double Device::tare_weight() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.weight().tare;
}

std::string Device::unit() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.unit();
}

int Device::decimals() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.decimals();
}

dse::TareMode Device::tare_mode() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.tare_mode();
}

bool Device::weight_stable() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.weight_stable();
}

bool Device::zero_required() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.zero_required();
}

bool Device::center_of_zero() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.center_of_zero();
}

bool Device::inside_zero() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.inside_zero();
}

bool Device::legal_for_trade() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.legal_for_trade();
}

bool Device::underload() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.underload();
}

bool Device::overload() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.overload();
}

bool Device::higher_safe_load_limit() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.higher_safe_load_limit();
}

bool Device::general_scale_error() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.general_scale_error();
}

bool Device::scale_alarm() const {
    std::scoped_lock lock(process_mutex_);
    return process_data_.scale_alarm();
}

std::int32_t Device::weight_step() const {
    return read_int(jetbus::commands::cia461_weight_step());
}

std::int32_t Device::scale_range() const {
    return read_int(jetbus::commands::cia461_multi_interval_range_control());
}

std::int32_t Device::maximum_capacity() const {
    return read_int(jetbus::commands::cia461_scale_maximum_capacity());
}

std::int32_t Device::zero_value() const {
    return read_int(jetbus::commands::cia461_zero_value());
}

std::int32_t Device::zero_signal() const {
    return read_int(jetbus::commands::dse_zero_signal());
}

std::int32_t Device::nominal_signal() const {
    return read_int(jetbus::commands::dse_nominal_signal());
}

std::string Device::identification() {
    return ensure_value(jetbus::commands::dse_identification(), kDefaultCommandTimeout);
}

std::string Device::firmware_version() {
    return ensure_value(jetbus::commands::dse_firmware_version(), kDefaultCommandTimeout);
}

std::uint32_t Device::serial_number() {
    return static_cast<std::uint32_t>(read_int(jetbus::commands::dse_serial_number()));
}

void Device::set_unit(const std::string& unit_code) {
    const auto unit_value = unit_code_from_string(unit_code);
    write_int(jetbus::commands::cia461_unit(), static_cast<std::int32_t>(unit_value));
}

void Device::set_manual_tare(double value) {
    const int decimals_value = decimals();
    auto digits = jetbus::double_to_digit(value, decimals_value);
    write_int(jetbus::commands::cia461_tare_value(), digits);
}

void Device::set_maximum_capacity(std::int32_t value) {
    write_int(jetbus::commands::cia461_scale_maximum_capacity(), value);
}

void Device::set_zero_signal(std::int32_t value) {
    write_int(jetbus::commands::dse_zero_signal(), value);
}

void Device::set_nominal_signal(std::int32_t value) {
    write_int(jetbus::commands::dse_nominal_signal(), value);
}

void Device::save_all_parameters() {
    write_int(jetbus::commands::cia461_save_all_parameters(), 0);
}

void Device::restore_default_parameters() {
    write_int(jetbus::commands::dse_restore_defaults(), 0x6c6f6164);
}

void Device::zero() {
    send_scale_command(kScaleCommandZero);
}

void Device::tare() {
    send_scale_command(kScaleCommandTare);
}

void Device::set_gross() {
    send_scale_command(kScaleCommandSetGross);
}

void Device::record_weight() {
    write_int(jetbus::commands::sto_record_weight(), static_cast<std::int32_t>(kScaleCommandTare));
}

bool Device::adjust_zero_signal() {
    send_scale_command(kScaleCommandCalibrateZero);
    return wait_for_status(CommandStatus::Ok, kDefaultCommandTimeout);
}

bool Device::adjust_nominal_signal() {
    send_scale_command(kScaleCommandCalibrateNominal);
    return wait_for_status(CommandStatus::Ok, kDefaultCommandTimeout);
}

bool Device::adjust_nominal_signal_with_calibration_weight(double weight) {
    const int decimals_value = decimals();
    write_int(jetbus::commands::cia461_calibration_weight(), jetbus::double_to_digit(weight, decimals_value));
    send_scale_command(kScaleCommandCalibrateNominal);
    return wait_for_status(CommandStatus::Ok, kDefaultCommandTimeout);
}

void Device::calculate_adjustment(double scale_zero_mvv, double capacity_mvv) {
    const auto scale_zero_d = static_cast<std::int32_t>(std::llround(scale_zero_mvv * conversion_factor_mvv_to_d));
    const auto capacity_d = static_cast<std::int32_t>(std::llround((scale_zero_mvv + capacity_mvv) * conversion_factor_mvv_to_d));
    write_int(jetbus::commands::ldw_zero_value(), scale_zero_d);
    write_int(jetbus::commands::lwt_nominal_value(), capacity_d);
}

std::string Device::ensure_value(const jetbus::Command& command, std::chrono::milliseconds timeout) const {
    auto cached = client_.read_cached(command.path);
    if (cached) {
        return *cached;
    }
    auto token = client_.fetch(command.path);
    bool ready = client_.wait_for(command.path, [](const std::string& value) { return !value.empty(); }, timeout);
    client_.unfetch(token);
    if (!ready) {
        throw jetbus::JetBusError("Timeout while waiting for value on path " + command.path);
    }
    cached = client_.read_cached(command.path);
    if (!cached) {
        throw jetbus::JetBusError("Value not available for path " + command.path);
    }
    return *cached;
}

std::int32_t Device::read_int(const jetbus::Command& command) const {
    const auto raw = ensure_value(command, kDefaultCommandTimeout);
    return command.to_int(raw);
}

void Device::write_int(const jetbus::Command& command, std::int32_t value) {
    nlohmann::json json_value = value;
    client_.set(command.path, json_value);
}

void Device::send_scale_command(std::uint32_t command_value) {
    write_int(jetbus::commands::cia461_scale_command(), static_cast<std::int32_t>(command_value));
    if (!wait_for_status(CommandStatus::Ongoing, kDefaultCommandTimeout)) {
        throw jetbus::JetBusError("Timeout waiting for scale command to start");
    }
    if (!wait_for_status(CommandStatus::Ok, kDefaultCommandTimeout)) {
        throw jetbus::JetBusError("Timeout waiting for scale command to finish");
    }
}

bool Device::wait_for_status(CommandStatus desired, std::chrono::milliseconds timeout) {
    const auto& status_command = jetbus::commands::cia461_scale_command_status();
    auto parse_status = [&](const std::string& value) -> std::optional<std::uint32_t> {
        try {
            return static_cast<std::uint32_t>(status_command.to_int(value));
        } catch (...) {
            return std::nullopt;
        }
    };

    bool ok = client_.wait_for(status_command.path,
                               [&](const std::string& value) {
                                   auto parsed = parse_status(value);
                                   if (!parsed) {
                                       return false;
                                   }
                                   if (desired == CommandStatus::Ongoing && *parsed == static_cast<std::uint32_t>(CommandStatus::Ok)) {
                                       return true;
                                   }
                                   return *parsed == static_cast<std::uint32_t>(desired);
                               },
                               timeout);
    if (!ok) {
        return false;
    }
    if (desired == CommandStatus::Ok) {
        const auto status = read_status();
        if (status != CommandStatus::Ok) {
            throw jetbus::JetBusError("Scale command finished with error");
        }
    }
    return true;
}

Device::CommandStatus Device::read_status() const {
    const auto raw = ensure_value(jetbus::commands::cia461_scale_command_status(), kDefaultCommandTimeout);
    auto status_value = static_cast<std::uint32_t>(jetbus::commands::cia461_scale_command_status().to_int(raw));
    return static_cast<CommandStatus>(status_value);
}

void Device::refresh_process_data() {
    auto snapshot = client_.snapshot();
    std::function<void(const jetbus::ProcessData&)> callback;
    {
        std::scoped_lock lock(process_mutex_);
        process_data_.update(snapshot);
        callback = process_callback_;
    }
    if (callback) {
        callback(process_data_);
    }
}

std::uint32_t Device::unit_code_from_string(const std::string& unit) {
    if (unit == "kg") {
        return 0x00020000;
    }
    if (unit == "g") {
        return 0x004B0000;
    }
    if (unit == "t") {
        return 0x004C0000;
    }
    if (unit == "lb") {
        return 0x00A60000;
    }
    if (unit == "N") {
        return 0x00210000;
    }
    throw std::invalid_argument("Unsupported unit: " + unit);
}

} // namespace dse
