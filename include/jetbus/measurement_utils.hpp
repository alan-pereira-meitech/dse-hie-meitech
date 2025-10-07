#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace jetbus {

inline double digit_to_double(std::int32_t value, int decimals) {
    const double factor = std::pow(10.0, static_cast<double>(decimals));
    return static_cast<double>(value) / factor;
}

inline std::int32_t double_to_digit(double value, int decimals) {
    const double factor = std::pow(10.0, static_cast<double>(decimals));
    return static_cast<std::int32_t>(std::llround(value * factor));
}

inline bool string_to_bool(const std::string& value) {
    return value != "0";
}

} // namespace jetbus
