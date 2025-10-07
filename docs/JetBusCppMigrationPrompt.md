# Prompt para Migração do Núcleo Jet/JetBus para C++ com CMake

A seguir está um prompt completo e detalhado para orientar outra IA na migração do núcleo Jet/JetBus deste repositório para C++ com CMake visando Linux. O objetivo é reimplementar o cliente Jet/JetBus via WebSocket + JSON e expor uma API equivalente ao core .NET, mantendo compatibilidade funcional com o dispositivo DSE.

## Brief de contexto e objetivo
- Migrar o core de comunicação e lógica (sem UI WinForms) do projeto atual (.NET Framework 4.6.1 / .NET Standard 2.0) para uma biblioteca C++ moderna, compilada com CMake em Linux.
- Substituir:
  - Stack de WebSocket/Jet atual (SharpJet) por Boost.Beast (ou WebSocket++).
  - Processamento JSON (Newtonsoft.Json) por nlohmann/json.
- Entregar:
  - Uma lib C++ com API equivalente ao núcleo BaseWTDevice/DSEJet/INetConnection/IProcessData.
  - Um exemplo console mínimo em C++ que se conecta ao DSE (ws://ip:80/jet/canopen), lê e imprime Net/Gross/Tare, executa Zero/Tare/SetGross e exibe status.
  - Testes e artefatos de build para Linux (CMake).

## Fontes e entradas canônicas
Use o código existente para extrair requisitos.

- Formato e fluxo do protocolo Jet/JetBus sobre WebSocket:
  - Arquivo: `Hbm.Automation.Api/Weighing/DSE/Jet/DSEJetConnection.cs`
  - Endpoint: `ws://<ip>:80/jet/canopen`
  - Operações: Fetch/Unfetch/Set; eventos recebidos: `"add"`, `"fetch"`, `"change"`
  - Estrutura do payload recebido: JSON com campos `"path"`, `"event"`, `"value"`
  - Conceitos: JetPeer, Matcher.EqualsTo, FetchId, callbacks OnFetchData/OnFetch, buffer AllData
  - Sequência de conexão: ConnectPeer -> FetchSelective -> WaitOne/AutoResetEvent
- Submódulo vendorizado SharpJet: `ThirdParty/SharpJet/SharpJet`
  - Classes e contratos: JetPeer, IJetConnection, Matcher, JetFetcher, JetMethod
  - Exemplos funcionais: `ThirdParty/SharpJet/Examples/*`
  - Use para inferir estrutura exata dos frames JSON enviados (fetch, set, unfetch, method).
- Dicionário de objetos (paths JetBus e tipos de dados):
  - Arquivo: `Hbm.Automation.Api/Weighing/WTX/Jet/JetBusCommands.cs`
  - Contém todos os paths (`"xxxx/yy"`) e tipos DataType (BIT, U08, U16, U32, S32, ASCII, NIL), com offsets/length para bits.
  - Baseie-se nele para gerar/implementar o mapeamento em C++ (constantes + metadados).
- Lógica de leitura/atualização de Process Data:
  - `Hbm.Automation.Api/Data/JetProcessData.cs`
  - Calcula decimais, unidade, flags (Underload/Overload/Movement), TareMode, ScaleRange, ZeroRequired etc.
  - Usa MeasurementUtils para converter dígitos em double de acordo com Decimals.
- Conversões numéricas:
  - `Hbm.Automation.Api/Utils/MeasurementUtils.cs`
  - DigitToDouble, DoubleToDigit (usar os mesmos critérios).
- Semântica de comandos e polling:
  - `Hbm.Automation.Api/Weighing/DSE/DSEJet.cs`
  - Constantes de comando (SCALE_COMMAND_) e status (SCALE_COMMAND_STATUS_)
  - Métodos: Connect/Disconnect, Zero, Tare, SetGross, SaveAllParameters, AdjustZeroSignal, AdjustNominalSignal, AdjustNominalSignalWithCalibrationWeight
  - Fluxo de polling: escrever comando, aguardar STATUS_ONGOING, aguardar finalização, checar STATUS_OK
- Timer de publicação de ProcessData:
  - `Hbm.Automation.Api/Weighing/BaseWTDevice.cs` (ProcessDataTimer/ProcessDataUpdateTick)

## Requisitos funcionais da nova lib C++ (API alvo)

### JetBusClient (WebSocket + JSON)
- Conectar a `ws://<ip>:80/jet/canopen`
- `fetch(path)`/`unfetch(path)` com suporte a matcher (EqualsTo)
- `set(path, value)` com tipos inteiros/strings
- Buffer local thread-safe: `map<string path, string rawValue>`
- Eventos de atualização: `on_data(path, value, eventType)` e `on_fetch_done(success, token)`
- Reconexão com backoff exponencial (configurável), timeouts configuráveis
- Logs (nível trace/debug/info/warn/error)

### DataModel/ProcessData
- Replicar propriedades do .NET: `ApplicationMode`, `Weight` (Net/Gross/Tare), `PrintableWeight`, `Unit`, `Decimals`, `TareMode`, `WeightStable`, `ScaleRange`, `CenterOfZero`, `InsideZero`, `ZeroRequired`, `LegalForTrade`, `Underload`, `Overload`, `HigherSafeLoadLimit`, `GeneralScaleError`, `ScaleAlarm`
- Atualização reativa a partir do buffer (decodifica com base no dicionário)

### Device (DSEDevice)
- Propriedades e métodos equivalentes a DSEJet/BaseWTDevice:
  - Leitura: `SerialNumber`, `Identification`, `FirmwareVersion`, `WeightStep`, `ScaleRange`, `TareMode`, `WeightStable`, `ManualTareValue`, `MaximumCapacity`, `CalibrationWeight`, `ZeroValue`, `ZeroSignal`, `NominalSignal`, `Unit` etc.
  - Escrita: `Unit`, `ManualTareValue`, `MaximumCapacity`, `CalibrationWeight`, `ZeroSignal`, `NominalSignal`, `WeightStep`, `ScaleRangeMode` e limites, filtros
  - Comandos: `SetGross`, `Zero`, `Tare`, `TareManually`, `RecordWeight`
  - Ajustes: `AdjustZeroSignal`, `AdjustNominalSignal`, `AdjustNominalSignalWithCalibrationWeight`, `CalculateAdjustment(scaleZero_mV/V, capacity_mV/V)`
- Polling de status: replicar exatamente a lógica de `DSEJet.cs` (aguardar ONGOING, depois OK)
- Timer de publicação: callback periódico semelhante a `ProcessDataReceived` (intervalo configurável)

### Conversões e tipos
- Implementar conversões `Digit <-> Double` (`MeasurementUtils`)
- Mapear tipos `DataType` para C++ (`ASCII` -> `std::string`; `U08/U16/U32/S32` -> inteiros com sinalidade/tamanho corretos; `BIT` -> extração de bitfield `[offset, length]`)

## Requisitos técnicos e de build (Linux)
- C++17 ou superior
- Dependências:
  - nlohmann/json
  - Boost (Beast/Asio) ou WebSocket++ (com Asio)
  - OpenSSL (se necessário para ws/wss)
  - Threads/pthread
- CMake:
  - Configurar `find_package` para nlohmann_json (caso via pacote), Boost (system, thread, beast), OpenSSL
  - Targets:
    - `jetbus_core` (lib)
    - `dse_device` (lib) – depende de `jetbus_core`
    - `dse_console_example` (exe) – exemplo CLI
  - Opções:
    - `BUILD_SHARED_LIBS` ON/OFF
    - `ENABLE_RECONNECT`, `LOG_LEVEL`, `PROCESS_DATA_INTERVAL`, `FETCH_PATHS` configuráveis
  - Instalação e exportação (`install(TARGETS …)`, export configs)
- Testes:
  - `ctest` com testes unitários de conversões e parsing de eventos JSON simulados
  - Opcional: testes de integração com IP do DSE via variável de ambiente

## Design e organização de código (sugestão)
```
include/jetbus/
  client.hpp        (conexão WS, fetch/unfetch/set, callbacks)
  types.hpp         (DataType, enums, erros)
  matcher.hpp       (EqualsTo)
  commands.hpp      (mapeamento gerado de JetBusCommands)
  measurement_utils.hpp (digit<->double, unit strings)
  process_data.hpp  (estrutura e atualização)
src/jetbus/
  client.cpp
  commands.cpp
  process_data.cpp
include/dse/
  device.hpp        (API pública DSEDevice)
src/dse/
  device.cpp
examples/
  dse_console_example.cpp (CLI)
```

## Protocolo e eventos Jet/JetBus (inferidos do código)
- Conexão: `ws://<ip>:80/jet/canopen`
- Assinatura (fetch):
  - Enviar JSON de fetch com matcher `EqualsTo` para cada path desejado (ver DSEFetchTargets em `DSEJetConnection.cs`)
  - Receber eventos com JSON contendo: `"path"`, `"event"` (`"add"|"fetch"|"change"`), `"value"`
  - Atualizar buffer thread-safe por path
  - Unfetch para parar a assinatura
- Escrita (set):
  - Enviar JSON set com path e value (string/int)
  - Aguardar confirmação (callback/futuro) com timeout
  - Mensagens e estrutura exata: extrair do SharpJet (`ThirdParty/SharpJet/SharpJet`). Replique o shape dos requests e handling de responses (incluindo tokens/ids se houver).

### Paths a assinar inicialmente (como no .NET) – DSEFetchTargets
```
"6002/02"   // Scale command status
"6012/01"   // Weight status
"6013/01"   // Decimals
"6015/01"   // Unit/prefix
"6016/01"   // Weight step
"601A/01"   // Output weight (net)
"6113/01"   // Scale maximum capacity
"611C/01"   // Range control
"611C/02"   // limit1
"611C/03"   // limit2
"6141/02"   // Zero value (máx zeroing time no .NET)
"6143/00"   // Tare value
"6144/00"   // Gross value
"6153/00"   // Weight movement detection
"6002/00"   // Scale command status? (observado no código)
```

### Constantes e semântica de comando (copiar valores do .NET)
```
SCALE_COMMAND_CALIBRATE_ZERO = 2053923171
SCALE_COMMAND_CALIBRATE_NOMINAL = 1852596579
SCALE_COMMAND_EXIT_CALIBRATE = 1953069157
SCALE_COMMAND_TARE = 1701994868
SCALE_COMMAND_CLEAR_PEAK_VALUES = 1801545072
SCALE_COMMAND_ZERO = 1869768058
SCALE_COMMAND_SET_GROSS = 1936683623
SCALE_COMMAND_STATUS_ONGOING = 1634168417
SCALE_COMMAND_STATUS_OK = 1801543519
SCALE_COMMAND_STATUS_ERROR_E1 = 826629983
SCALE_COMMAND_STATUS_ERROR_E2 = 843407199
SCALE_COMMAND_STATUS_ERROR_E3 = 860184415
CONVERISION_FACTOR_MVV_TO_D = 1000000
```

### Conversões e unidades
- `DigitToDouble(value, decimals)` e `DoubleToDigit(value, decimals)` conforme `MeasurementUtils.cs`
- Mapear `Unit`:
  - `0x00020000` → kg
  - `0x004B0000` → g
  - `0x00A60000` → lb
  - `0x004C0000` → t
  - `0x00210000` → N
- Interpretação de bits de status (paths e offsets conforme `JetBusCommands.cs`)

## Plano de execução
1. Analisar e extrair requisitos:
   - Ler `DSEJetConnection.cs`, `DSEJet.cs`, `JetProcessData.cs`, `JetBusCommands.cs`
   - Catalogar todos os paths e tipos com base em `JetBusCommands.cs`
2. Definir arquitetura C++:
   - Projetar interfaces equivalentes: `IJetConnection/INetConnection` -> `JetBusClient`; `BaseWTDevice` -> abstrata `DeviceBase`; `DSEJet` -> `DSEDevice`
   - Assinatura, buffer e eventos (thread-safe)
3. Implementar JetBusClient:
   - Boost.Asio/Beast (ou WebSocket++)
   - Loop de recebimento de frames e parse JSON (nlohmann/json)
   - `fetch`/`unfetch`/`set`/`method` (requests e correlacionamento de respostas)
   - Reconnect/backoff/timeout
4. Implementar mapeamento e conversões:
   - `commands.(hpp|cpp)` com as entradas de `JetBusCommands.cs`
   - `measurement_utils.(hpp|cpp)`
5. Implementar ProcessData:
   - Leitura do buffer, cálculo de `PrintableWeight` e flags
6. Implementar DSEDevice:
   - Propriedades e set/get via client + conversions
   - Comandos (`Zero`/`Tare`/`SetGross`/`SaveAllParameters`/`Adjust…`)
   - Polling de status (`ONGOING` -> `OK`)
7. CMake:
   - Targets (libs e exemplo), dependências, install/exports
8. Exemplo console:
   - Conectar, imprimir Net/Gross/Tare + unidade, executar `Zero`/`Tare` e mostrar estados
9. Testes:
   - Unitário (conversões; parse de eventos JSON), integração (opcional)
10. Documentação:
    - README de build/run em Linux, opções de linha de comando, variáveis e dependências

## Critérios de aceite
- Conecta ao DSE via `ws://ip:80/jet/canopen`; inicia fetch nos paths do DSEFetchTargets; buffer atualiza mediante `"add"/"fetch"/"change"`.
- Mostra corretamente Net/Gross/Tare com unidade e decimais; flags de status coerentes.
- Executa `Zero`/`Tare`/`SetGross` e confirma via STATUS (`ONGOING` -> `OK`) dentro do timeout.
- Suporta reconexão automática (simular disconnect).
- Build em Linux via CMake; dependências resolvidas; binários gerados; exemplo funcional.

## Restrições e atenção
- Não copiar código de SharpJet/Json.NET; use apenas como referência de protocolo e estrutura. Licenças: Json.NET (MIT), veja HBM/SharpJet (verificar).
- GUI WinForms não será portada; apenas core. Para Linux UI, considerar CLI/Ncurses/Qt/Avalonia (fora do escopo aqui).
- Segurança: validar dados recebidos, timeouts robustos, reconexão, logs; evitar deadlocks (locks granulares).
- Performance: minimizar alocações e cópias ao parsear JSON, usar strands/executors para sincronização.

## Checklist de entrega
- Código-fonte C++ (`include/src`) conforme a organização proposta
- `CMakeLists.txt` no root e subdiretórios; instruções de build (README)
- Exemplo console (`examples/dse_console_example`)
- Testes básicos com `ctest`
- Script de build Linux (opcional) e lista de pacotes apt necessários. O repositório tem submodule.
