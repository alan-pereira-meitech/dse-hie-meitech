#pragma once

#include "jetbus/types.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace jetbus {

struct Command {
    std::string name;
    DataType type{DataType::Nil};
    std::string path;
    int bit_index{0};
    int bit_length{0};

    [[nodiscard]] int to_int(const std::string& raw) const;
    [[nodiscard]] std::string to_string(const std::string& raw) const;
};

namespace commands {

const Command& cia461_net_value();
const Command& cia461_gross_value();
const Command& cia461_tare_value();
const Command& cia461_decimals();
const Command& cia461_unit();
const Command& cia461_weight_status();
const Command& cia461_weight_status_general_weight_error();
const Command& cia461_weight_status_scale_alarm();
const Command& cia461_weight_status_weight_moving();
const Command& cia461_weight_status_scale_seal_is_open();
const Command& cia461_weight_status_scale_range();
const Command& cia461_weight_status_manual_tare();
const Command& cia461_weight_status_weight_type();
const Command& cia461_weight_status_zero_required();
const Command& cia461_weight_status_center_of_zero();
const Command& cia461_weight_status_inside_zero();
const Command& cia461_weight_status_limit_status();
const Command& cia461_scale_command();
const Command& cia461_scale_command_status();
const Command& cia461_scale_maximum_capacity();
const Command& cia461_multi_interval_range_control();
const Command& cia461_multi_limit1();
const Command& cia461_multi_limit2();
const Command& cia461_weight_step();
const Command& cia461_zero_value();
const Command& cia461_calibration_weight();
const Command& cia461_save_all_parameters();
const Command& dse_restore_defaults();
const Command& dse_serial_number();
const Command& dse_identification();
const Command& dse_firmware_version();
const Command& dse_zero_signal();
const Command& dse_nominal_signal();
const Command& ldw_zero_value();
const Command& lwt_nominal_value();
const Command& sto_record_weight();
const Command& imd_application_mode();
const Command& cia461_scale_command_status_raw();
const std::vector<Command>& all();
const Command* find_by_path(std::string_view path);

} // namespace commands

} // namespace jetbus
