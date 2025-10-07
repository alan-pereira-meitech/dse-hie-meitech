#include "hbm/automation/api/weighing/wtx/jet/jet_bus_command.hpp"

#include <cstdlib>

namespace hbm::automation::api::weighing::wtx::jet {

std::string JetBusCommand::to_string(const std::string& input) const
{
    if (data_type_ == DataType::BIT) {
        return std::to_string(extract_bit(std::stoi(input)));
    }
    return input;
}

int JetBusCommand::to_int(const std::string& input) const
{
    if (input.empty()) {
        return 0;
    }

    try {
        switch (data_type_) {
        case DataType::BIT:
            return extract_bit(std::stoi(input));
        case DataType::S08:
        case DataType::S16:
        case DataType::S32:
            return std::stoi(input);
        case DataType::U08:
        case DataType::U16:
        case DataType::U32:
            return static_cast<int>(std::stoul(input));
        case DataType::ASCII:
        case DataType::NIL:
            return 0;
        default:
            return std::stoi(input);
        }
    } catch (...) {
        return 0;
    }
}

std::string JetBusCommand::format_value(int value) const
{
    switch (data_type_) {
    case DataType::ASCII:
        return std::string{static_cast<char>(value)};
    default:
        return std::to_string(value);
    }
}

int JetBusCommand::extract_bit(int value) const
{
    int mask = 0;
    switch (bit_length_) {
    case 0:
        mask = 0xFFFF;
        break;
    case 1:
        mask = 0x1;
        break;
    case 2:
        mask = 0x3;
        break;
    case 3:
        mask = 0x7;
        break;
    default:
        mask = 0x1;
        break;
    }

    mask <<= bit_index_;
    return (value & mask) >> bit_index_;
}

} // namespace hbm::automation::api::weighing::wtx::jet
