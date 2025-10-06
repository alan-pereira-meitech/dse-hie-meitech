#include "pch.h"

using namespace System;
using namespace System::Threading;
using namespace Hbm::Automation::Api::Data;
using namespace Hbm::Automation::Api::Weighing::DSE;
using namespace Hbm::Automation::Api::Weighing::DSE::Jet;

namespace
{
    literal double DEFAULT_TIMER_INTERVAL = 500.0;
}

ref class DseJetConsoleApp
{
public:
    DseJetConsoleApp(String^ ipAddress, int intervalMs)
        : _ipAddress(ipAddress),
          _interval(intervalMs),
          _sync(gcnew Object()),
          _dataReady(gcnew ManualResetEventSlim(false))
    {
    }

    int Run()
    {
        Console::WriteLine("HBM DSE Jet console sample");
        Console::WriteLine("Target IP: {0}", _ipAddress);
        Console::WriteLine("Update interval: {0} ms", _interval);

        try
        {
            InitializeDevice();
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("Failed to initialize device: {0}", ex->Message);
            return 1;
        }

        Console::WriteLine("Establishing JetBus handshake...");

        try
        {
            _device->Connect();
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("Connection failed: {0}", ex->Message);
            Cleanup();
            return 1;
        }

        if (!_dataReady->Wait(TimeSpan::FromSeconds(5)))
        {
            Console::WriteLine("Timed out waiting for process data from the device.");
            Cleanup();
            return 2;
        }

        Console::WriteLine("Handshake complete. Press 'help' to list available commands.\n");

        PrintStatus();
        PrintMenu();

        while (true)
        {
            Console::Write("\n> ");
            String^ command = Console::ReadLine();

            if (String::IsNullOrWhiteSpace(command))
            {
                continue;
            }

            command = command->Trim()->ToLowerInvariant();

            if (command == "q" || command == "quit" || command == "exit")
            {
                break;
            }

            if (command == "help" || command == "h")
            {
                PrintMenu();
                continue;
            }

            try
            {
                ExecuteCommand(command);
            }
            catch (Exception^ ex)
            {
                Console::WriteLine("Command failed: {0}", ex->Message);
            }
        }

        Console::WriteLine("Disconnecting...");
        try
        {
            _device->Disconnect();
        }
        catch (Exception^ ex)
        {
            Console::WriteLine("Disconnect failed: {0}", ex->Message);
        }

        Cleanup();
        return 0;
    }

private:
    void InitializeDevice()
    {
        _connection = gcnew DSEJetConnection(_ipAddress);
        _processHandler = gcnew EventHandler<ProcessDataReceivedEventArgs^>(this, &DseJetConsoleApp::OnProcessData);
        _device = gcnew DSEJet(_connection, _interval, _processHandler);
    }

    void Cleanup()
    {
        if (_device != nullptr)
        {
            if (_processHandler != nullptr)
            {
                _device->ProcessDataReceived -= _processHandler;
                _processHandler = nullptr;
            }

            delete _device;
            _device = nullptr;
        }

        if (_connection != nullptr)
        {
            delete _connection;
            _connection = nullptr;
        }

        if (_dataReady != nullptr)
        {
            delete _dataReady;
            _dataReady = nullptr;
        }
    }

    void ExecuteCommand(String^ command)
    {
        if (command == "status")
        {
            PrintStatus();
            return;
        }

        if (command == "info")
        {
            PrintDeviceInformation();
            return;
        }

        if (command == "gross")
        {
            _device->SetGross();
            Console::WriteLine("Requested gross weight display.");
            return;
        }

        if (command == "tare")
        {
            _device->Tare();
            Console::WriteLine("Tare command sent.");
            return;
        }

        if (command == "zero")
        {
            _device->Zero();
            Console::WriteLine("Zero command sent.");
            return;
        }

        if (command == "step")
        {
            double step = _device->WeightStep;
            Console::WriteLine("Weight step: {0} {1}", step, _unit);
            return;
        }

        if (command == "capacity")
        {
            Console::WriteLine("Maximum capacity: {0} {1}", _device->MaximumCapacity, _unit);
            return;
        }

        Console::WriteLine("Unknown command '{0}'. Type 'help' to list supported commands.", command);
    }

    void PrintMenu()
    {
        Console::WriteLine("Available commands:");
        Console::WriteLine("  status    - Show the latest weight values and scale state");
        Console::WriteLine("  info      - Read identification, serial and firmware data");
        Console::WriteLine("  gross     - Switch display mode to gross weight");
        Console::WriteLine("  tare      - Send a tare command");
        Console::WriteLine("  zero      - Send a zero command");
        Console::WriteLine("  step      - Show the configured weight step");
        Console::WriteLine("  capacity  - Show the maximum capacity");
        Console::WriteLine("  help      - Show this menu");
        Console::WriteLine("  quit      - Close the application");
    }

    void PrintStatus()
    {
        String^ net;
        String^ gross;
        String^ tare;
        String^ unit;
        bool underload;
        bool overload;
        bool safeLimit;
        bool stable;
        String^ tareMode;
        DateTime lastUpdate;

        Monitor::Enter(_sync);
        try
        {
            net = _net;
            gross = _gross;
            tare = _tare;
            unit = _unit;
            underload = _underload;
            overload = _overload;
            safeLimit = _higherSafeLoadLimit;
            stable = _stable;
            tareMode = _tareMode;
            lastUpdate = _lastUpdate;
        }
        finally
        {
            Monitor::Exit(_sync);
        }

        Console::WriteLine("Net  : {0} {1}", net, unit);
        Console::WriteLine("Gross: {0} {1}", gross, unit);
        Console::WriteLine("Tare : {0} {1}", tare, unit);
        Console::WriteLine("Stable: {0} | Tare mode: {1}", stable, tareMode);

        if (underload)
        {
            Console::WriteLine("Warning: Underload condition active.");
        }
        if (overload)
        {
            Console::WriteLine("Warning: Overload condition active.");
        }
        if (safeLimit)
        {
            Console::WriteLine("Warning: Above safe load limit.");
        }

        Console::WriteLine("Last update received at {0:u}", lastUpdate);
    }

    void PrintDeviceInformation()
    {
        Console::WriteLine("Device identification: {0}", _device->Identification);
        Console::WriteLine("Serial number        : {0}", _device->SerialNumber);
        Console::WriteLine("Firmware version     : {0}", _device->FirmwareVersion);
        Console::WriteLine("Scale range          : {0}", _device->ScaleRange);
        Console::WriteLine("Application mode     : {0}", _device->ApplicationMode);
    }

    void OnProcessData(Object^ sender, ProcessDataReceivedEventArgs^ e)
    {
        auto printable = e->ProcessData->PrintableWeight;

        Monitor::Enter(_sync);
        try
        {
            _net = printable->Net;
            _gross = printable->Gross;
            _tare = printable->Tare;
            _unit = e->ProcessData->Unit;
            _underload = e->ProcessData->Underload;
            _overload = e->ProcessData->Overload;
            _higherSafeLoadLimit = e->ProcessData->HigherSafeLoadLimit;
            _stable = e->ProcessData->WeightStable;
            _tareMode = e->ProcessData->TareMode.ToString();
            _lastUpdate = DateTime::UtcNow;
        }
        finally
        {
            Monitor::Exit(_sync);
        }

        if (_dataReady != nullptr)
        {
            _dataReady->Set();
        }
    }

private:
    String^ _ipAddress;
    int _interval;
    DSEJetConnection^ _connection;
    DSEJet^ _device;
    Object^ _sync;
    ManualResetEventSlim^ _dataReady;
    EventHandler<ProcessDataReceivedEventArgs^>^ _processHandler;

    String^ _net = "0";
    String^ _gross = "0";
    String^ _tare = "0";
    String^ _unit = "kg";
    bool _underload = false;
    bool _overload = false;
    bool _higherSafeLoadLimit = false;
    bool _stable = false;
    String^ _tareMode = "Unknown";
    DateTime _lastUpdate = DateTime::MinValue;
};

int main(array<String^>^ args)
{
    String^ ip = args->Length > 0 ? args[0] : "192.168.100.88";
    int interval = static_cast<int>(DEFAULT_TIMER_INTERVAL);

    if (args->Length > 1)
    {
        int parsedInterval;
        if (Int32::TryParse(args[1], parsedInterval) && parsedInterval > 0)
        {
            interval = parsedInterval;
        }
        else
        {
            Console::WriteLine("Warning: Invalid update interval '{0}' supplied. Falling back to {1} ms.", args[1], interval);
        }
    }

    DseJetConsoleApp^ app = gcnew DseJetConsoleApp(ip, interval);
    return app->Run();
}
