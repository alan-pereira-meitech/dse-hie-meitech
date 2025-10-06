#pragma once

#include "pch.h"
#include "../Enums.h"
#include "WeightType.h"
#include "PrintableWeightType.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Data
{
    public interface class IProcessData
    {
        void UpdateData(Object^ sender, EventArgs^ e);

        property ApplicationMode ApplicationMode
        {
            Hbm::Automation::Api::ApplicationMode get();
        }

        property WeightType Weight
        {
            WeightType get();
        }

        property PrintableWeightType PrintableWeight
        {
            PrintableWeightType get();
        }

        property String^ Unit
        {
            String^ get();
        }

        property int Decimals
        {
            int get();
        }

        property TareMode TareMode
        {
            Hbm::Automation::Api::TareMode get();
        }

        property bool WeightStable
        {
            bool get();
        }

        property bool CenterOfZero
        {
            bool get();
        }

        property bool InsideZero
        {
            bool get();
        }

        property bool ZeroRequired
        {
            bool get();
        }

        property int ScaleRange
        {
            int get();
        }

        property bool LegalForTrade
        {
            bool get();
        }

        property bool Underload
        {
            bool get();
        }

        property bool Overload
        {
            bool get();
        }

        property bool HigherSafeLoadLimit
        {
            bool get();
        }

        property bool GeneralScaleError
        {
            bool get();
        }

        property bool ScaleAlarm
        {
            bool get();
        }

        property bool Handshake
        {
            bool get();
        }
    };
}
}
}
}
