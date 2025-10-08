# DSE JetBus Node + TypeScript

Refatoração do cliente JetBus para TypeScript/Node.js com comunicação WebSocket e painel web que expõe todas as operações declaradas em `device.hpp` (agora portadas para TypeScript). O projeto usa Express para a API REST/WebSocket proxy e uma interface web estática para interação humana.

## Visão geral

- **WebSocket JetBus**: cliente TypeScript inspirado no código C++ original (`jetbus/client.*`).
- **Device API**: classe `Device` em TypeScript com os mesmos métodos documentados em `device.hpp`.
- **Interface Web**: página única (`public/index.html`) que lista e executa todas as funções disponíveis, além de exibir dados de processo em tempo real.
- **Configuração flexível**: informe a URL do dispositivo diretamente na UI ou via variável de ambiente `DEVICE_URL`.

## Pré-requisitos

- Node.js 18+ (recomendado 20 LTS)
- npm 9+

## Instalação

```bash
npm install
```

## Desenvolvimento

Execute o servidor com `ts-node` (hot reload simples).

```bash
npm run dev
```

O servidor ficará disponível em `http://localhost:3000`. A página web permite conectar/desconectar, visualizar dados de processo e executar qualquer função do dispositivo.

## Produção / build

```bash
npm run build
npm start
```

Os arquivos compilados ficam em `dist/`. O comando `npm start` sobe o servidor Express servindo o bundle JavaScript e os arquivos estáticos.

## Variáveis de ambiente

- `DEVICE_URL`: URL WebSocket padrão para o JetBus (ex: `ws://192.168.1.100/jet/canopen`). Pode ser sobrescrita pela interface web.
- `PORT`: porta HTTP da aplicação (default 3000).

## Estrutura

```
├── src
│   ├── dse
│   │   ├── device.ts              # Classe Device com os métodos portados de device.hpp
│   │   └── deviceFunctions.ts     # Metadados usados pela UI para gerar a lista de funções
│   ├── jetbus
│   │   ├── client.ts              # Cliente JetBus/WebSocket
│   │   ├── commands.ts            # Comandos e paths equivalentes aos do C++
│   │   ├── measurementUtils.ts    # Funções utilitárias (double<->digit)
│   │   └── processData.ts         # Parser de dados de processo
│   └── server.ts                  # API Express + servidor de arquivos estáticos
├── public
│   └── index.html                 # Interface web (lista todas as funções disponíveis)
└── tsconfig.json
```

## Como usar

1. Defina a URL do dispositivo na interface ou via `DEVICE_URL`.
2. Clique em **Conectar** para estabelecer a sessão JetBus.
3. Acompanhe os dados de processo (peso líquido, bruto, tara, flags de status etc.).
4. Utilize os cartões de funções para executar comandos ou atualizar parâmetros (tare, zero, ajustes, leitura de identificação, etc.).

Os resultados das chamadas são exibidos no próprio cartão, permitindo verificar respostas numéricas, booleanas ou mensagens de erro.

## Observações

- O cliente JetBus mantém cache dos valores recebidos via eventos `fetch` e replica a semântica do código C++ original (incluindo timeouts e códigos de comando).
- A UI é responsiva e utiliza apenas HTML/CSS/JS, sem dependências externas.
- Para execução headless (sem UI), a API REST pode ser utilizada diretamente (`/api/device/functions/:name`).

## Licença

Nenhuma licença foi fornecida com o projeto original.
