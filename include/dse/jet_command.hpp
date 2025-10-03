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
    DataType type;
    std::string_view path;
};

}  // namespace dse
