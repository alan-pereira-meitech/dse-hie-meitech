#pragma once

#include "hbm/automation/api/weighing/wtx/jet/jet_bus_command.hpp"

namespace hbm::automation::api::weighing::wtx::jet {

class JetBusCommands {
public:
    static const JetBusCommand& CIA461NetValue()
    {
        static const JetBusCommand command{DataType::S32, "601A/01", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461GrossValue()
    {
        static const JetBusCommand command{DataType::S32, "6144/00", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461TareValue()
    {
        static const JetBusCommand command{DataType::S32, "6143/00", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461Decimals()
    {
        static const JetBusCommand command{DataType::U08, "6013/01", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461Unit()
    {
        static const JetBusCommand command{DataType::U32, "6015/01", 16, 8};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusGeneralWeightError()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 0, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusScaleAlarm()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 1, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusLimitStatus()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 2, 2};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusWeightMoving()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 4, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusScaleSealIsOpen()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 5, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusManualTare()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 6, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusWeightType()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 7, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusScaleRange()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 8, 2};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusZeroRequired()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 10, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusCenterOfZero()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 11, 1};
        return command;
    }

    static const JetBusCommand& CIA461WeightStatusInsideZero()
    {
        static const JetBusCommand command{DataType::BIT, "6012/01", 12, 1};
        return command;
    }

    static const JetBusCommand& IMDApplicationMode()
    {
        static const JetBusCommand command{DataType::U08, "2010/07", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461ScaleCommand()
    {
        static const JetBusCommand command{DataType::U32, "6002/01", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461ScaleCommandStatus()
    {
        static const JetBusCommand command{DataType::U32, "6002/02", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461SaveAllParameters()
    {
        static const JetBusCommand command{DataType::U32, "1010/01", 0, 0};
        return command;
    }

    static const JetBusCommand& CIA461RestoreAllDefaultParameters()
    {
        static const JetBusCommand command{DataType::U32, "1011/01", 0, 0};
        return command;
    }

};

} // namespace hbm::automation::api::weighing::wtx::jet
