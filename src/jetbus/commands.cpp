#include "jetbus/commands.hpp"

#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <unordered_map>

namespace jetbus {
namespace {

int extract_bit(int value, int bit_index, int bit_length) {
    int bit_mask = 0;
    switch (bit_length) {
        case 0: bit_mask = 0xFFFF; break;
        case 1: bit_mask = 0x1; break;
        case 2: bit_mask = 0x3; break;
        case 3: bit_mask = 0x7; break;
        case 4: bit_mask = 0xF; break;
        default: bit_mask = (1 << bit_length) - 1; break;
    }
    const int mask = bit_mask << bit_index;
    return (value & mask) >> bit_index;
}

Command make_command(std::string name, DataType type, std::string path, int bit_index = 0, int bit_length = 0) {
    return Command{std::move(name), type, std::move(path), bit_index, bit_length};
}

struct CommandRegistry {
    std::vector<Command> commands;
    std::unordered_map<std::string, std::size_t> by_name;
    std::unordered_multimap<std::string, std::size_t> by_path;

    CommandRegistry();

    const Command& emplace(Command cmd) {
        const std::size_t index = commands.size();
        by_name.emplace(cmd.name, index);
        by_path.emplace(cmd.path, index);
        commands.emplace_back(std::move(cmd));
        return commands.back();
    }
};

CommandRegistry::CommandRegistry() {
    commands.reserve(64);

    emplace(make_command("CIA461NetValue", DataType::S32, "601A/01"));
    emplace(make_command("CIA461GrossValue", DataType::S32, "6144/00"));
    emplace(make_command("CIA461TareValue", DataType::S32, "6143/00"));
    emplace(make_command("CIA461Decimals", DataType::U08, "6013/01"));
    emplace(make_command("CIA461Unit", DataType::U32, "6015/01", 16, 8));
    emplace(make_command("CIA461WeightStatus", DataType::U16, "6012/01"));
    emplace(make_command("CIA461WeightStatusGeneralWeightError", DataType::Bit, "6012/01", 0, 1));
    emplace(make_command("CIA461WeightStatusScaleAlarm", DataType::Bit, "6012/01", 1, 1));
    emplace(make_command("CIA461WeightStatusLimitStatus", DataType::Bit, "6012/01", 2, 2));
    emplace(make_command("CIA461WeightStatusWeightMoving", DataType::Bit, "6012/01", 4, 1));
    emplace(make_command("CIA461WeightStatusScaleSealIsOpen", DataType::Bit, "6012/01", 5, 1));
    emplace(make_command("CIA461WeightStatusManualTare", DataType::Bit, "6012/01", 6, 1));
    emplace(make_command("CIA461WeightStatusWeightType", DataType::Bit, "6012/01", 7, 1));
    emplace(make_command("CIA461WeightStatusScaleRange", DataType::Bit, "6012/01", 8, 2));
    emplace(make_command("CIA461WeightStatusZeroRequired", DataType::Bit, "6012/01", 10, 1));
    emplace(make_command("CIA461WeightStatusCenterOfZero", DataType::Bit, "6012/01", 11, 1));
    emplace(make_command("CIA461WeightStatusInsideZero", DataType::Bit, "6012/01", 12, 1));
    emplace(make_command("CIA461ScaleCommand", DataType::U32, "6002/01"));
    emplace(make_command("CIA461ScaleCommandStatus", DataType::U32, "6002/02"));
    emplace(make_command("CIA461ScaleMaximumCapacity", DataType::S32, "6113/01"));
    emplace(make_command("CIA461MultiIntervalRangeControl", DataType::U08, "611C/01"));
    emplace(make_command("CIA461MultiLimit1", DataType::S32, "611C/02"));
    emplace(make_command("CIA461MultiLimit2", DataType::S32, "611C/03"));
    emplace(make_command("CIA461WeightStep", DataType::U08, "6016/01"));
    emplace(make_command("CIA461ZeroValue", DataType::S32, "6142/00"));
    emplace(make_command("CIA461CalibrationWeight", DataType::S32, "6152/00"));
    emplace(make_command("CIA461SaveAllParameters", DataType::U32, "1010/01"));
    emplace(make_command("DSERestoreAllDefaultParameters", DataType::U32, "1011/03"));
    emplace(make_command("DSESerialNumber", DataType::U32, "4280/04"));
    emplace(make_command("DSEIdentification", DataType::Ascii, "1008/00"));
    emplace(make_command("DSEFirmwareVersion", DataType::Ascii, "100A/00"));
    emplace(make_command("DSEZeroSignal", DataType::S32, "6150/00"));
    emplace(make_command("DSENominalSignal", DataType::S32, "6151/00"));
    emplace(make_command("LDWZeroValue", DataType::S32, "2110/06"));
    emplace(make_command("LWTNominalValue", DataType::S32, "2110/07"));
    emplace(make_command("IMDApplicationMode", DataType::U08, "2010/07"));
    emplace(make_command("STORecordWeight", DataType::U08, "2040/05"));
}

CommandRegistry& registry() {
    static CommandRegistry instance;
    return instance;
}

} // namespace

int Command::to_int(const std::string& raw) const {
    try {
        if (type == DataType::Bit) {
            const int value = std::stoi(raw);
            return extract_bit(value, bit_index, bit_length);
        }
        return std::stoi(raw);
    } catch (...) {
        return 0;
    }
}

std::string Command::to_string(const std::string& raw) const {
    if (type == DataType::Bit) {
        return std::to_string(to_int(raw));
    }
    return raw;
}

namespace commands {

const Command& lookup(const std::string& name) {
    auto& reg = registry();
    auto it = reg.by_name.find(name);
    if (it == reg.by_name.end()) {
        throw JetBusError("Unknown command: " + name);
    }
    return reg.commands.at(it->second);
}

const Command& cia461_net_value() { return lookup("CIA461NetValue"); }
const Command& cia461_gross_value() { return lookup("CIA461GrossValue"); }
const Command& cia461_tare_value() { return lookup("CIA461TareValue"); }
const Command& cia461_decimals() { return lookup("CIA461Decimals"); }
const Command& cia461_unit() { return lookup("CIA461Unit"); }
const Command& cia461_weight_status() { return lookup("CIA461WeightStatus"); }
const Command& cia461_weight_status_general_weight_error() { return lookup("CIA461WeightStatusGeneralWeightError"); }
const Command& cia461_weight_status_scale_alarm() { return lookup("CIA461WeightStatusScaleAlarm"); }
const Command& cia461_weight_status_weight_moving() { return lookup("CIA461WeightStatusWeightMoving"); }
const Command& cia461_weight_status_scale_seal_is_open() { return lookup("CIA461WeightStatusScaleSealIsOpen"); }
const Command& cia461_weight_status_scale_range() { return lookup("CIA461WeightStatusScaleRange"); }
const Command& cia461_weight_status_manual_tare() { return lookup("CIA461WeightStatusManualTare"); }
const Command& cia461_weight_status_weight_type() { return lookup("CIA461WeightStatusWeightType"); }
const Command& cia461_weight_status_zero_required() { return lookup("CIA461WeightStatusZeroRequired"); }
const Command& cia461_weight_status_center_of_zero() { return lookup("CIA461WeightStatusCenterOfZero"); }
const Command& cia461_weight_status_inside_zero() { return lookup("CIA461WeightStatusInsideZero"); }
const Command& cia461_weight_status_limit_status() { return lookup("CIA461WeightStatusLimitStatus"); }
const Command& cia461_scale_command() { return lookup("CIA461ScaleCommand"); }
const Command& cia461_scale_command_status() { return lookup("CIA461ScaleCommandStatus"); }
const Command& cia461_scale_maximum_capacity() { return lookup("CIA461ScaleMaximumCapacity"); }
const Command& cia461_multi_interval_range_control() { return lookup("CIA461MultiIntervalRangeControl"); }
const Command& cia461_multi_limit1() { return lookup("CIA461MultiLimit1"); }
const Command& cia461_multi_limit2() { return lookup("CIA461MultiLimit2"); }
const Command& cia461_weight_step() { return lookup("CIA461WeightStep"); }
const Command& cia461_zero_value() { return lookup("CIA461ZeroValue"); }
const Command& cia461_calibration_weight() { return lookup("CIA461CalibrationWeight"); }
const Command& cia461_save_all_parameters() { return lookup("CIA461SaveAllParameters"); }
const Command& dse_restore_defaults() { return lookup("DSERestoreAllDefaultParameters"); }
const Command& dse_serial_number() { return lookup("DSESerialNumber"); }
const Command& dse_identification() { return lookup("DSEIdentification"); }
const Command& dse_firmware_version() { return lookup("DSEFirmwareVersion"); }
const Command& dse_zero_signal() { return lookup("DSEZeroSignal"); }
const Command& dse_nominal_signal() { return lookup("DSENominalSignal"); }
const Command& ldw_zero_value() { return lookup("LDWZeroValue"); }
const Command& lwt_nominal_value() { return lookup("LWTNominalValue"); }
const Command& sto_record_weight() { return lookup("STORecordWeight"); }
const Command& imd_application_mode() { return lookup("IMDApplicationMode"); }
const Command& cia461_scale_command_status_raw() { return lookup("CIA461ScaleCommandStatus"); }

const std::vector<Command>& all() { return registry().commands; }

const Command* find_by_path(std::string_view path) {
    auto& reg = registry();
    auto range = reg.by_path.equal_range(std::string(path));
    if (range.first == range.second) {
        return nullptr;
    }
    return &reg.commands[range.first->second];
}

} // namespace commands

} // namespace jetbus
