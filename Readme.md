# Automation-API


[![Build status](https://hbmdevelopment.visualstudio.com/HBM%20Weighing/_apis/build/status/HBM%20Weighing%20API%20CI)](https://hbmdevelopment.visualstudio.com/HBM%20Weighing/_build/latest?definitionId=47)

Connect your own application to weighing terminals WTX110 and WTX120 or digital sensor electronic DSE from HBM.


Contains API and 3 templates (Console application, Simple GUI, PLC view). 


Documentation can be found on the [product page](https://www.hbm.com/wtx/).


## License



Copyright (c) 2019 HBM. See the [LICENSE](LICENSE) file for license rights and
limitations (MIT).

## Migração para C++ (Linux)

Consulte o documento [Prompt para Migração do Núcleo Jet/JetBus para C++ com CMake](docs/JetBusCppMigrationPrompt.md) para um guia detalhado de como portar o núcleo Jet/JetBus para uma nova biblioteca C++ multiplataforma.

## Núcleo Jet/JetBus em C++

Este repositório agora inclui uma implementação em C++17 do cliente Jet/JetBus e das funcionalidades específicas do DSE. O código está localizado em `include/` e `src/` com a seguinte organização:

- `jetbus_core`: biblioteca que encapsula o cliente WebSocket (Boost.Beast), buffer de dados e processamento de Process Data.
- `dse_device`: biblioteca que expõe uma API de alto nível compatível com o comportamento do `DSEJet` original (.NET).
- `examples/dse_console_example.cpp`: exemplo de console que se conecta ao dispositivo, acompanha Net/Gross/Tare e executa comandos básicos (Zero, Tare, SetGross).

### Requisitos de build (Linux)

- CMake ≥ 3.16
- Compilador C++ com suporte a C++17
- Boost (componentes `system`)
- pthreads
- A biblioteca [nlohmann/json](https://github.com/nlohmann/json) é obtida automaticamente via `FetchContent` caso não esteja instalada no sistema.
- O conjunto de testes utiliza Catch2 (também baixado automaticamente).

### Como compilar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Isso gera as bibliotecas `jetbus_core`, `dse_device`, o executável de exemplo `dse_console_example` e o binário de testes `jetbus_tests`.

### Executando testes

```bash
ctest --test-dir build
```

### Executando o exemplo de console

```bash
./build/dse_console_example <ip-do-dse>
```

O exemplo estabelece a conexão em `ws://<ip>:80/jet/canopen`, exibe os valores Net/Gross/Tare em tempo real e dispara os comandos Zero, Tare e SetGross.
