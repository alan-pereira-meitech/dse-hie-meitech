#pragma once

#include "pch.h"
#include "JetBusCommand.h"

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
    public ref class JetBusCommands abstract sealed
    {
    public:
        static initonly JetBusCommand^ CIA461NetValue = gcnew JetBusCommand(DataType::S32, "6144/00", 0, 0);
        static initonly JetBusCommand^ CIA461GrossValue = gcnew JetBusCommand(DataType::S32, "6144/01", 0, 0);
        static initonly JetBusCommand^ CIA461TareValue = gcnew JetBusCommand(DataType::S32, "6143/00", 0, 0);
        static initonly JetBusCommand^ CIA461Decimals = gcnew JetBusCommand(DataType::U08, "6013/01", 0, 0);
        static initonly JetBusCommand^ CIA461Unit = gcnew JetBusCommand(DataType::U32, "6015/01", 16, 8);
        static initonly JetBusCommand^ CIA461WeightStatusGeneralWeightError = gcnew JetBusCommand(DataType::BIT, "6012/01", 0, 1);
        static initonly JetBusCommand^ CIA461WeightStatusScaleAlarm = gcnew JetBusCommand(DataType::BIT, "6012/01", 1, 1);
        static initonly JetBusCommand^ CIA461WeightStatusLimitStatus = gcnew JetBusCommand(DataType::BIT, "6012/01", 2, 2);
        static initonly JetBusCommand^ CIA461WeightStatusWeightMoving = gcnew JetBusCommand(DataType::BIT, "6012/01", 4, 1);
        static initonly JetBusCommand^ CIA461WeightStatusScaleSealIsOpen = gcnew JetBusCommand(DataType::BIT, "6012/01", 5, 1);
        static initonly JetBusCommand^ CIA461WeightStatusManualTare = gcnew JetBusCommand(DataType::BIT, "6012/01", 6, 1);
        static initonly JetBusCommand^ CIA461WeightStatusWeightType = gcnew JetBusCommand(DataType::BIT, "6012/01", 7, 1);
        static initonly JetBusCommand^ CIA461WeightStatusScaleRange = gcnew JetBusCommand(DataType::BIT, "6012/01", 8, 2);
        static initonly JetBusCommand^ CIA461WeightStatusZeroRequired = gcnew JetBusCommand(DataType::BIT, "6012/01", 10, 1);
        static initonly JetBusCommand^ CIA461WeightStatusCenterOfZero = gcnew JetBusCommand(DataType::BIT, "6012/01", 11, 1);
        static initonly JetBusCommand^ CIA461WeightStatusInsideZero = gcnew JetBusCommand(DataType::BIT, "6012/01", 12, 1);
        static initonly JetBusCommand^ IMDApplicationMode = gcnew JetBusCommand(DataType::U08, "2010/07", 0, 0);
        static initonly JetBusCommand^ DSEHandshake = gcnew JetBusCommand(DataType::BIT, "6012/01", 14, 1);
        static initonly JetBusCommand^ CIA461ScaleCommand = gcnew JetBusCommand(DataType::U32, "6002/01", 0, 0);
        static initonly JetBusCommand^ CIA461ScaleCommandStatus = gcnew JetBusCommand(DataType::U32, "6002/02", 0, 0);
    };
}
}
}
}
}
}
