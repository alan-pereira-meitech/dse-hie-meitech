# DSE JetBus C++ Client

Modern C++17 client for HBM DSE devices over JetBus (WebSocket) with a single, simple streaming example and unit tests. Optionally, a Modbus-only sample is provided via a simple Makefile.

## Build (CMake)

Prerequisites:
- C++17 compiler (g++/clang++)
- CMake >= 3.16
- Internet access on first configure (to fetch Catch2 and nlohmann/json; Boost headers are auto-downloaded if not found)

Steps:
1) Configure a build directory in Release mode
2) Build targets (libraries, example, and tests)

Artifacts are placed in the build directory (e.g., `build-cmake/`).

## Run the streaming example

Primary example (1-second loop): `dse_stream_1s`

Quick start with helper script (zsh):

```bash
chmod +x examples/run_stream.sh
examples/run_stream.sh 192.168.1.100
```

Or run the binary directly (IP or full URL):

```bash
build-cmake/dse_stream_1s 192.168.1.100
# or
build-cmake/dse_stream_1s ws://192.168.1.100/jet/canopen
```

What it does:
- Connects to the device over WebSocket (subprotocol `jet`)
- Prints Net and Gross once per second with the proper unit/decimals
- Runs indefinitely until you press Ctrl+C

Notes:
- The WebSocket handshake is tuned to match embedded server expectations:
  - `Sec-WebSocket-Protocol: jet`
  - `Sec-WebSocket-Version: 13`
  - `Connection: Upgrade` and `Upgrade: websocket`
  - No `Origin`, `User-Agent`, or extensions; permessage-deflate disabled
- Default scheme/port is `ws`/80 unless you provide `wss` explicitly

## Tests

Catch2-based unit tests are included. After building, run the test binary from your build directory. All tests should pass.

## Como executar (exemplo de streaming)

Exemplo principal (loop a cada 1 segundo): dse_stream_1s

Com script (zsh):

```bash
chmod +x examples/run_stream.sh
examples/run_stream.sh 192.168.1.100
```

Ou rode o binário diretamente (IP ou URL completa):

```bash
build-cmake/dse_stream_1s 192.168.1.100
# ou
build-cmake/dse_stream_1s ws://192.168.1.100/jet/canopen
```

O programa conecta no dispositivo e imprime Net e Gross uma vez por segundo, até você pressionar Ctrl+C.

## Como testar

Após compilar o projeto com CMake, execute:

```bash
build-cmake/jetbus_tests
```

Todos os testes devem passar.

## Operações disponíveis (Leitura e Escrita)

A API de alto nível está na classe `dse::Device` (veja `include/dse/device.hpp`).

Leituras (getters):
- Pesos e formato:
  - `double net_weight()` — Peso líquido
  - `double gross_weight()` — Peso bruto
  - `double tare_weight()` — Tara
  - `std::string unit()e (ex.: kg)
  - `int decimals()` — Casas decima` — Unidadis
- Estados/flags da balança:
  - `dse::TareMode tare_mode()`
  - `bool weight_stable()`
  - `bool zero_required()`
  - `bool center_of_zero()`
  - `bool inside_zero()`
  - `bool legal_for_trade()`
  - `bool underload()`
  - `bool overload()`
  - `bool higher_safe_load_limit()`
  - `bool general_scale_error()`
  - `bool scale_alarm()`
- Parâmetros/identificação:
  - `std::int32_t weight_step()`
  - `std::int32_t scale_range()`
  - `std::int32_t maximum_capacity()`
  - `std::int32_t zero_value()`
  - `std::int32_t zero_signal()`
  - `std::int32_t nominal_signal()`
  - `std::string identification()`
  - `std::string firmware_version()`
  - `std::uint32_t serial_number()`

Escritas (comandos e parâmetros):
- Comandos de operação:
  - `void zero()` — Zerar
  - `void tare()` — Fazer tara
  - `void set_gross()` — Voltar a bruto
  - `void record_weight()` — Registrar peso
- Parâmetros:
  - `void set_unit(const std::string& unit_code)` — Ajustar unidade
  - `void set_manual_tare(double value)` — Definir tara manual
  - `void set_maximum_capacity(std::int32_t value)` — Capacidade máxima
  - `void set_zero_signal(std::int32_t value)` — Sinal de zero
  - `void set_nominal_signal(std::int32_t value)` — Sinal nominal
- Persistência:
  - `void save_all_parameters()` — Salvar parâmetros
  - `void restore_default_parameters()` — Restaurar parâmetros padrão
- Ajustes/Calibração:
  - `bool adjust_zero_signal()`
  - `bool adjust_nominal_signal()`
  - `bool adjust_nominal_signal_with_calibration_weight(double weight)`
  - `void calculate_adjustment(double scale_zero_mvv, double capacity_mvv)`

Para stream de dados, você pode usar `set_process_data_callback(...)` para receber atualizações contínuas (Net/Gross/Tare, unidade, flags). O exemplo `dse_stream_1s` mostra leitura periódica com getters.

<!-- Modbus-only Makefile sample removed; this repository now focuses on the JetBus/WebSocket client. -->

## Troubleshooting

- Connection refused or timeouts:
  - Verify the device IP and that port 80 (or your custom port) is reachable
  - Ensure you are using `ws://` (or `wss://` if TLS is required) and the path `/jet/canopen`
  - Firewalls or NAT may block the TCP connection
- Handshake differences:
  - Some embedded servers are sensitive to headers; this client mirrors a known-good handshake
  - If you customize headers, keep the `Sec-WebSocket-Protocol: jet` and version 13
- No process data displayed:
  - The client explicitly fetches initial process data (e.g., `601A/01`) and waits briefly; allow a few seconds after connect

## Project layout

- `include/` Public headers for JetBus and DSE abstractions
- `src/` Implementations (JetBus client, commands, process data, device wrapper)
- `examples/`
  - `stream_every_second.cpp` – 1-second streaming example (dse_stream_1s)
  - `run_stream.sh` – convenience runner (optional)
- `tests/` Minimal unit tests for measurement utils and process data parsing

## License

No license file included.
