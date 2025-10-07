#pragma once

#include <cmath>

namespace hbm::automation::api::utils {

inline double digit_to_double(int value, int decimals)
{
    const double factor = std::pow(10.0, static_cast<double>(decimals));
    return value / factor;
}

inline int double_to_digit(double value, int decimals)
{
    const double factor = std::pow(10.0, static_cast<double>(decimals));
    return static_cast<int>(std::llround(value * factor));
}

} // namespace hbm::automation::api::utils
