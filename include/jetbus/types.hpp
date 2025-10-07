#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace jetbus {

enum class DataType {
    Nil,
    Bit,
    U08,
    U16,
    U32,
    S16,
    S32,
    Ascii
};

inline constexpr const char* to_string(DataType type) noexcept {
    switch (type) {
        case DataType::Nil: return "nil";
        case DataType::Bit: return "bit";
        case DataType::U08: return "u8";
        case DataType::U16: return "u16";
        case DataType::U32: return "u32";
        case DataType::S16: return "s16";
        case DataType::S32: return "s32";
        case DataType::Ascii: return "ascii";
    }
    return "unknown";
}

enum class JetEventType {
    Add,
    Fetch,
    Change,
    Remove,
    Unknown
};

inline JetEventType event_type_from_string(const std::string& value) {
    if (value == "add") {
        return JetEventType::Add;
    }
    if (value == "fetch") {
        return JetEventType::Fetch;
    }
    if (value == "change") {
        return JetEventType::Change;
    }
    if (value == "remove") {
        return JetEventType::Remove;
    }
    return JetEventType::Unknown;
}

struct JetBusError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

} // namespace jetbus
