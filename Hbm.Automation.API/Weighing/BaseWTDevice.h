#pragma once

#include "pch.h"
#include "../INetConnection.h"
#include "../Enums.h"
#include "../Data/IProcessData.h"
#include "../Data/ProcessDataReceivedEventArgs.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Weighing
{
    public ref class BaseWTDevice abstract
    {
    public:
        BaseWTDevice(INetConnection^ connection, int timerIntervalMs);
        BaseWTDevice(INetConnection^ connection);
        virtual ~BaseWTDevice();

        virtual event EventHandler<Data::ProcessDataReceivedEventArgs^>^ ProcessDataReceived;

        property INetConnection^ Connection
        {
            INetConnection^ get();
        }

        property Data::IProcessData^ ProcessData
        {
            Data::IProcessData^ get();
        }

        property int TimerInterval
        {
            int get();
            void set(int value);
        }

        virtual void Connect(int timeoutMs);
        virtual void Disconnect();
        virtual void Start();
        virtual void Stop();

        virtual void Tare() = 0;
        virtual void Zero() = 0;
        virtual void SetGross() = 0;
        virtual void SetNet() = 0;

        property virtual String^ Unit
        {
            String^ get();
            void set(String^ value);
        }

        property virtual Data::WeightType Weight
        {
            Data::WeightType get();
        }

        property virtual Data::PrintableWeightType PrintableWeight
        {
            Data::PrintableWeightType get();
        }

        property virtual bool IsConnected
        {
            bool get();
        }

        property virtual Hbm::Automation::Api::ConnectionType ConnectionType
        {
            Hbm::Automation::Api::ConnectionType get();
        }

        property virtual Data::IProcessData^ CurrentProcessData
        {
            Data::IProcessData^ get();
        }

    protected:
        void RaiseProcessData();
        void UpdateProcessData(Object^ state);
        void RestartTimer();

        INetConnection^ _connection;
        Data::IProcessData^ _processData;
        int _timerInterval;
        System::Threading::Timer^ _timer;
        String^ _unit;
    };
}
}
}
}
