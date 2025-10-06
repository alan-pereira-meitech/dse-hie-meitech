# Strategy for Converting GUIsimple DSE Jet Workflow to a C++ Console Application

## Objectives
- Provide a console-based sample that focuses exclusively on connecting to a DSE device via JetBus.
- Reproduce the existing handshake, cyclic data acquisition, and basic command operations implemented in the C# `GUIsimple` example for the DSE Jet path.
- Prepare the groundwork for a future implementation without introducing a graphical user interface.

## Dependencies and External Libraries
1. **HBM Automation API** (`Hbm.Automation.Api` project)
   - Contains `DSEJet` and `DSEJetConnection` classes that encapsulate JetBus communication logic.
   - Built for `netstandard2.0` / `.NET Framework 4.5`; can be consumed from a C++/CLI console project targeting `.NET Framework 4.6.1` or higher.
2. **SharpJet** (NuGet dependency of `Hbm.Automation.Api`)
   - Provides the JetBus protocol implementation used internally by `DSEJetConnection`.
   - Automatically satisfied when referencing `Hbm.Automation.Api` because the managed assembly exposes the JetBus wrapper types.
3. **Newtonsoft.Json**
   - Required by `DSEJetConnection` for JSON handling during JetBus transactions; comes transitively with `Hbm.Automation.Api`.
4. **.NET Framework**
   - A Managed C++ (/clr) console project targeting `.NET Framework 4.7.2` is recommended to ensure full compatibility with the netstandard2.0 assembly.

## Proposed Architecture
- Create a new C++/CLI console project (e.g., `Examples/DSEJetConsole/DSEJetConsole.vcxproj`).
- Reference the existing `Hbm.Automation.Api` project so that managed classes (`DSEJet`, `DSEJetConnection`) are accessible.
- Implement a minimal console workflow:
  1. Parse IP address from command-line arguments (default to `192.168.100.88`).
  2. Instantiate `DSEJetConnection` with the IP and create a `DSEJet` device using the same update callback signature as in C#.
  3. Perform `Connect(timeout)` and wait for the initial fetch cycle to complete, mirroring the GUI handshake sequence.
  4. Subscribe to process-data events (`UpdateData`) to print net/gross/tare weights and diagnostic information.
  5. Provide console commands for basic operations (e.g., tare, zero, toggle motion detection) using the same Jet commands invoked in `GUIsimple`.
  6. Ensure graceful shutdown by calling `Disconnect()` and disposing of managed resources when the user exits.

## Implementation Steps
1. **Project Scaffolding**
   - Add the new `vcxproj` to `Automation-API.sln` and configure it for `/clr` with the desired framework.
   - Include a `packages.config` or use MSBuild item groups to reference the output of `Hbm.Automation.Api`.

2. **Interop Layer**
   - Wrap the managed API usage inside C++/CLI classes (`ref class DseJetClient`) to keep the `main` function concise.
   - Implement delegates for the process-data callback and communication logging, translating managed events into console output.

3. **Console Workflow**
   - Implement `int main(array<System::String ^> ^args)` that builds the client, connects, starts a background task for cyclic output, and waits for user commands.
   - Mirror the C# update handler logic to report NE107 states and weight values in the console.

4. **Error Handling & Cleanup**
   - Surround connection logic with try/catch blocks for `System::Exception` to capture JetBus communication errors.
   - Use `try/finally` semantics (`try { ... } finally { client->Disconnect(); }`) to ensure the device is released.

5. **Testing Strategy**
   - Validate the build in Debug/Release configurations.
   - If hardware is available, run against a DSE Jet device to confirm connection, data streaming, and command execution.

This document provides the high-level approach required before starting the actual implementation, complying with the request to describe the strategy ahead of coding.
