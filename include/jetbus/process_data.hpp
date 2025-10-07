#pragma once

#include "dse/types.hpp"
#include "jetbus/commands.hpp"
#include "jetbus/measurement_utils.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace jetbus {

class ProcessData {
public:
    ProcessData();

    void update(const std::unordered_map<std::string, std::string>& cache);

    dse::ApplicationMode application_mode() const noexcept { return application_mode_; }
    const dse::WeightValues& weight() const noexcept { return weight_; }
    const dse::PrintableWeightValues& printable_weight() const noexcept { return printable_weight_; }
    const std::string& unit() const noexcept { return unit_; }
    int decimals() const noexcept { return decimals_; }
    dse::TareMode tare_mode() const noexcept { return tare_mode_; }
    bool weight_stable() const noexcept { return weight_stable_; }
    bool center_of_zero() const noexcept { return center_of_zero_; }
    bool inside_zero() const noexcept { return inside_zero_; }
    bool zero_required() const noexcept { return zero_required_; }
    int scale_range() const noexcept { return scale_range_; }
    bool legal_for_trade() const noexcept { return legal_for_trade_; }
    bool underload() const noexcept { return underload_; }
    bool overload() const noexcept { return overload_; }
    bool higher_safe_load_limit() const noexcept { return higher_safe_load_limit_; }
    bool general_scale_error() const noexcept { return general_scale_error_; }
    bool scale_alarm() const noexcept { return scale_alarm_; }

private:
    static std::optional<std::string> find_value(const std::unordered_map<std::string, std::string>& cache,
                                                 const Command& command);
    static int read_int(const std::unordered_map<std::string, std::string>& cache,
                        const Command& command,
                        int fallback = 0);
    static std::string unit_from_id(int id);
    static dse::TareMode evaluate_tare_mode(int tare, int preset_tare);
    static void update_printable(dse::PrintableWeightValues& printable,
                                 const dse::WeightValues& weight,
                                 int decimals);

    dse::ApplicationMode application_mode_{dse::ApplicationMode::Standard};
    dse::WeightValues weight_{};
    dse::PrintableWeightValues printable_weight_{};
    std::string unit_{};
    int decimals_{0};
    dse::TareMode tare_mode_{dse::TareMode::None};
    bool weight_stable_{false};
    bool center_of_zero_{false};
    bool inside_zero_{false};
    bool zero_required_{false};
    int scale_range_{0};
    bool legal_for_trade_{false};
    bool underload_{false};
    bool overload_{false};
    bool higher_safe_load_limit_{false};
    bool general_scale_error_{false};
    bool scale_alarm_{false};
};

} // namespace jetbus
