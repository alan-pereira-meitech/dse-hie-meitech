# DSE Jet C++ Library

This directory contains a standalone C++ implementation of the DSE Jet communication stack.  
It refactors the original .NET classes into a portable library that encapsulates the JetBus
WebSocket protocol and exposes a convenient wrapper to read and write process data.

## Building

The library uses CMake and depends on Boost.Asio/Boost.Beast as well as the header-only
*nlohmann/json* parser (vendored in `third_party`).

```bash
cmake -S cpp -B build
cmake --build build
```

You can install the library to your preferred prefix with `cmake --install build`.

## Usage

```cpp
#include <dse_jet/DseJetConnection.hpp>

int main()
{
    dse::jet::DseJetConnection connection{"192.168.0.10"};
    connection.connect();

    auto weight = connection.readFromDevice("601A/01");
    connection.write("611C/01", 1);

    connection.disconnect();
    return 0;
}
```

Use `setLogHandler` and `setUpdateHandler` to hook into diagnostic output and
subscription updates.
