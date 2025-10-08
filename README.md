# DSE JetBus Dashboard

Aplicação Node.js + TypeScript com interface web para monitorar e controlar balanças HBM DSE via JetBus (WebSocket). O backend mantém uma réplica fiel do cliente JetBus original (handshake, fila de mensagens, comandos `fetch/set/unfetch`) e expõe os dados em tempo real para um painel web responsivo.

## Visão geral

- **Backend em TypeScript (Node 18+)** usando Express e `ws` para conectar ao dispositivo em `ws://192.168.1.243/jet/canopen` (ajustável via variável `DEVICE_WS_URL`).
- **Cliente JetBus reimplementado**: mesmos headers de handshake, `Sec-WebSocket-Protocol: jet`, permessage-deflate desabilitado, timeouts e fila de requisições como no código C++ original.
- **Dashboard em tempo real** (`/`): streaming de Net/Gross/Tare e estado da balança, com atalho para o painel avançado.
- **Painel de Ações** (`/settings.html`): formulários agrupados e log lateral para executar comandos, leituras e calibrações.
- **Sessão de configurações completa**: todos os comandos expostos pela antiga classe `dse::Device` (tare, zero, ajustes, leituras de parâmetros e gravações).
- **Testes automatizados** com Vitest para utilitários de medição e interpretação de dados.

## Requisitos

- Node.js 18 ou superior
- npm 9+ (ou compatível)

## Instalação

```bash
npm install
```

Você pode configurar o endpoint JetBus exportando `DEVICE_WS_URL` (padrão `ws://192.168.1.243/jet/canopen`).

## Desenvolvimento

```bash
# Gera o bundle front-end em modo watch e sobe o servidor com hot reload
npm run dev
```

O servidor ficará disponível em `http://localhost:3000`. O painel web consome os endpoints REST (`/api/*`) e o WebSocket local (`/ws/stream`) para o stream de dados.

### Scripts úteis

- `npm run bundle:frontend` – gera `public/assets/app.js` com o dashboard
- `npm run build:server` – compila o backend TypeScript para `dist/`
- `npm run build` – build completo (backend + front-end)
- `npm start` – executa o servidor compilado a partir de `dist/`
- `npm test` – roda os testes com Vitest

## Estrutura de diretórios

```
public/           # index.html e assets estáticos
src/
  config.ts       # configuração (porta, URL do dispositivo)
  index.ts        # entrypoint do servidor Express/WebSocket
  dse/            # Device em TypeScript
  jetbus/         # Cliente JetBus, comandos e utilitários
  server/         # DeviceManager e metadados de ações
web/              # código TypeScript do dashboard (bundle via esbuild)
tests/            # testes unitários (Vitest)
```

## Fluxo de dados

1. `JetBusClient` conecta ao dispositivo via WebSocket usando os mesmos headers e timeouts do cliente C++.
2. `Device` realiza os `fetch` padrão, aguarda a primeira amostra de processo e mantém o cache atualizado.
3. `DeviceManager` converte os dados em um snapshot amigável e dispara:
   - WebSocket local (`/ws/stream`) com eventos `processData` e `state`
   - Endpoints REST para ações (`/api/action`, `/api/connect`, `/api/state`)
4. `web/index.ts` consome os eventos, atualiza o painel e envia comandos sob demanda.

## Testes

```bash
npm test
```

Os testes garantem a conversão dígito↔peso (`measurementUtils`) e a interpretação correta do cache JetBus em `ProcessData`.

## Observações

- Certifique-se de que o dispositivo JetBus esteja acessível a partir da máquina que executa o Node (porta 80 por padrão).
- Para produção, execute `npm run build` seguido de `npm start`.
- Artefatos gerados (ex.: `dist/`, `public/assets/`) são ignorados pelo Git — gere-os com os scripts acima.
