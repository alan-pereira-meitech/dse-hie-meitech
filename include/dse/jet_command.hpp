#pragma once

#include <cstdint>
#include <string_view>

namespace dse {

enum class DataType {
    kAscii,
    kS32,
    kU16,
    kU32,
};

struct JetCommand {
    DataType type{};
    std::string_view path{};

    constexpr JetCommand() = default;
    constexpr JetCommand(DataType command_type, std::string_view command_path) noexcept
        : type(command_type), path(command_path) {}
};

}  // namespace dse
