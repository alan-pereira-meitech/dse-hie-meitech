#pragma once

#include "pch.h"
#include "IProcessData.h"
#include "../INetConnection.h"
#include "../Utils/MeasurementUtils.h"
#include "../Weighing/WTX/Jet/JetBusCommands.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Data
{
    public ref class JetProcessData : IProcessData
    {
    public:
        JetProcessData(INetConnection^ connection);

        virtual void UpdateData(Object^ sender, EventArgs^ e);

        property ApplicationMode ApplicationMode
        {
            virtual Hbm::Automation::Api::ApplicationMode get();
        }

        property WeightType Weight
        {
            virtual WeightType get();
        }

        property PrintableWeightType PrintableWeight
        {
            virtual PrintableWeightType get();
        }

        property String^ Unit
        {
            virtual String^ get();
        }

        property int Decimals
        {
            virtual int get();
        }

        property TareMode TareMode
        {
            virtual Hbm::Automation::Api::TareMode get();
        }

        property bool WeightStable
        {
            virtual bool get();
        }

        property bool CenterOfZero
        {
            virtual bool get();
        }

        property bool InsideZero
        {
            virtual bool get();
        }

        property bool ZeroRequired
        {
            virtual bool get();
        }

        property int ScaleRange
        {
            virtual int get();
        }

        property bool LegalForTrade
        {
            virtual bool get();
        }

        property bool Underload
        {
            virtual bool get();
        }

        property bool Overload
        {
            virtual bool get();
        }

        property bool HigherSafeLoadLimit
        {
            virtual bool get();
        }

        property bool GeneralScaleError
        {
            virtual bool get();
        }

        property bool ScaleAlarm
        {
            virtual bool get();
        }

        property bool Handshake
        {
            virtual bool get();
        }

    private:
        String^ UnitIdToString(int id);
        Hbm::Automation::Api::TareMode EvaluateTareMode(int tare, int weightType);

        INetConnection^ _connection;
        ApplicationMode _applicationMode;
        WeightType _weight;
        PrintableWeightType _printableWeight;
        String^ _unit;
        int _decimals;
        TareMode _tareMode;
        bool _weightStable;
        bool _centerOfZero;
        bool _insideZero;
        bool _zeroRequired;
        int _scaleRange;
        bool _legalForTrade;
        bool _underload;
        bool _overload;
        bool _higherSafeLoadLimit;
        bool _generalScaleError;
        bool _scaleAlarm;
        bool _handshake;
    };
}
}
}
}
