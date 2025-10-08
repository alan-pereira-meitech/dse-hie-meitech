import express from "express";
import path from "path";
import { createServer } from "http";
import { WebSocketServer, WebSocket } from "ws";
import { config } from "./config";
import { DeviceManager } from "./server/deviceManager";
import { deviceActions } from "./server/actions";
import { JetBusError } from "./jetbus/types";

const app = express();
app.use(express.json());

const publicDir = path.resolve(__dirname, "../public");
app.use(express.static(publicDir));

const manager = new DeviceManager({
  clientOptions: {
    url: config.deviceUrl,
    enableReconnect: true,
    enableDebugLogs: config.enableDebugLogs,
    reconnectInitialDelay: 500,
    reconnectMaxDelay: 30_000,
    requestTimeout: 5_000
  }
});

app.get("/api/state", (_req, res) => {
  res.json(manager.getState());
});

app.get("/api/actions", (_req, res) => {
  res.json({ actions: deviceActions });
});

app.post("/api/connect", async (_req, res, next) => {
  try {
    const state = await manager.connect();
    broadcastState();
    res.json(state);
  } catch (error) {
    next(error);
  }
});

app.post("/api/disconnect", async (_req, res, next) => {
  try {
    const state = await manager.disconnect();
    broadcastState();
    res.json(state);
  } catch (error) {
    next(error);
  }
});

app.post("/api/action", async (req, res, next) => {
  const { action, payload } = req.body ?? {};
  if (typeof action !== "string" || action.trim().length === 0) {
    res.status(400).json({ error: "action is required" });
    return;
  }
  try {
    const result = await manager.performAction(action, payload);
    broadcastState();
    res.json({ result });
  } catch (error) {
    next(error);
  }
});

app.use((err: unknown, _req: express.Request, res: express.Response, _next: express.NextFunction) => {
  const status = err instanceof JetBusError ? 400 : 500;
  res.status(status).json({
    error: err instanceof Error ? err.message : "Unknown error"
  });
});

function broadcastProcessData(payload: unknown): void {
  const data = JSON.stringify({ type: "processData", payload });
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
}

function broadcastState(): void {
  const data = JSON.stringify({ type: "state", payload: manager.getState() });
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
}

const server = createServer(app);
const wss = new WebSocketServer({ server, path: "/ws/stream" });

manager.onData((snapshot) => {
  broadcastProcessData(snapshot);
  broadcastState();
});

wss.on("connection", (socket) => {
  socket.send(
    JSON.stringify({
      type: "state",
      payload: manager.getState()
    })
  );
  const snapshot = manager.getState().snapshot;
  if (snapshot) {
    socket.send(
      JSON.stringify({
        type: "processData",
        payload: snapshot
      })
    );
  }
});

server.listen(config.port, () => {
  // eslint-disable-next-line no-console
  console.log(`Server listening on http://localhost:${config.port}`);
});
