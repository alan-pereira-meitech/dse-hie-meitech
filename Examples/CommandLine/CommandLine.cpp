#include "pch.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Globalization;
using namespace System::Threading;
using namespace Hbm::Automation::Api;
using namespace Hbm::Automation::Api::Data;
using namespace Hbm::Automation::Api::Weighing;
using namespace Hbm::Automation::Api::Weighing::WTX;
using namespace Hbm::Automation::Api::Weighing::WTX::Jet;
using namespace Hbm::Automation::Api::Weighing::WTX::Modbus;

namespace CommandLineSample
{
    ref class ConsoleRunner
    {
    private:
        BaseWTDevice^ _device;
        ConnectionType _connectionType;
        String^ _ipAddress;
        int _timerInterval;
        CultureInfo^ _culture;
        ManualResetEventSlim^ _stopSignal;

    public:
        ConsoleRunner(array<String^>^ args)
        {
            _connectionType = ConnectionType::Jetbus;
            _ipAddress = "192.168.100.88";
            _timerInterval = 200;
            _culture = CultureInfo::InvariantCulture;
            _stopSignal = gcnew ManualResetEventSlim(false);
            ParseArguments(args);
        }

        void Run()
        {
            while (!TryConnect())
            {
                Console::Write("Connection failed. Enter a different IP address or press ENTER to retry [{0}]: ", _ipAddress);
                String^ input = Console::ReadLine();
                if (!String::IsNullOrWhiteSpace(input))
                {
                    _ipAddress = input->Trim();
                }
            }

            PrintBanner();
            CommandLoop();
            Shutdown();
        }

    private:
        void ParseArguments(array<String^>^ args)
        {
            if (args == nullptr)
            {
                return;
            }

            if (args->Length > 0)
            {
                String^ mode = args[0]->ToLowerInvariant();
                if (mode->Contains("modbus"))
                {
                    _connectionType = ConnectionType::Modbus;
                }
                else if (mode->Contains("jet"))
                {
                    _connectionType = ConnectionType::Jetbus;
                }
            }

            if (args->Length > 1 && !String::IsNullOrWhiteSpace(args[1]))
            {
                _ipAddress = args[1]->Trim();
            }

            if (args->Length > 2)
            {
                int interval;
                if (Int32::TryParse(args[2], interval))
                {
                    _timerInterval = Math::Max(50, Math::Min(interval, 5000));
                }
            }
        }

        void PrintBanner()
        {
            Console::WriteLine();
            Console::WriteLine("HBM Automation API command line sample (C++/CLI)");
            Console::WriteLine("Connected to {0} via {1}", _ipAddress, _connectionType.ToString());
            Console::WriteLine();
            Console::WriteLine("Commands:");
            Console::WriteLine("  help           - Show this help");
            Console::WriteLine("  status         - Dump last process values");
            Console::WriteLine("  tare           - Send tare command");
            Console::WriteLine("  zero           - Send zero command");
            Console::WriteLine("  gross          - Switch to gross weight");
            Console::WriteLine("  net            - Switch to net weight");
            Console::WriteLine("  handshake      - Display JetBus handshake flag");
            Console::WriteLine("  settimer <ms>  - Update the process data polling interval");
            Console::WriteLine("  reconnect      - Reconnect to the device");
            Console::WriteLine("  exit           - Close the application");
            Console::WriteLine();
        }

        bool TryConnect()
        {
            Console::WriteLine("Connecting to {0} via {1}...", _ipAddress, _connectionType.ToString());

            try
            {
                _device = CreateDevice();
                _device->Connect(5000);
            }
            catch (Exception^ ex)
            {
                Console::WriteLine("Connection failed: {0}", ex->Message);
                return false;
            }

            if (_device == nullptr || !_device->Connection->IsConnected)
            {
                Console::WriteLine("Handshake not acknowledged by device.");
                return false;
            }

            Console::WriteLine("Connection established. Waiting for process data...\n");
            return true;
        }

        BaseWTDevice^ CreateDevice()
        {
            if (_connectionType == ConnectionType::Modbus)
            {
                ModbusTCPConnection^ connection = gcnew ModbusTCPConnection(_ipAddress);
                return gcnew WTXModbus(connection, _timerInterval, gcnew ProcessDataReceivedEventHandler(this, &ConsoleRunner::OnProcessDataReceived));
            }

            JetBusConnection^ jetConnection = gcnew JetBusConnection(_ipAddress, "Administrator", "wtx");
            return gcnew WTXJet(jetConnection, _timerInterval, gcnew ProcessDataReceivedEventHandler(this, &ConsoleRunner::OnProcessDataReceived));
        }

        void CommandLoop()
        {
            while (!_stopSignal->IsSet)
            {
                Console::Write("command> ");
                String^ command = Console::ReadLine();

                if (command == nullptr)
                {
                    continue;
                }

                command = command->Trim();
                if (command->Length == 0)
                {
                    continue;
                }

                array<String^>^ tokens = command->Split(gcnew array<wchar_t>{' '}, StringSplitOptions::RemoveEmptyEntries);
                String^ verb = tokens[0]->ToLowerInvariant();

                if (verb == "exit" || verb == "quit")
                {
                    _stopSignal->Set();
                }
                else if (verb == "help")
                {
                    PrintBanner();
                }
                else if (verb == "status")
                {
                    DumpStatus();
                }
                else if (verb == "tare")
                {
                    SafeExecute(gcnew Action(this, &ConsoleRunner::SendTare));
                }
                else if (verb == "zero")
                {
                    SafeExecute(gcnew Action(this, &ConsoleRunner::SendZero));
                }
                else if (verb == "gross")
                {
                    SafeExecute(gcnew Action(this, &ConsoleRunner::SetGross));
                }
                else if (verb == "net")
                {
                    SafeExecute(gcnew Action(this, &ConsoleRunner::SetNet));
                }
                else if (verb == "handshake")
                {
                    ReportHandshake();
                }
                else if (verb == "reconnect")
                {
                    Reconnect();
                }
                else if (verb == "settimer" && tokens->Length > 1)
                {
                    UpdateTimer(tokens[1]);
                }
                else
                {
                    Console::WriteLine("Unknown command '{0}'. Type 'help' to list available commands.", verb);
                }
            }
        }

        void SafeExecute(Action^ action)
        {
            if (_device == nullptr)
            {
                Console::WriteLine("Device is not connected.");
                return;
            }

            try
            {
                action();
            }
            catch (Exception^ ex)
            {
                Console::WriteLine("Device command failed: {0}", ex->Message);
            }
        }

        void SendTare()
        {
            _device->Tare();
            Console::WriteLine("Tare command sent.");
        }

        void SendZero()
        {
            _device->Zero();
            Console::WriteLine("Zero command sent.");
        }

        void SetGross()
        {
            _device->SetGross();
            Console::WriteLine("Gross weight mode requested.");
        }

        void SetNet()
        {
            _device->SetNet();
            Console::WriteLine("Net weight mode requested.");
        }

        void ReportHandshake()
        {
            if (_device == nullptr)
            {
                Console::WriteLine("Handshake unavailable: device disconnected.");
                return;
            }

            bool flag = _device->ProcessData != nullptr && _device->ProcessData->Handshake;
            Console::WriteLine("Handshake flag: {0}", flag ? "SET" : "CLEARED");
        }

        void UpdateTimer(String^ value)
        {
            int interval;
            if (!Int32::TryParse(value, interval))
            {
                Console::WriteLine("Invalid interval '{0}'.", value);
                return;
            }

            interval = Math::Max(50, Math::Min(interval, 5000));
            _timerInterval = interval;

            if (_device != nullptr)
            {
                _device->Stop();
                _device->TimerInterval = interval;
                _device->Start();
            }

            Console::WriteLine("Process data polling interval set to {0} ms.", interval);
        }

        void Reconnect()
        {
            Shutdown();
            Thread::Sleep(500);
            TryConnect();
        }

        void DumpStatus()
        {
            if (_device == nullptr || _device->ProcessData == nullptr)
            {
                Console::WriteLine("No process data available yet.");
                return;
            }

            ProcessData^ data = _device->ProcessData;
            Console::WriteLine("Timestamp       : {0}", DateTime::Now.ToString("u", _culture));
            Console::WriteLine("Net weight      : {0:F3} {1}", data->Weight->Net, _device->Unit);
            Console::WriteLine("Gross weight    : {0:F3} {1}", data->Weight->Gross, _device->Unit);
            Console::WriteLine("Inside zero     : {0}", data->InsideZero);
            Console::WriteLine("Center of zero  : {0}", data->CenterOfZero);
            Console::WriteLine("Overload        : {0}", data->Overload);
            Console::WriteLine("Underload       : {0}", data->Underload);
            Console::WriteLine("Weight stable   : {0}", data->WeightStable);
            Console::WriteLine("Handshake       : {0}", data->Handshake);
        }

        void OnProcessDataReceived(Object^ sender, ProcessDataReceivedEventArgs^ e)
        {
            if (e == nullptr || e->ProcessData == nullptr)
            {
                return;
            }

            ProcessData^ data = e->ProcessData;
            Console::WriteLine("[{0}] Net={1:F3} {2}, Gross={3:F3} {2}, Handshake={4}",
                DateTime::Now.ToString("HH:mm:ss", _culture),
                data->Weight->Net,
                _device->Unit,
                data->Weight->Gross,
                data->Handshake ? "1" : "0");
        }

        void Shutdown()
        {
            if (_device != nullptr)
            {
                try
                {
                    Console::WriteLine("Disconnecting from device...");
                    _device->Disconnect();
                }
                catch (Exception^ ex)
                {
                    Console::WriteLine("Failed to disconnect cleanly: {0}", ex->Message);
                }
                finally
                {
                    _device = nullptr;
                }
            }
        }
    };
}

int main(array<System::String ^> ^args)
{
    try
    {
        CommandLineSample::ConsoleRunner runner(args);
        runner.Run();
        return 0;
    }
    catch (Exception^ ex)
    {
        Console::Error->WriteLine("Unhandled exception: {0}", ex->Message);
        return 1;
    }
}
