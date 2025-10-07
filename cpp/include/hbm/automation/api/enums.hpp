#pragma once

#include <cstdint>

namespace hbm::automation::api {

enum class ConnectionType : std::uint8_t {
    Modbus = 0,
    Jetbus = 1,
    DSEJet = 2
};

enum class DataType : std::uint8_t {
    NIL,
    BIT,
    S08,
    U08,
    S16,
    U16,
    S32,
    U32,
    ASCII
};

enum class ApplicationMode : std::uint8_t {
    Standard = 0,
    Checkweigher = 1,
    Filler = 2
};

enum class TareMode : std::uint8_t {
    None = 0,
    Tare = 1,
    PresetTare = 2
};

} // namespace hbm::automation::api
