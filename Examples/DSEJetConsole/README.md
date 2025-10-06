# DSE Jet Console Sample

This sample demonstrates how to connect to a DSE Jet weighing terminal using the managed `Hbm.Automation.Api` library from a C++/CLI console application.

## Prerequisites

- Build the solution with Visual Studio 2019 (or later) using the `Automation-API.sln` solution file.
- Ensure the `Hbm.Automation.Api` project is built for the .NET Framework target (the C++/CLI project references the managed assembly directly).

## Running the sample

1. Start the console application (`DSEJetConsole`).
2. Provide the target device IP address as the first argument (default `192.168.100.88`).
3. Optionally, provide a second argument to override the process-data polling interval (in milliseconds).

```
DSEJetConsole.exe 192.168.100.50 200
```

At startup the application performs the same JetBus handshake as the WinForms sample by waiting for the first cyclic `ProcessDataReceived` event before accepting commands.

## Available commands

- `status` — display the latest gross, net, and tare values together with stability and alarm flags.
- `info` — read identification, serial number, firmware version, scale range, and application mode.
- `gross` — switch the device display to gross weight.
- `tare` — execute a tare command.
- `zero` — execute a zero command.
- `step` — show the configured weight step.
- `capacity` — show the maximum capacity configured in the device.
- `help` — print the command list.
- `quit` — exit the application.

The application logs any communication errors returned by the API so that issues (e.g. handshake failures) are visible immediately.
