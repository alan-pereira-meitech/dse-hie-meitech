#pragma once

#include "pch.h"
#include "../Enums.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Data
{
    public interface class IDataDigitalFilter
    {
        property DigitalFilterMode FilterMode
        {
            DigitalFilterMode get();
            void set(DigitalFilterMode value);
        }

        property int FilterTimeConstant
        {
            int get();
            void set(int value);
        }
    };
}
}
}
}
