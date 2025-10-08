import { EventEmitter } from "events";
import WebSocket from "ws";
import { JetBusError, JetEventType, eventTypeFromString } from "./types";

export interface JetBusClientOptions {
  url: string;
  enableReconnect?: boolean;
  reconnectInitialDelay?: number;
  reconnectMaxDelay?: number;
  requestTimeout?: number;
  enableDebugLogs?: boolean;
}

export type DataCallback = (path: string, value: string, event: JetEventType) => void;
export type FetchCallback = (success: boolean, token: string) => void;

interface PendingMessage {
  payload: string;
  token?: string;
  resolve?: (value: unknown) => void;
  reject?: (reason: Error) => void;
  timeout?: NodeJS.Timeout;
}

interface PendingWaiter {
  predicate: (value: string) => boolean;
  resolve: (value: boolean) => void;
  timer: NodeJS.Timeout;
}

interface ParsedUrl {
  protocol: "ws" | "wss";
  host: string;
  port: string;
  target: string;
  hostHeader: string;
}

function parseUrl(rawUrl: string): ParsedUrl {
  try {
    const url = new URL(rawUrl);
    const protocol = url.protocol.replace(":", "") as "ws" | "wss";
    const host = url.hostname;
    const port = url.port || (protocol === "wss" ? "443" : "80");
    const pathname = url.pathname || "/";
    const target = pathname + (url.search ?? "");
    const defaultPort = protocol === "wss" ? "443" : "80";
    const hostHeader = port === defaultPort ? host : `${host}:${port}`;
    return { protocol, host, port, target, hostHeader };
  } catch (error) {
    throw new JetBusError(`Invalid JetBus URL: ${rawUrl}`);
  }
}

function buildFetchPayload(id: number, path: string): string {
  return JSON.stringify({
    id,
    method: "fetch",
    params: {
      path: { equals: path },
      caseInsensitive: false,
      id
    }
  });
}

function buildUnfetchPayload(id: number, token: string): string {
  return JSON.stringify({
    id,
    method: "unfetch",
    params: { token }
  });
}

function buildSetPayload(id: number, path: string, value: unknown): string {
  return JSON.stringify({
    id,
    method: "set",
    params: { path, value }
  });
}

export class JetBusClient {
  private readonly options: Required<JetBusClientOptions>;
  private readonly cache = new Map<string, string>();
  private readonly queue: PendingMessage[] = [];
  private readonly pendingPromises = new Map<number, PendingMessage>();
  private readonly pendingTokens = new Map<number, string>();
  private readonly waiters = new Map<string, Set<PendingWaiter>>();
  private readonly emitter = new EventEmitter();
  private parsedUrl: ParsedUrl;
  private ws: WebSocket | null = null;
  private running = false;
  private reconnectDelay: number;
  private nextId = 1;
  private dataCallback?: DataCallback;
  private fetchCallback?: FetchCallback;
  private openPromise: Promise<void> | null = null;

  constructor(options: JetBusClientOptions) {
    if (!options.url) {
      throw new JetBusError("JetBusClient requires a non-empty URL");
    }
    this.options = {
      enableReconnect: true,
      reconnectInitialDelay: 500,
      reconnectMaxDelay: 30_000,
      requestTimeout: 5_000,
      enableDebugLogs: false,
      ...options
    };
    this.parsedUrl = parseUrl(this.options.url);
    this.reconnectDelay = this.options.reconnectInitialDelay;
  }

  setDataCallback(callback: DataCallback | undefined): void {
    this.dataCallback = callback;
  }

  setFetchCallback(callback: FetchCallback | undefined): void {
    this.fetchCallback = callback;
  }

  connect(): void {
    if (this.running) {
      return;
    }
    this.running = true;
    this.openSocket();
  }

  disconnect(): void {
    this.running = false;
    this.reconnectDelay = this.options.reconnectInitialDelay;
    this.closeSocket();
    this.rejectAllPending(new JetBusError("JetBus client disconnected"));
  }

  async fetch(path: string): Promise<string> {
    await this.ensureOpen();
    const id = this.nextId++;
    const token = String(id);
    const payload = buildFetchPayload(id, path);
    const response = await this.enqueueWithResponse(id, payload, token);
    if (response && typeof response === "object" && "error" in response) {
      throw new JetBusError(`Fetch request failed for path: ${path}`);
    }
    return token;
  }

  async unfetch(token: string): Promise<void> {
    await this.ensureOpen();
    const id = this.nextId++;
    const payload = buildUnfetchPayload(id, token);
    this.enqueue({ payload });
  }

  async set(path: string, value: unknown): Promise<void> {
    await this.ensureOpen();
    const id = this.nextId++;
    const payload = buildSetPayload(id, path, value);
    const response = await this.enqueueWithResponse(id, payload);
    if (response && typeof response === "object" && "error" in response) {
      throw new JetBusError(`Set request failed for path: ${path}`);
    }
  }

  readCached(path: string): string | undefined {
    return this.cache.get(path);
  }

  snapshot(): Map<string, string> {
    return new Map(this.cache);
  }

  async waitFor(
    path: string,
    predicate: (value: string) => boolean,
    timeout: number
  ): Promise<boolean> {
    const cached = this.cache.get(path);
    if (cached !== undefined && predicate(cached)) {
      return true;
    }

    return new Promise<boolean>((resolve) => {
      const timer = setTimeout(() => {
        const set = this.waiters.get(path);
        if (set) {
          for (const waiter of set) {
            if (waiter.resolve === resolve) {
              set.delete(waiter);
              break;
            }
          }
          if (set.size === 0) {
            this.waiters.delete(path);
          }
        }
        resolve(false);
      }, timeout);

      const waitersForPath = this.waiters.get(path) ?? new Set<PendingWaiter>();
      waitersForPath.add({
        predicate,
        resolve,
        timer
      });
      this.waiters.set(path, waitersForPath);
    });
  }

  private enqueueWithResponse(
    id: number,
    payload: string,
    token?: string
  ): Promise<unknown> {
    return new Promise<unknown>((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pendingPromises.delete(id);
        this.pendingTokens.delete(id);
        reject(new JetBusError("JetBus request timed out"));
      }, this.options.requestTimeout);

      const message: PendingMessage = {
        payload,
        token,
        resolve,
        reject,
        timeout
      };

      this.pendingPromises.set(id, message);
      if (token) {
        this.pendingTokens.set(id, token);
      }

      this.enqueue(message);
    });
  }

  private enqueue(message: PendingMessage): void {
    this.queue.push(message);
    this.flushQueue();
  }

  private debug(message: string): void {
    if (this.options.enableDebugLogs) {
      // eslint-disable-next-line no-console
      console.debug(`[JetBus] ${message}`);
    }
  }

  private openSocket(): void {
    if (!this.running) {
      return;
    }

    this.closeSocket();

    const { protocol, hostHeader, host, port, target } = this.parsedUrl;
    const url = `${protocol}://${host}:${port}${target}`;
    this.debug(`Opening websocket to ${url}`);

    const ws = new WebSocket(url, "jet", {
      headers: {
        Host: hostHeader,
        Upgrade: "websocket",
        Connection: "Upgrade",
        "Sec-WebSocket-Protocol": "jet"
      },
      perMessageDeflate: false
    });

    this.ws = ws;
    this.openPromise = new Promise<void>((resolve, reject) => {
      const cleanup = (): void => {
        ws.off("open", handleOpen);
        ws.off("error", handleError);
      };

      const handleOpen = (): void => {
        cleanup();
        this.reconnectDelay = this.options.reconnectInitialDelay;
        this.debug("Websocket connected");
        resolve();
        this.flushQueue();
      };

      const handleError = (error: Error): void => {
        cleanup();
        reject(error);
      };

      ws.once("open", handleOpen);
      ws.once("error", handleError);
    });

    ws.on("message", (data) => {
      try {
        this.handleMessage(data.toString());
      } catch (error) {
        this.debug(`Failed to handle message: ${(error as Error).message}`);
      }
    });

    ws.on("close", () => {
      this.debug("Websocket closed");
      this.openPromise = null;
      this.notifyDisconnect(new JetBusError("JetBus connection closed"));
      if (this.running && this.options.enableReconnect) {
        const delay = this.reconnectDelay;
        this.reconnectDelay = Math.min(
          this.reconnectDelay * 2,
          this.options.reconnectMaxDelay
        );
        setTimeout(() => this.openSocket(), delay);
      }
    });

    ws.on("error", (error) => {
      this.debug(`Websocket error: ${error.message}`);
    });
  }

  private closeSocket(): void {
    if (this.ws) {
      try {
        this.ws.removeAllListeners();
        this.ws.close();
      } catch {
        // ignore errors on shutdown
      }
      this.ws = null;
      this.openPromise = null;
    }
  }

  private notifyDisconnect(error: JetBusError): void {
    this.rejectAllPending(error);
  }

  private rejectAllPending(error: JetBusError): void {
    for (const [, message] of this.pendingPromises) {
      if (message.timeout) clearTimeout(message.timeout);
      message.reject?.(error);
    }
    this.pendingPromises.clear();
    this.pendingTokens.clear();
  }

  private async ensureOpen(): Promise<void> {
    this.connect();
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      return;
    }
    if (!this.openPromise) {
      this.openSocket();
    }
    await this.openPromise;
  }

  private flushQueue(): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      return;
    }
    while (this.queue.length > 0) {
      const message = this.queue.shift();
      if (!message) {
        continue;
      }
      this.debug(`Sending payload: ${message.payload}`);
      try {
        this.ws.send(message.payload);
      } catch (error) {
        this.debug(`Failed to send payload: ${(error as Error).message}`);
        if (message.reject) {
          message.reject(new JetBusError("Failed to send payload"));
        }
      }
    }
  }

  private handleMessage(text: string): void {
    const message = safeParseJson(text);
    if (!message) {
      return;
    }

    if (message.params && typeof message.params === "object") {
      const params = message.params as Record<string, unknown>;
      const path = typeof params.path === "string" ? params.path : "";
      const eventStr = typeof params.event === "string" ? params.event : "";
      let rawValue = "";
      if ("value" in params) {
        const value = params.value;
        rawValue =
          typeof value === "string" || typeof value === "number" || typeof value === "boolean"
            ? String(value)
            : JSON.stringify(value);
      }
      if (path) {
        this.cache.set(path, rawValue);
        this.emitter.emit("update", path, rawValue);
        this.dispatchWaiters(path, rawValue);
        if (this.dataCallback) {
          this.dataCallback(path, rawValue, eventTypeFromString(eventStr));
        }
      }
    }

    if (typeof message.id === "number") {
      const id = message.id as number;
      const token = this.pendingTokens.get(id);
      if (token) {
        this.pendingTokens.delete(id);
      }
      const pending = this.pendingPromises.get(id);
      if (pending) {
        this.pendingPromises.delete(id);
        if (pending.timeout) clearTimeout(pending.timeout);
        pending.resolve?.(message);
      }
      if (token && this.fetchCallback) {
        const success = !("error" in message);
        this.fetchCallback(success, token);
      }
    }
  }

  private dispatchWaiters(path: string, value: string): void {
    const waiters = this.waiters.get(path);
    if (!waiters) {
      return;
    }
    for (const waiter of [...waiters]) {
      try {
        if (waiter.predicate(value)) {
          clearTimeout(waiter.timer);
          waiters.delete(waiter);
          waiter.resolve(true);
        }
      } catch {
        // ignore predicate errors
      }
    }
    if (waiters.size === 0) {
      this.waiters.delete(path);
    }
  }
}

function safeParseJson(text: string): Record<string, unknown> | null {
  try {
    const parsed = JSON.parse(text);
    if (parsed && typeof parsed === "object") {
      return parsed as Record<string, unknown>;
    }
  } catch {
    return null;
  }
  return null;
}
