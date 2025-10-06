#pragma once

#include "pch.h"
#include "../../Enums.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Weighing
{
namespace WTX
{
namespace Jet
{
    public ref class JetBusCommand
    {
    public:
        JetBusCommand(DataType dataType, String^ path, int bitIndex, int bitLength)
        {
            DataType = dataType;
            Path = path;
            BitIndex = bitIndex;
            BitLength = bitLength;
        }

        property DataType DataType;
        property String^ Path;
        property int BitIndex;
        property int BitLength;

        String^ ToString(String^ input);
        int ToSValue(String^ input);

    private:
        int ExtractBit(int value);
    };
}
}
}
}
}
}
