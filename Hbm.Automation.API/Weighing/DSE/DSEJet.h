#pragma once

#include "pch.h"
#include "../BaseWTDevice.h"
#include "../../Data/JetProcessData.h"
#include "../WTX/Jet/JetBusConnection.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
namespace Weighing
{
namespace DSE
{
    public ref class DSEJet : BaseWTDevice, Data::IDataScale, Data::IDataDigitalFilter
    {
    public:
        DSEJet(Hbm::Automation::Api::INetConnection^ connection, int timerIntervalMs, EventHandler<Data::ProcessDataReceivedEventArgs^>^ handler);

        property virtual String^ Identification
        {
            String^ get();
            void set(String^ value);
        }

        property virtual String^ SerialNumber
        {
            String^ get();
        }

        property virtual String^ FirmwareVersion
        {
            String^ get();
        }

        property virtual Hbm::Automation::Api::ApplicationMode ApplicationMode
        {
            Hbm::Automation::Api::ApplicationMode get();
            void set(Hbm::Automation::Api::ApplicationMode value);
        }

        property virtual bool GeneralScaleError
        {
            bool get();
        }

        property virtual Hbm::Automation::Api::TareMode TareMode
        {
            Hbm::Automation::Api::TareMode get();
        }

        property virtual bool WeightStable
        {
            bool get();
        }

        property virtual int ScaleRange
        {
            int get();
        }

        property virtual double ManualTareValue
        {
            double get();
            void set(double value);
        }

        property virtual int MaximumCapacity
        {
            int get();
            void set(int value);
        }

        property virtual double CalibrationWeight
        {
            double get();
            void set(double value);
        }

        property virtual double ZeroValue
        {
            double get();
        }

        property virtual bool LegalForTrade
        {
            bool get();
        }

        property virtual bool Underload
        {
            bool get();
        }

        property virtual bool Overload
        {
            bool get();
        }

        property virtual bool HigherSafeLoadLimit
        {
            bool get();
        }

        property virtual bool ZeroRequired
        {
            bool get();
        }

        property virtual bool CenterOfZero
        {
            bool get();
        }

        property virtual bool InsideZero
        {
            bool get();
        }

        property virtual bool ScaleAlarm
        {
            bool get();
        }

        property virtual Data::DigitalFilterMode FilterMode
        {
            Data::DigitalFilterMode get();
            void set(Data::DigitalFilterMode value);
        }

        property virtual int FilterTimeConstant
        {
            int get();
            void set(int value);
        }

        virtual void Tare() override;
        virtual void Zero() override;
        virtual void SetGross() override;
        virtual void SetNet() override;

        property virtual Data::IProcessData^ ProcessData
        {
            Data::IProcessData^ get() override;
        }

    private:
        void InitializeProcessData(EventHandler<Data::ProcessDataReceivedEventArgs^>^ handler);
        void SendScaleCommand(int command);
        void AwaitCommandCompletion();

        String^ _identification;
        String^ _firmwareVersion;
        String^ _serialNumber;
    };
}
}
}
}
}
