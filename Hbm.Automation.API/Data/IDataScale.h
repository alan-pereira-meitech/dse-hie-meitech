#pragma once

#include "pch.h"
#include "IProcessData.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Data
{
    public interface class IDataScale
    {
        property IProcessData^ ProcessData
        {
            IProcessData^ get();
        }

        property int ScaleRange
        {
            int get();
        }

        property bool LegalForTrade
        {
            bool get();
        }

        void Reset();
        void Tare();
        void Zero();
        void SetGross();
        void SetNet();
    };
}
}
}
}
