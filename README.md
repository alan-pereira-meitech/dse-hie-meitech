# DSE Jet Communication (C++)

Biblioteca moderna em C++ para comunicação com dispositivos DSE via protocolo JetBus, com foco na gestão dos filtros digitais e demais parâmetros de pesagem.

## Estrutura

- `include/`: cabeçalhos públicos da biblioteca.
- `src/`: implementação da biblioteca.
- `samples/`: exemplo de aplicação em linha de comando.

## Funcionalidades

- Conexão TCP direta ao endpoint Jet (`JetConnection`).
- Classe de alto nível `DSEJet` com métodos para:
  - Ler identificação, número de série e versão de firmware.
  - Gerir filtros digitais por estágio (modo e frequência de corte).
  - Configurar filtros passa-baixa (FIR e IIR).
  - Restaurar parâmetros de fábrica do dispositivo.

## Compilação

```bash
cmake -S . -B build
cmake --build build
```

O executável de exemplo `configure_filters` será gerado em `build/` e pode ser executado com:

```bash
./build/configure_filters <ip_do_dse> [porta]
```

O exemplo estabelece a conexão, imprime metadados e aplica uma configuração de filtro FIR comb no estágio 2.
