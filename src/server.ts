import cors from 'cors';
import express from 'express';
import { createServer } from 'node:http';
import path from 'node:path';
import { WebSocketServer, WebSocket } from 'ws';

import { Device, DeviceOptions } from './dse/device.js';
import { DEVICE_FUNCTIONS, DeviceFunctionMeta } from './dse/deviceFunctions.js';
import { ProcessDataSnapshot } from './jetbus/processData.js';

const DEFAULT_URL = process.env.DEVICE_URL ?? 'ws://127.0.0.1/jet/canopen';
const PORT = Number(process.env.PORT ?? 3000);

let currentDevice: Device | null = null;
let currentUrl = DEFAULT_URL;
let deviceConnected = false;
let lastProcessSnapshot: ProcessDataSnapshot | null = null;

const weightClients = new Set<WebSocket>();

function buildWeightPayload(): string {
  const payload = {
    type: 'weight' as const,
    connected: deviceConnected && lastProcessSnapshot !== null,
    net: lastProcessSnapshot?.printableWeight.net,
    gross: lastProcessSnapshot?.printableWeight.gross,
    tare: lastProcessSnapshot?.printableWeight.tare,
    unit: lastProcessSnapshot?.unit,
    decimals: lastProcessSnapshot?.decimals,
    stable: lastProcessSnapshot?.weightStable ?? false
  };
  return JSON.stringify(payload);
}

function broadcastWeight(snapshot?: ProcessDataSnapshot | null): void {
  if (snapshot !== undefined) {
    lastProcessSnapshot = snapshot;
  }
  const message = buildWeightPayload();
  for (const client of weightClients) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(message);
    }
  }
}

async function ensureDevice(url: string): Promise<Device> {
  if (currentDevice && currentUrl === url) {
    currentDevice.setProcessDataCallback((snapshot) => {
      broadcastWeight(snapshot);
    });
    return currentDevice;
  }
  if (currentDevice) {
    await currentDevice.disconnect().catch(() => undefined);
    deviceConnected = false;
    broadcastWeight(null);
  }
  const options: DeviceOptions = {
    clientOptions: {
      url,
      enableReconnect: true,
      requestTimeout: 5000,
      enableDebugLogs: false
    }
  };
  const device = new Device(options);
  device.setProcessDataCallback((snapshot) => {
    broadcastWeight(snapshot);
  });
  currentDevice = device;
  currentUrl = url;
  return currentDevice;
}

function requireDevice(): Device {
  if (!currentDevice) {
    throw new Error('Device is not initialized. Connect first.');
  }
  return currentDevice;
}

async function invokeFunction(device: Device, meta: DeviceFunctionMeta, args: Record<string, unknown>): Promise<unknown> {
  const method = (device as any)[meta.method];
  if (typeof method !== 'function') {
    throw new Error(`Método ${String(meta.method)} não encontrado`);
  }
  const callArgs = meta.inputs?.map((input) => {
    if (!(input.name in args)) {
      throw new Error(`Parâmetro obrigatório: ${input.name}`);
    }
    const raw = args[input.name];
    if (input.type === 'number') {
      const value = Number(raw);
      if (Number.isNaN(value)) {
        throw new Error(`Valor inválido para ${input.name}`);
      }
      return value;
    }
    return String(raw);
  }) ?? [];
  const result = method.apply(device, callArgs);
  if (result instanceof Promise) {
    return await result;
  }
  return result;
}

async function main(): Promise<void> {
  const app = express();
  app.use(cors());
  app.use(express.json());

  const server = createServer(app);
  const wss = new WebSocketServer({ server, path: '/process' });

  wss.on('connection', (socket) => {
    weightClients.add(socket);
    socket.send(buildWeightPayload());
    socket.on('close', () => {
      weightClients.delete(socket);
    });
    socket.on('error', () => {
      weightClients.delete(socket);
    });
  });

  app.post('/api/device/connect', async (req, res) => {
    try {
      const url = typeof req.body?.url === 'string' ? req.body.url : DEFAULT_URL;
      const device = await ensureDevice(url);
      if (!device.isConnected()) {
        await device.connect();
      }
      deviceConnected = true;
      broadcastWeight(device.snapshot().processData);
      res.json({ connected: true, url });
    } catch (error) {
      console.error('connect error', error);
      res.status(500).json({ error: (error as Error).message });
    }
  });

  app.post('/api/device/disconnect', async (_req, res) => {
    try {
      if (currentDevice && currentDevice.isConnected()) {
        await currentDevice.disconnect();
      }
      deviceConnected = false;
      broadcastWeight(null);
      res.json({ connected: false });
    } catch (error) {
      res.status(500).json({ error: (error as Error).message });
    }
  });

  app.get('/api/device/status', (_req, res) => {
    try {
      if (!currentDevice) {
        res.json({ connected: false });
        return;
      }
      res.json(currentDevice.snapshot());
    } catch (error) {
      res.status(500).json({ error: (error as Error).message });
    }
  });

  app.get('/api/device/functions', (_req, res) => {
    res.json(DEVICE_FUNCTIONS);
  });

  app.post('/api/device/functions/:name', async (req, res) => {
    try {
      const device = requireDevice();
      if (!device.isConnected()) {
        res.status(503).json({ error: 'Device not connected' });
        return;
      }
      const name = req.params.name;
      const meta = DEVICE_FUNCTIONS.find((fn) => fn.name === name);
      if (!meta) {
        res.status(404).json({ error: `Função ${name} não encontrada` });
        return;
      }
      const result = await invokeFunction(device, meta, req.body ?? {});
      res.json({ name, result: result ?? null });
    } catch (error) {
      res.status(500).json({ error: (error as Error).message });
    }
  });

  const publicDir = path.resolve(process.cwd(), 'public');
  app.use(express.static(publicDir));

  app.get('*', (_req, res) => {
    res.sendFile(path.join(publicDir, 'index.html'));
  });

  server.listen(PORT, () => {
    console.log(`DSE JetBus UI disponível em http://localhost:${PORT}`);
  });
}

main().catch((error) => {
  console.error('Fatal error starting server', error);
  process.exitCode = 1;
});
