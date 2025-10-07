# JetBus C++ Port Review

## Referências consultadas
Durante a revisão não foi possível encontrar no código qualquer menção às implementações de referência "node-jet" ou "SharpJet". As classes fornecidas não citam os conceitos centrais dessas bibliotecas (por exemplo, registros JET, operações `call`/`fetch` via JSON-RPC, websockets ou publicação/assinatura). O código existente funciona apenas como um armazenamento local em memória e portanto não reflete o protocolo JetBus documentado nas referências.

## Problemas principais observados
- `JetBusConnection` mantém um `std::map` interno com os valores da balança, mas não abre sockets TCP/WebSocket, não serializa mensagens JSON nem autentica contra um servidor JetBus. O método `connect()` apenas marca um `bool` como verdadeiro e nenhuma troca de mensagens é feita com o dispositivo.【F:cpp/src/jet_bus_connection.cpp†L9-L68】
- A implementação de comandos (`JetBusCommand`) limita-se a conversões numéricas simples, ignorando o modelo de comunicação baseado em JSON-RPC definido pelo protocolo JetBus e pelas bibliotecas de referência.【F:cpp/src/jet_bus_command.cpp†L1-L44】
- As classes de nível mais alto (`BaseWtDevice`, `WtxJet`) apenas manipulam o mapa local fornecido pela conexão simulada e não executam chamadas `fetch`, `set`, `call` ou mecanismos de subscription típicos do JetBus.【F:cpp/src/wtx_jet.cpp†L1-L99】

## Conclusão
O código atual não implementa o protocolo JetBus de forma correta nem reutiliza os conceitos das bibliotecas de referência indicadas. Para alinhar-se às referências, é necessário introduzir uma camada de transporte baseada em WebSocket/JSON-RPC, implementar os comandos previstos pelo JetBus (como `fetch`, `set`, `call` e subscriptions), bem como lidar com autenticação e tratamento de erros conforme exemplificado em `node-jet` e `SharpJet`.
