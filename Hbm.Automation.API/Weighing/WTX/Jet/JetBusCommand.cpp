#include "pch.h"
#include "JetBusCommand.h"

using namespace System;
using namespace Hbm::Automation::Api;
using namespace Hbm::Automation::Api::Weighing::WTX::Jet;

String^ JetBusCommand::ToString(String^ input)
{
    try
    {
        if (DataType == DataType::BIT)
        {
            return ExtractBit(Convert::ToInt32(input)).ToString();
        }

        return input;
    }
    catch (...)
    {
        return "0";
    }
}

int JetBusCommand::ToSValue(String^ input)
{
    try
    {
        if (DataType == DataType::BIT)
        {
            return ExtractBit(Convert::ToInt32(input));
        }

        return Convert::ToInt32(input);
    }
    catch (...)
    {
        return 0;
    }
}

int JetBusCommand::ExtractBit(int value)
{
    int mask = 0;

    switch (BitLength)
    {
    case 0:
        mask = 0xFFFF;
        break;
    case 1:
        mask = 1;
        break;
    case 2:
        mask = 3;
        break;
    case 3:
        mask = 7;
        break;
    default:
        mask = 1;
        break;
    }

    mask <<= BitIndex;
    return (value & mask) >> BitIndex;
}
