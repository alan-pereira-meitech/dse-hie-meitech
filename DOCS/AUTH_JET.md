# Jet Authentication Workflow

The DSE Jet transport performs a native WebSocket handshake followed by Jet-specific authentication and subscription steps. The sequence implemented by the legacy .NET reference (`Hbm.Automation.API/Weighing/DSE/Jet/DSEJetConnection.cs`) and mirrored by this C++ SDK is:

1. **TCP connect + WebSocket upgrade**
   * Endpoint: `ws://<host>:80/jet/canopen` or `wss://<host>:443/jet/canopen` when TLS is enabled.
   * Only the HTTP upgrade handshake uses HTTP headers; all payload frames remain Jet JSON messages.
2. **`authenticate` call**
   * JSON request emitted by the SDK (see `MakeAuthenticateRequest`):
     ```json
     {
       "type": "call",
       "id": <unique counter>,
       "method": "authenticate",
       "params": {"user": "<username>", "password": "<password>"}
     }
     ```
   * The device replies with a `result` or `error` frame. On success the SDK marks the connection as ready and proceeds to subscriptions. **TODO:** Confirm the exact response schema with official Jet documentation; current assumptions are based on the .NET stubs and may require adjustment.
3. **Initial fetch / subscription**
   * After authentication the client issues `fetch` calls for the desired paths (with `subscribe=true`) so the peer pushes `add`/`change`/`fetch` events. The event payloads contain `{ "type": "event", "path": "<jet path>", "event": "change", "value": ... }` as derived from the unit tests.
4. **Keep-alive**
   * WebSocket control frames (`PING`/`PONG`) are used for transport keep-alive in addition to Jet application heartbeats.

## Credentials and environment variables

The SDK reads credentials from either the process environment or a `.env` file in the project root:

| Variable | Description |
| -------- | ----------- |
| `DSE_HOST` | Hostname or IP of the DSE Jet peer. |
| `DSE_PORT` | TCP port (`80` for plaintext, `443` for TLS). |
| `DSE_TLS` | `yes`/`no` toggle for TLS. |
| `DSE_USER` / `DSE_PASSWORD` | Jet credentials provisioned on the device. |
| `DSE_JET_PATH` | WebSocket resource path (default `/jet/canopen`). |

**Important:** The repository does not include production credentials. Populate `.env` or the runtime environment variables with site-specific secrets before running the SDK or examples.

