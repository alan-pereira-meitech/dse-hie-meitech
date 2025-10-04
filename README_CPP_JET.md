# DSE Jet C++ SDK

This SDK provides a native C++17 client for the Hottinger Baldwin Messtechnik (HBM) DSE Jet/JetBus interface. It uses Boost.Beast for WebSocket transport, nlohmann/json for payloads and OpenSSL for TLS. The layout mirrors the original .NET reference implementation while exposing a clean CMake build.

## Requirements

Tested on Ubuntu 22.04 with the following packages:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libboost-all-dev libssl-dev
```

CMake fetches `nlohmann_json` and GoogleTest automatically when they are not already available system-wide.

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON
cmake --build build -j
```

The build generates the following targets:

- `dsejet` – static library with the SDK implementation.
- `dsejet_examples` – custom target aggregating example binaries (`read_stream`, `read_poll`, `tare_zero`, `set_filter`).
- `dsejet_tests` – GoogleTest suite (unit + optional integration).

## Configuration

Runtime parameters can be passed via environment variables or a `.env` file (see `.env.example`). CLI flags override the defaults:

```
--host <hostname>
--port <port>
--tls yes|no
--user <jet user>
--password <jet password>
--interval-ms <polling interval>
--timeout-ms <request timeout>
--log-level <0..4>
--ping-ms <keepalive interval>
--reconnect-ms <initial backoff>
--reconnect-max-ms <max backoff>
--jet-path </jet path>
```

## Running tests

```bash
ctest --test-dir build
```

Integration tests are skipped unless `INTEGRATION_DSE=1` is set and the credentials in the environment allow access to a real device.

## Running examples

Examples require valid credentials. When testing without hardware you can run them with dummy parameters to verify startup behaviour:

```bash
./build/read_stream --host 127.0.0.1 --port 9 --tls no --timeout-ms 1000
./build/read_poll --host 127.0.0.1 --port 9 --tls no --timeout-ms 1000
./build/tare_zero --host 127.0.0.1 --port 9 --tls no --timeout-ms 1000
./build/set_filter --host 127.0.0.1 --port 9 --tls no --timeout-ms 1000 --stage 2 --type fir_comb --frequency 10
```

Each binary reads the `.env` file first, then applies CLI overrides.

## Limitations / TODOs

- The exact Jet message schema must be validated against the official Jet/JetBus specification. The current JSON frames follow the legacy .NET stubs and may require adjustments.
- TLS verification currently defaults to `verify_none`. Configure certificate validation before using in production.
- Automatic reconnection and command retries rely on backoff timers; tune `DSE_RECONNECT_*` according to the target installation.

