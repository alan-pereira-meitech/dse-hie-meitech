# DSE JetBus Node + TypeScript

Refatoração do cliente JetBus para TypeScript/Node.js com comunicação WebSocket e painel web que expõe todas as operações declaradas em `device.hpp` (agora portadas para TypeScript). O projeto usa Express para a API REST/WebSocket proxy e uma interface web estática para interação humana.

## Visão geral

- **WebSocket JetBus**: cliente TypeScript inspirado no código C++ original (`jetbus/client.*`).
- **Device API**: classe `Device` em TypeScript com os mesmos métodos documentados em `device.hpp`.
- **Interface Web (React)**: SPA em React + TypeScript (bundle via esbuild) que lista e executa todas as funções disponíveis, além de exibir dados de processo em tempo real.
- **Configuração flexível**: informe a URL do dispositivo diretamente na UI ou via variável de ambiente `DEVICE_URL`.

## Pré-requisitos

- Node.js 18+ (recomendado 20 LTS)
- npm 9+

## Instalação

```bash
npm install
```

## Comandos rápidos

Todos os passos abaixo devem ser executados dentro da raiz do projeto (`dse-hie-meitech`).

| Objetivo | Comando |
| --- | --- |
| Rodar em modo desenvolvimento com recarga automática | `npm run dev` |
| Gerar o build TypeScript | `npm run build` |
| Iniciar o servidor a partir do build gerado | `npm start` |

Para ambientes automatizados (CI/CD), execute `npm ci` em vez de `npm install` para garantir versões determinísticas.

## Desenvolvimento

Execute o servidor Express e o bundler React em paralelo com hot reload.

```bash
npm run dev
```

O servidor ficará disponível em `http://localhost:3000`. A página React recompila automaticamente, permitindo conectar/desconectar, visualizar dados de processo e executar qualquer função do dispositivo.

### Não é necessário compilar C++

Todo o fluxo agora roda apenas em Node.js + TypeScript + React. O código C++ original foi removido para simplificar o repositório, portanto não é preciso instalar toolchains, CMake ou bibliotecas nativas para executar a aplicação web.

## Produção / build

```bash
npm run build
npm start
```

O comando `npm run build` compila o servidor (TypeScript) e gera o bundle React em `public/assets`. Já `npm start` executa o servidor Express a partir de `dist/`, servindo os arquivos estáticos construídos.

## Variáveis de ambiente

- `DEVICE_URL`: URL WebSocket padrão para o JetBus (ex: `ws://192.168.1.100/jet/canopen`). O valor também é enviado à interface web, que o usa como sugestão inicial no campo de conexão.
- `PORT`: porta HTTP da aplicação (default 3000).

## Estrutura

```
├── src
│   ├── dse
│   │   ├── device.ts              # Classe Device com os métodos portados de device.hpp
│   │   └── deviceFunctions.ts     # Metadados usados pela UI e API
│   ├── jetbus
│   │   ├── client.ts              # Cliente JetBus/WebSocket
│   │   ├── commands.ts            # Comandos e paths equivalentes aos do C++
│   │   ├── measurementUtils.ts    # Funções utilitárias (double<->digit)
│   │   └── processData.ts         # Parser de dados de processo
│   ├── server.ts                  # API Express + servidor de arquivos estáticos
│   └── web
│       ├── App.tsx                # SPA React com a dashboard
│       ├── components/            # Componentes (cartões de função etc.)
│       ├── index.tsx              # Ponto de entrada React
│       └── styles.css             # Estilos da interface
├── public
│   └── index.html                 # HTML base (injeta o bundle React)
└── tsconfig.json
```

## Como usar

1. Defina a URL do dispositivo na interface (o campo já vem preenchido com o valor de `DEVICE_URL`, quando configurado) ou informe-a via variável de ambiente.
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
