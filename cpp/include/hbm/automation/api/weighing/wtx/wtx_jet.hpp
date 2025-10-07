#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "hbm/automation/api/data/jet_process_data.hpp"
#include "hbm/automation/api/weighing/base_wt_device.hpp"
#include "hbm/automation/api/weighing/wtx/jet/jet_bus_connection.hpp"
#include "hbm/automation/api/weighing/wtx/jet/jet_bus_commands.hpp"

namespace hbm::automation::api::weighing::wtx {

class WTXJet : public BaseWTDevice {
public:
    static constexpr int kScaleCommandCalibrateZero = 2053923171;
    static constexpr int kScaleCommandCalibrateNominal = 1852596579;
    static constexpr int kScaleCommandExitCalibrate = 1953069157;
    static constexpr int kScaleCommandTare = 1701994868;
    static constexpr int kScaleCommandZero = 1869768058;
    static constexpr int kScaleCommandSetGross = 1936683623;

    WTXJet(std::shared_ptr<jet::JetBusConnection> connection,
           int timer_interval_ms,
           ProcessDataHandler handler = nullptr);

    ~WTXJet() override;

    std::string connection_type() const;
    bool is_connected() const;

    const data::JetProcessData& process_data() const;

    void set_unit(const std::string& unit);
    void tare();
    void zero();
    void set_manual_tare(double value);

    void start();
    void stop();

private:
    void register_connection_handler();

    std::shared_ptr<jet::JetBusConnection> jet_connection_;
    data::JetProcessData* jet_process_data_{nullptr};
    std::mutex process_mutex_;
};

} // namespace hbm::automation::api::weighing::wtx
