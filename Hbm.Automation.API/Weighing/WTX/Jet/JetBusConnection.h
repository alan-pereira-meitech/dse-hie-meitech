#pragma once

#include "pch.h"
#include "../../../INetConnection.h"
#include "JetBusCommands.h"
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
    public ref class JetBusConnection : INetConnection, IDisposable
    {
    public:
        JetBusConnection(String^ ipAddress);
        JetBusConnection(String^ ipAddress, String^ user, String^ password);
        ~JetBusConnection();
        !JetBusConnection();

        virtual event EventHandler<LogEventArgs^>^ CommunicationLog;
        virtual event EventHandler^ UpdateData;

        property String^ IpAddress
        {
            virtual String^ get();
            virtual void set(String^ value);
        }

        property Hbm::Automation::Api::ConnectionType ConnectionType
        {
            virtual Hbm::Automation::Api::ConnectionType get();
        }

        property bool IsConnected
        {
            virtual bool get();
        }

        virtual void Connect(int timeoutMs);
        virtual void Disconnect();

        virtual String^ Read(Object^ command);
        virtual bool WriteInteger(Object^ command, int value);
        virtual bool Write(Object^ command, String^ value);
        virtual String^ ReadFromBuffer(Object^ command);
        virtual String^ ReadFromDevice(Object^ command);
        virtual int ReadIntegerFromBuffer(Object^ command);

        property Dictionary<String^, String^>^ AllData
        {
            Dictionary<String^, String^>^ get();
        }

    private:
        void Initialize(String^ ipAddress, String^ user, String^ password);
        void ConnectPeer(String^ user, String^ password, int timeoutMs);
        void PeerOnData(System::Object^ sender, Hbm::Devices::Jet::JetDataReceivedEventArgs^ e);
        void PeerOnDisconnected(System::Object^ sender, EventArgs^ e);
        void WaitOne(int seconds);
        void SetData(String^ path, Newtonsoft::Json::Linq::JValue^ value);

        bool _disposed;
        String^ _ipAddress;
        String^ _user;
        String^ _password;
        int _timeoutMs;
        bool _isConnected;
        Hbm::Devices::Jet::JetPeer^ _peer;
        System::Threading::AutoResetEvent^ _successEvent;
        Exception^ _localException;
        Dictionary<String^, String^>^ _allData;
    };
}
}
}
}
}
}
