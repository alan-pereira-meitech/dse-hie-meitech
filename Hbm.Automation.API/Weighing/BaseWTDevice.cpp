#include "pch.h"
#include "BaseWTDevice.h"

using namespace System;
using namespace System::Threading;
using namespace Hbm::Automation::Api;
using namespace Hbm::Automation::Api::Data;
using namespace Hbm::Automation::Api::Weighing;

BaseWTDevice::BaseWTDevice(INetConnection^ connection, int timerIntervalMs)
    : _connection(connection), _processData(nullptr), _timerInterval(timerIntervalMs), _unit("kg")
{
    if (connection == nullptr)
    {
        throw gcnew ArgumentNullException("connection");
    }

    _timer = gcnew Timer(gcnew TimerCallback(this, &BaseWTDevice::UpdateProcessData), nullptr, Timeout::Infinite, Timeout::Infinite);
}

BaseWTDevice::BaseWTDevice(INetConnection^ connection)
    : BaseWTDevice(connection, 500)
{
}

BaseWTDevice::~BaseWTDevice()
{
    delete _timer;
    _timer = nullptr;
}

INetConnection^ BaseWTDevice::Connection::get()
{
    return _connection;
}

IProcessData^ BaseWTDevice::ProcessData::get()
{
    return _processData;
}

int BaseWTDevice::TimerInterval::get()
{
    return _timerInterval;
}

void BaseWTDevice::TimerInterval::set(int value)
{
    _timerInterval = Math::Max(50, value);
    RestartTimer();
}

void BaseWTDevice::Connect(int timeoutMs)
{
    _connection->Connect(timeoutMs);
    Start();
}

void BaseWTDevice::Disconnect()
{
    Stop();
    _connection->Disconnect();
}

void BaseWTDevice::Start()
{
    if (_timer != nullptr)
    {
        _timer->Change(0, _timerInterval);
    }
}

void BaseWTDevice::Stop()
{
    if (_timer != nullptr)
    {
        _timer->Change(Timeout::Infinite, Timeout::Infinite);
    }
}

String^ BaseWTDevice::Unit::get()
{
    return _unit;
}

void BaseWTDevice::Unit::set(String^ value)
{
    _unit = value;
}

WeightType BaseWTDevice::Weight::get()
{
    return _processData == nullptr ? WeightType(0, 0, 0) : _processData->Weight;
}

PrintableWeightType BaseWTDevice::PrintableWeight::get()
{
    return _processData == nullptr ? PrintableWeightType(0, 0, 0, 0) : _processData->PrintableWeight;
}

bool BaseWTDevice::IsConnected::get()
{
    return _connection->IsConnected;
}

ConnectionType BaseWTDevice::ConnectionType::get()
{
    return _connection->ConnectionType;
}

IProcessData^ BaseWTDevice::CurrentProcessData::get()
{
    return _processData;
}

void BaseWTDevice::RaiseProcessData()
{
    if (ProcessDataReceived != nullptr && _processData != nullptr)
    {
        ProcessDataReceived(this, gcnew ProcessDataReceivedEventArgs(_processData));
    }
}

void BaseWTDevice::UpdateProcessData(Object^ state)
{
    if (_processData == nullptr || !_connection->IsConnected)
    {
        return;
    }

    _processData->UpdateData(this, EventArgs::Empty);
    RaiseProcessData();
}

void BaseWTDevice::RestartTimer()
{
    Stop();
    Start();
}
