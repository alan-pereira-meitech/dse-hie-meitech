import express from "express";
import fs from "fs";
import fsPromises from "fs/promises";
import path from "path";
import { createServer } from "http";
import { WebSocketServer, WebSocket } from "ws";
import { config } from "./config";
import { DeviceManager } from "./server/deviceManager";
import { deviceActions } from "./server/actions";
import { JetBusError } from "./jetbus/types";

const isProd = process.env.NODE_ENV === "production";
const rootDir = path.resolve(__dirname, "..");
const publicDir = path.resolve(rootDir, "public");

async function start(): Promise<void> {
  const app = express();
  app.use(express.json());

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

  let wss: WebSocketServer | null = null;

  const broadcastProcessData = (payload: unknown): void => {
    if (!wss) return;
    const data = JSON.stringify({ type: "processData", payload });
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(data);
      }
    });
  };

  const broadcastState = (): void => {
    if (!wss) return;
    const data = JSON.stringify({ type: "state", payload: manager.getState() });
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(data);
      }
    });
  };

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

  if (isProd) {
    if (fs.existsSync(publicDir)) {
      app.use(express.static(publicDir));
    }

    app.get("*", (req, res, next) => {
      if (req.path.startsWith("/api") || req.path.startsWith("/ws")) {
        next();
        return;
      }
      const indexPath = path.join(publicDir, "index.html");
      if (!fs.existsSync(indexPath)) {
        res.status(404).json({
          error: "Frontend não encontrado. Execute `npm run build:client` para gerar o bundle."
        });
        return;
      }
      res.sendFile(indexPath, (err) => {
        if (err) {
          next(err);
        }
      });
    });
  } else {
    const { createServer: createViteServer } = await import("vite");
    const vite = await createViteServer({
      configFile: path.resolve(rootDir, "vite.config.ts"),
      root: path.resolve(rootDir, "src/client"),
      server: {
        middlewareMode: true
      },
      appType: "custom"
    });
    app.use(vite.middlewares);

    app.use("*", async (req, res, next) => {
      if (req.path.startsWith("/api") || req.path.startsWith("/ws")) {
        next();
        return;
      }
      try {
        const url = req.originalUrl;
        const templatePath = path.resolve(rootDir, "src/client/index.html");
        const template = await fsPromises.readFile(templatePath, "utf-8");
        const transformed = await vite.transformIndexHtml(url, template);
        res.status(200).set({ "Content-Type": "text/html" }).send(transformed);
      } catch (error) {
        vite.ssrFixStacktrace(error as Error);
        next(error);
      }
    });
  }

  app.use((err: unknown, _req: express.Request, res: express.Response, _next: express.NextFunction) => {
    const status = err instanceof JetBusError ? 400 : 500;
    res.status(status).json({
      error: err instanceof Error ? err.message : "Unknown error"
    });
  });

  const server = createServer(app);
  wss = new WebSocketServer({ server, path: "/ws/stream" });

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
}

void start().catch((error) => {
  // eslint-disable-next-line no-console
  console.error("Erro ao iniciar o servidor:", error);
  process.exit(1);
});
