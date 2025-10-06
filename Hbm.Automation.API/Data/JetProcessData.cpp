#include "pch.h"
#include "JetProcessData.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace Hbm::Automation::Api;
using namespace Hbm::Automation::Api::Data;
using namespace Hbm::Automation::Api::Utils;
using namespace Hbm::Automation::Api::Weighing::WTX::Jet;

JetProcessData::JetProcessData(INetConnection^ connection)
    : _connection(connection),
      _weight(0.0, 0.0, 0.0),
      _printableWeight(0.0, 0.0, 0.0, 0)
{
    if (connection == nullptr)
    {
        throw gcnew ArgumentNullException("connection");
    }

    _applicationMode = ApplicationMode::Unknown;
    _unit = String::Empty;
    _decimals = 0;
    _tareMode = TareMode::None;
    _weightStable = false;
    _centerOfZero = false;
    _insideZero = false;
    _zeroRequired = false;
    _scaleRange = 0;
    _legalForTrade = false;
    _underload = false;
    _overload = false;
    _higherSafeLoadLimit = false;
    _generalScaleError = false;
    _scaleAlarm = false;
    _handshake = false;

    _connection->UpdateData += gcnew EventHandler(this, &JetProcessData::UpdateData);
}

void JetProcessData::UpdateData(Object^ sender, EventArgs^ e)
{
    try
    {
        _applicationMode = static_cast<ApplicationMode>(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::IMDApplicationMode)));
        _generalScaleError = Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusGeneralWeightError)));
        _scaleAlarm = Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusScaleAlarm)));

        int limitStatus = Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusLimitStatus));
        _underload = (limitStatus == 1);
        _overload = (limitStatus == 2);
        _higherSafeLoadLimit = (limitStatus == 3);

        int manualTare = Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusManualTare));
        int weightType = Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusWeightType));
        _tareMode = EvaluateTareMode(manualTare, weightType);

        _weightStable = !Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusWeightMoving)));
        _legalForTrade = !Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusScaleSealIsOpen)));
        _scaleRange = Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusScaleRange));
        _zeroRequired = Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusZeroRequired)));
        _centerOfZero = Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusCenterOfZero)));
        _insideZero = Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461WeightStatusInsideZero)));
        _decimals = Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461Decimals));
        _unit = UnitIdToString(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461Unit)));
        _handshake = Convert::ToBoolean(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::DSEHandshake)));

        double net = MeasurementUtils::DigitToDouble(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461NetValue)), _decimals);
        double gross = MeasurementUtils::DigitToDouble(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461GrossValue)), _decimals);
        double tare = MeasurementUtils::DigitToDouble(Convert::ToInt32(_connection->ReadFromBuffer(JetBusCommands::CIA461TareValue)), _decimals);

        _weight.Update(net, gross, tare);
        _printableWeight.Update(net, gross, tare, _decimals);
    }
    catch (KeyNotFoundException^)
    {
        Console::WriteLine("KeyNotFoundException while updating Jet process data");
    }
}

ApplicationMode JetProcessData::ApplicationMode::get()
{
    return _applicationMode;
}

WeightType JetProcessData::Weight::get()
{
    return _weight;
}

PrintableWeightType JetProcessData::PrintableWeight::get()
{
    return _printableWeight;
}

String^ JetProcessData::Unit::get()
{
    return _unit;
}

int JetProcessData::Decimals::get()
{
    return _decimals;
}

TareMode JetProcessData::TareMode::get()
{
    return _tareMode;
}

bool JetProcessData::WeightStable::get()
{
    return _weightStable;
}

bool JetProcessData::CenterOfZero::get()
{
    return _centerOfZero;
}

bool JetProcessData::InsideZero::get()
{
    return _insideZero;
}

bool JetProcessData::ZeroRequired::get()
{
    return _zeroRequired;
}

int JetProcessData::ScaleRange::get()
{
    return _scaleRange;
}

bool JetProcessData::LegalForTrade::get()
{
    return _legalForTrade;
}

bool JetProcessData::Underload::get()
{
    return _underload;
}

bool JetProcessData::Overload::get()
{
    return _overload;
}

bool JetProcessData::HigherSafeLoadLimit::get()
{
    return _higherSafeLoadLimit;
}

bool JetProcessData::GeneralScaleError::get()
{
    return _generalScaleError;
}

bool JetProcessData::ScaleAlarm::get()
{
    return _scaleAlarm;
}

bool JetProcessData::Handshake::get()
{
    return _handshake;
}

String^ JetProcessData::UnitIdToString(int id)
{
    switch (id)
    {
    case 0x00020000:
        return "kg";
    case 0x004B0000:
        return "g";
    case 0x004C0000:
        return "t";
    case 0x00A60000:
        return "lb";
    case 0x00210000:
        return "N";
    default:
        return String::Empty;
    }
}

TareMode JetProcessData::EvaluateTareMode(int tare, int weightType)
{
    if (tare > 0)
    {
        if (weightType > 0)
        {
            return TareMode::PresetTare;
        }

        return TareMode::Tare;
    }

    return TareMode::None;
}
