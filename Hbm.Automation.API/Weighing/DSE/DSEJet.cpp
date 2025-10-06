#include "pch.h"
#include "DSEJet.h"

using namespace System;
using namespace Hbm::Automation::Api;
using namespace Hbm::Automation::Api::Data;
using namespace Hbm::Automation::Api::Utils;
using namespace Hbm::Automation::Api::Weighing::DSE;
using namespace Hbm::Automation::Api::Weighing::WTX::Jet;

namespace
{
    const int SCALE_COMMAND_TARE = 1701994868;
    const int SCALE_COMMAND_ZERO = 1869768058;
    const int SCALE_COMMAND_SET_GROSS = 1936683623;
    const int SCALE_COMMAND_SET_NET = 1852139364;
}

DSEJet::DSEJet(INetConnection^ connection, int timerIntervalMs, EventHandler<ProcessDataReceivedEventArgs^>^ handler)
    : BaseWTDevice(connection, timerIntervalMs)
{
    InitializeProcessData(handler);
    _identification = "DSE Jet";
    _firmwareVersion = "1.0";
    _serialNumber = "000000";
}

void DSEJet::InitializeProcessData(EventHandler<ProcessDataReceivedEventArgs^>^ handler)
{
    _processData = gcnew JetProcessData(_connection);
    if (handler != nullptr)
    {
        ProcessDataReceived += handler;
    }
}

String^ DSEJet::Identification::get()
{
    return _identification;
}

void DSEJet::Identification::set(String^ value)
{
    _identification = value;
}

String^ DSEJet::SerialNumber::get()
{
    return _serialNumber;
}

String^ DSEJet::FirmwareVersion::get()
{
    return _firmwareVersion;
}

ApplicationMode DSEJet::ApplicationMode::get()
{
    return _processData == nullptr ? ApplicationMode::Unknown : _processData->ApplicationMode;
}

void DSEJet::ApplicationMode::set(ApplicationMode value)
{
    // read-only in this simplified port
}

bool DSEJet::GeneralScaleError::get()
{
    return _processData != nullptr && _processData->GeneralScaleError;
}

TareMode DSEJet::TareMode::get()
{
    return _processData == nullptr ? TareMode::None : _processData->TareMode;
}

bool DSEJet::WeightStable::get()
{
    return _processData != nullptr && _processData->WeightStable;
}

int DSEJet::ScaleRange::get()
{
    return _processData == nullptr ? 0 : _processData->ScaleRange;
}

double DSEJet::ManualTareValue::get()
{
    if (_processData == nullptr)
    {
        return 0.0;
    }

    return MeasurementUtils::DigitToDouble(_connection->ReadIntegerFromBuffer(JetBusCommands::CIA461TareValue), _processData->Decimals);
}

void DSEJet::ManualTareValue::set(double value)
{
    if (_processData == nullptr)
    {
        return;
    }

    int digits = MeasurementUtils::DoubleToDigit(value, _processData->Decimals);
    _connection->WriteInteger(JetBusCommands::CIA461TareValue, digits);
}

int DSEJet::MaximumCapacity::get()
{
    return 0;
}

void DSEJet::MaximumCapacity::set(int value)
{
    // Not implemented in simplified port
}

double DSEJet::CalibrationWeight::get()
{
    return 0.0;
}

void DSEJet::CalibrationWeight::set(double value)
{
    // Not implemented in simplified port
}

double DSEJet::ZeroValue::get()
{
    return MeasurementUtils::DigitToDouble(_connection->ReadIntegerFromBuffer(JetBusCommands::CIA461TareValue), _processData != nullptr ? _processData->Decimals : 0);
}

bool DSEJet::LegalForTrade::get()
{
    return _processData != nullptr && _processData->LegalForTrade;
}

bool DSEJet::Underload::get()
{
    return _processData != nullptr && _processData->Underload;
}

bool DSEJet::Overload::get()
{
    return _processData != nullptr && _processData->Overload;
}

bool DSEJet::HigherSafeLoadLimit::get()
{
    return _processData != nullptr && _processData->HigherSafeLoadLimit;
}

bool DSEJet::ZeroRequired::get()
{
    return _processData != nullptr && _processData->ZeroRequired;
}

bool DSEJet::CenterOfZero::get()
{
    return _processData != nullptr && _processData->CenterOfZero;
}

bool DSEJet::InsideZero::get()
{
    return _processData != nullptr && _processData->InsideZero;
}

bool DSEJet::ScaleAlarm::get()
{
    return _processData != nullptr && _processData->ScaleAlarm;
}

DigitalFilterMode DSEJet::FilterMode::get()
{
    return DigitalFilterMode::Medium;
}

void DSEJet::FilterMode::set(DigitalFilterMode value)
{
    // not implemented
}

int DSEJet::FilterTimeConstant::get()
{
    return 0;
}

void DSEJet::FilterTimeConstant::set(int value)
{
    // not implemented
}

void DSEJet::Tare()
{
    SendScaleCommand(SCALE_COMMAND_TARE);
}

void DSEJet::Zero()
{
    SendScaleCommand(SCALE_COMMAND_ZERO);
}

void DSEJet::SetGross()
{
    SendScaleCommand(SCALE_COMMAND_SET_GROSS);
}

void DSEJet::SetNet()
{
    SendScaleCommand(SCALE_COMMAND_SET_NET);
}

IProcessData^ DSEJet::ProcessData::get()
{
    return _processData;
}

void DSEJet::SendScaleCommand(int command)
{
    _connection->WriteInteger(JetBusCommands::CIA461ScaleCommand, command);
    AwaitCommandCompletion();
}

void DSEJet::AwaitCommandCompletion()
{
    DateTime start = DateTime::UtcNow;
    while ((DateTime::UtcNow - start).TotalSeconds < 2)
    {
        int status = _connection->ReadIntegerFromBuffer(JetBusCommands::CIA461ScaleCommandStatus);
        if (status != 0)
        {
            break;
        }

        System::Threading::Thread::Sleep(50);
    }
}
