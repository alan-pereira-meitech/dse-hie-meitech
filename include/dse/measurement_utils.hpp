#pragma once

#include <cmath>

namespace dse {

inline double digits_to_double(int value, int decimals) {
    if (decimals <= 0) {
        return static_cast<double>(value);
    }
    return static_cast<double>(value) / std::pow(10.0, static_cast<double>(decimals));
}

inline int double_to_digits(double value, int decimals) {
    if (decimals <= 0) {
        return static_cast<int>(value);
    }
    return static_cast<int>(std::round(value * std::pow(10.0, static_cast<double>(decimals))));
}

}  // namespace dse
