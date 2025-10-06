#pragma once

#include "pch.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
    public ref class LogEventArgs : EventArgs
    {
    public:
        LogEventArgs(String^ level, String^ message)
        {
            Level = level;
            Message = message;
            Timestamp = DateTime::UtcNow;
        }

        property String^ Level;
        property String^ Message;
        property DateTime Timestamp;
    };
}
}
}
