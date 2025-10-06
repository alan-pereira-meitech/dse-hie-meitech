#pragma once

#include "pch.h"
#include "Enums.h"
#include "LogEventArgs.h"

namespace Hbm
{
namespace Automation
{
namespace Api
{
    public interface class INetConnection
    {
        event EventHandler<LogEventArgs^>^ CommunicationLog;
        event EventHandler^ UpdateData;

        property String^ IpAddress
        {
            String^ get();
            void set(String^ value);
        }

        property ConnectionType ConnectionType
        {
            Hbm::Automation::Api::ConnectionType get();
        }

        property bool IsConnected
        {
            bool get();
        }

        void Connect(int timeoutMs);
        void Disconnect();

        String^ Read(Object^ command);
        bool WriteInteger(Object^ command, int value);
        bool Write(Object^ command, String^ value);
        String^ ReadFromBuffer(Object^ command);
        String^ ReadFromDevice(Object^ command);
        int ReadIntegerFromBuffer(Object^ command);
    };
}
}
}
