#pragma once

#include <cstdint>
#include <string>

namespace dse {

enum class DataType {
    kAscii,
    kS32,
    kU16,
    kU32,
};

struct JetCommand {
    DataType type;
    std::string path;
};

}  // namespace dse
