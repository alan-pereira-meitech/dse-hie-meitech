# DSE Jet C++ Library

This repository contains a standalone C++ client library for interacting with
HBM DSE Jet devices over JetBus.  The code is organized as a modern CMake
project located in the `cpp/` directory and exposes a small, focused API for
connecting, subscribing to variables, reading cached values, and writing new
values.

## Building

```bash
cmake -S cpp -B build
cmake --build build
```

## License

This project remains licensed under the MIT License.  See the
[LICENSE](LICENSE) file for the full text.
