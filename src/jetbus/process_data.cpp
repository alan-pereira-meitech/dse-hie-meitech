#include "jetbus/process_data.hpp"

#include "jetbus/measurement_utils.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace jetbus {

ProcessData::ProcessData() = default;

void ProcessData::update(const std::unordered_map<std::string, std::string>& cache) {
    application_mode_ = static_cast<dse::ApplicationMode>(
        read_int(cache, commands::imd_application_mode(), static_cast<int>(dse::ApplicationMode::Standard)));

    general_scale_error_ = read_int(cache, commands::cia461_weight_status_general_weight_error(), 0) != 0;
    scale_alarm_ = read_int(cache, commands::cia461_weight_status_scale_alarm(), 0) != 0;

    const int limit_status = read_int(cache, commands::cia461_weight_status_limit_status(), 0);
    underload_ = (limit_status == 1);
    overload_ = (limit_status == 2);
    higher_safe_load_limit_ = (limit_status == 3);

    const int manual_tare = read_int(cache, commands::cia461_weight_status_manual_tare(), 0);
    const int weight_type = read_int(cache, commands::cia461_weight_status_weight_type(), 0);
    tare_mode_ = evaluate_tare_mode(manual_tare, weight_type);

    weight_stable_ = read_int(cache, commands::cia461_weight_status_weight_moving(), 0) == 0;
    legal_for_trade_ = read_int(cache, commands::cia461_weight_status_scale_seal_is_open(), 0) == 0;
    scale_range_ = read_int(cache, commands::cia461_weight_status_scale_range(), 0);
    zero_required_ = read_int(cache, commands::cia461_weight_status_zero_required(), 0) != 0;
    center_of_zero_ = read_int(cache, commands::cia461_weight_status_center_of_zero(), 0) != 0;
    inside_zero_ = read_int(cache, commands::cia461_weight_status_inside_zero(), 0) != 0;

    decimals_ = read_int(cache, commands::cia461_decimals(), 0);

    unit_ = unit_from_id(read_int(cache, commands::cia461_unit(), 0));

    const int net_raw = read_int(cache, commands::cia461_net_value(), 0);
    const int gross_raw = read_int(cache, commands::cia461_gross_value(), 0);
    const int tare_raw = read_int(cache, commands::cia461_tare_value(), 0);

    weight_.net = digit_to_double(net_raw, decimals_);
    weight_.gross = digit_to_double(gross_raw, decimals_);
    weight_.tare = digit_to_double(tare_raw, decimals_);

    update_printable(printable_weight_, weight_, decimals_);
}

std::optional<std::string> ProcessData::find_value(const std::unordered_map<std::string, std::string>& cache,
                                                   const Command& command) {
    auto it = cache.find(command.path);
    if (it == cache.end()) {
        return std::nullopt;
    }
    return it->second;
}

int ProcessData::read_int(const std::unordered_map<std::string, std::string>& cache,
                          const Command& command,
                          int fallback) {
    auto value = find_value(cache, command);
    if (!value) {
        return fallback;
    }
    try {
        return command.to_int(*value);
    } catch (...) {
        return fallback;
    }
}

std::string ProcessData::unit_from_id(int id) {
    switch (id) {
        case 0x00020000: return "kg";
        case 0x004B0000: return "g";
        case 0x004C0000: return "t";
        case 0x00A60000: return "lb";
        case 0x00210000: return "N";
        default: return "";
    }
}

dse::TareMode ProcessData::evaluate_tare_mode(int tare, int preset_tare) {
    if (tare > 0) {
        if (preset_tare > 0) {
            return dse::TareMode::PresetTare;
        }
        return dse::TareMode::Tare;
    }
    return dse::TareMode::None;
}

void ProcessData::update_printable(dse::PrintableWeightValues& printable,
                                   const dse::WeightValues& weight,
                                   int decimals) {
    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(decimals);

    stream.str("");
    stream.clear();
    stream << weight.net;
    printable.net = stream.str();

    stream.str("");
    stream.clear();
    stream << weight.gross;
    printable.gross = stream.str();

    stream.str("");
    stream.clear();
    stream << weight.tare;
    printable.tare = stream.str();
}

} // namespace jetbus
