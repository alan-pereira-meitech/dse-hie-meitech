#pragma once

#include <cstdint>
#include <string>

namespace dse {

enum class ApplicationMode {
    Standard = 0,
    Checkweigher = 1,
    Filler = 2
};

enum class TareMode {
    None,
    Tare,
    PresetTare
};

enum class ScaleRangeMode {
    None,
    MultiRange,
    MultiInterval
};

struct WeightValues {
    double net{0.0};
    double gross{0.0};
    double tare{0.0};
};

struct PrintableWeightValues {
    std::string net{"0"};
    std::string gross{"0"};
    std::string tare{"0"};
};

} // namespace dse
