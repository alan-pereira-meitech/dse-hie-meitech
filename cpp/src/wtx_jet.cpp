#include "hbm/automation/api/weighing/wtx/wtx_jet.hpp"

#include <stdexcept>
#include <utility>

#include "hbm/automation/api/utils/measurement_utils.hpp"

namespace hbm::automation::api::weighing::wtx {

WTXJet::WTXJet(std::shared_ptr<jet::JetBusConnection> connection,
               int timer_interval_ms,
               ProcessDataHandler handler)
    : BaseWTDevice(connection, timer_interval_ms), jet_connection_(std::move(connection))
{
    auto process_data = std::make_unique<data::JetProcessData>();
    jet_process_data_ = process_data.get();
    set_process_data(std::move(process_data));

    if (handler) {
        add_process_data_handler(std::move(handler));
    }

    register_connection_handler();
}

WTXJet::~WTXJet()
{
    stop();
}

std::string WTXJet::connection_type() const
{
    return "Jetbus";
}

bool WTXJet::is_connected() const
{
    return connection().is_connected();
}

const data::JetProcessData& WTXJet::process_data() const
{
    return *jet_process_data_;
}

void WTXJet::set_unit(const std::string& unit)
{
    int value = 0;
    if (unit == "kg") {
        value = 0x00020000;
    } else if (unit == "g") {
        value = 0x004B0000;
    } else if (unit == "lb") {
        value = 0x00A60000;
    } else if (unit == "t") {
        value = 0x004C0000;
    } else {
        throw std::invalid_argument("Unsupported unit");
    }

    connection().write_integer(jet::JetBusCommands::CIA461Unit(), value);
}

void WTXJet::tare()
{
    connection().write_integer(jet::JetBusCommands::CIA461ScaleCommand(), kScaleCommandTare);
}

void WTXJet::zero()
{
    connection().write_integer(jet::JetBusCommands::CIA461ScaleCommand(), kScaleCommandZero);
}

void WTXJet::set_manual_tare(double value)
{
    const int digits = utils::double_to_digit(value, jet_process_data_->decimals());
    connection().write_integer(jet::JetBusCommands::CIA461TareValue(), digits);
}

void WTXJet::start()
{
    BaseWTDevice::start();
}

void WTXJet::stop()
{
    BaseWTDevice::stop();
}

void WTXJet::register_connection_handler()
{
    connection().register_update_handler([this]() {
        std::lock_guard<std::mutex> lock(process_mutex_);
        jet_process_data_->refresh(connection());
        notify_process_data_received();
    });
}

} // namespace hbm::automation::api::weighing::wtx
