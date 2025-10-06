#pragma once

namespace Hbm
{
namespace Automation
{
namespace Api
{
    public enum class ConnectionType
    {
        Jetbus = 0,
        Modbus = 1
    };

    public enum class ApplicationMode
    {
        WTX = 0,
        DSE = 1,
        Unknown = 2
    };

    public enum class TareMode
    {
        None = 0,
        Tare = 1,
        PresetTare = 2
    };

    public enum class ScaleCommandResult
    {
        Ongoing,
        Ok,
        ErrorE1,
        ErrorE2,
        ErrorE3
    };

    public enum class DigitalFilterMode
    {
        Low = 0,
        Medium = 1,
        High = 2
    };

    public enum class DataType
    {
        NIL = 0,
        BIT = 1,
        U08 = 2,
        S16 = 3,
        U16 = 4,
        S32 = 5,
        U32 = 6,
        ASCII = 7
    };
}
}
}
}
}
}
