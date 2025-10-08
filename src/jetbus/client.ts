import EventEmitter from 'node:events';
import WebSocket from 'ws';

import { eventTypeFromString, JetBusError, JetEventType } from './types.js';

export interface JetBusOptions {
  url: string;
  enableReconnect?: boolean;
  reconnectInitialDelay?: number;
  reconnectMaxDelay?: number;
  requestTimeout?: number;
  enableDebugLogs?: boolean;
}

interface PendingRequest {
  id: number;
  payload: string;
  resolve: (value: unknown) => void;
  reject: (err: Error) => void;
  timeout?: NodeJS.Timeout;
}

interface Waiter {
  predicate: (value: string) => boolean;
  resolve: (result: boolean) => void;
  timeout?: NodeJS.Timeout;
}

type Snapshot = Map<string, string>;

type DataEvent = {
  path: string;
  value: string;
  event: JetEventType;
};

export declare interface JetBusClient {
  on(event: 'data', listener: (event: DataEvent) => void): this;
  on(event: 'fetch', listener: (token: string, success: boolean) => void): this;
  on(event: string, listener: (...args: unknown[]) => void): this;
}

export class JetBusClient extends EventEmitter {
  private readonly options: Required<JetBusOptions>;
  private ws?: WebSocket;
  private nextId = 1;
  private readonly pending = new Map<number, PendingRequest>();
  private readonly queue: PendingRequest[] = [];
  private readonly cache: Snapshot = new Map();
  private readonly waiters = new Map<string, Set<Waiter>>();
  private reconnectDelay: number;
  private closed = false;
  private connectingPromise?: Promise<void>;

  constructor(options: JetBusOptions) {
    super();
    if (!options.url) {
      throw new JetBusError('JetBusClient requires a non-empty URL');
    }
    this.options = {
      enableReconnect: true,
      reconnectInitialDelay: 500,
      reconnectMaxDelay: 30_000,
      requestTimeout: 5_000,
      enableDebugLogs: true,
      ...options
    };
    this.reconnectDelay = this.options.reconnectInitialDelay;
  }

  async connect(): Promise<void> {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      return;
    }
    if (this.connectingPromise) {
      return this.connectingPromise;
    }
    this.closed = false;
    this.connectingPromise = new Promise<void>((resolve, reject) => {
      if (this.options.enableDebugLogs) {
        console.debug(`[jetbus] connecting to ${this.options.url}`);
      }
      const ws = new WebSocket(this.options.url, 'jet', {
        perMessageDeflate: false,
        headers: {
          'User-Agent': '',
          Origin: ''
        }
      });

      const cleanup = () => {
        ws.removeAllListeners();
        this.connectingPromise = undefined;
      };

      ws.on('open', () => {
        this.ws = ws;
        this.reconnectDelay = this.options.reconnectInitialDelay;
        if (this.options.enableDebugLogs) {
          console.debug('[jetbus] websocket connected');
        }
        this.flushQueue();
        cleanup();
        resolve();
      });

      ws.on('message', (data) => {
        const text = typeof data === 'string' ? data : data.toString('utf8');
        this.handleMessage(text);
      });

      ws.on('close', () => {
        if (this.options.enableDebugLogs) {
          console.debug('[jetbus] websocket closed');
        }
        this.ws = undefined;
        this.rejectAllPending(new JetBusError('WebSocket connection closed'));
        if (!this.closed && this.options.enableReconnect) {
          this.scheduleReconnect();
        }
      });

      ws.on('error', (err) => {
        if (this.options.enableDebugLogs) {
          console.error('[jetbus] websocket error', err);
        }
        this.rejectAllPending(new JetBusError(`WebSocket error: ${String(err)}`));
        cleanup();
        reject(err instanceof Error ? err : new Error(String(err)));
      });
    });
    return this.connectingPromise;
  }

  async disconnect(): Promise<void> {
    this.closed = true;
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      await new Promise<void>((resolve) => {
        this.ws?.once('close', () => resolve());
        this.ws?.close();
      });
    } else if (this.ws && this.ws.readyState !== WebSocket.CLOSED) {
      this.ws.close();
    }
    this.ws = undefined;
    this.connectingPromise = undefined;
    this.rejectAllPending(new JetBusError('Client disconnected'));
  }

  async fetch(path: string): Promise<string> {
    const id = this.nextId++;
    const payload = JSON.stringify({
      id,
      method: 'fetch',
      params: {
        path: { equals: path },
        caseInsensitive: false,
        id
      }
    });
    const response = await this.enqueue(id, payload);
    const token = (response as any)?.result?.token ?? (response as any)?.result;
    if (!token) {
      throw new JetBusError(`Fetch request for path ${path} returned no token`);
    }
    this.emit('fetch', String(token), !(response as any)?.error);
    return String(token);
  }

  async unfetch(token: string): Promise<void> {
    const id = this.nextId++;
    const payload = JSON.stringify({
      id,
      method: 'unfetch',
      params: { token }
    });
    await this.enqueue(id, payload);
  }

  async set(path: string, value: unknown): Promise<void> {
    const id = this.nextId++;
    const payload = JSON.stringify({
      id,
      method: 'set',
      params: { path, value }
    });
    const response = await this.enqueue(id, payload);
    if ((response as any)?.error) {
      throw new JetBusError(`Set request failed for path ${path}`);
    }
  }

  readCached(path: string): string | undefined {
    return this.cache.get(path);
  }

  snapshot(): Snapshot {
    return new Map(this.cache);
  }

  waitFor(path: string, predicate: (value: string) => boolean, timeout: number): Promise<boolean> {
    const cached = this.cache.get(path);
    if (cached !== undefined && predicate(cached)) {
      return Promise.resolve(true);
    }
    return new Promise<boolean>((resolve) => {
      const waiter: Waiter = {
        predicate,
        resolve
      };
      if (timeout > 0) {
        waiter.timeout = setTimeout(() => {
          this.removeWaiter(path, waiter);
          resolve(false);
        }, timeout);
      }
      if (!this.waiters.has(path)) {
        this.waiters.set(path, new Set());
      }
      this.waiters.get(path)!.add(waiter);
    });
  }

  private async enqueue(id: number, payload: string): Promise<unknown> {
    await this.connect();
    return new Promise<unknown>((resolve, reject) => {
      const timeout = setTimeout(() => {
        this.pending.delete(id);
        reject(new JetBusError(`Request ${id} timed out`));
      }, this.options.requestTimeout);
      const pending: PendingRequest = { id, payload, resolve, reject, timeout };
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        this.sendPending(pending);
      } else {
        this.queue.push(pending);
      }
    });
  }

  private sendPending(pending: PendingRequest): void {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      this.queue.push(pending);
      return;
    }
    this.pending.set(pending.id, pending);
    this.ws.send(pending.payload, (err) => {
      if (err) {
        if (pending.timeout) {
          clearTimeout(pending.timeout);
        }
        this.pending.delete(pending.id);
        pending.reject(new JetBusError(`Failed to send request ${pending.id}: ${String(err)}`));
      }
    });
  }

  private flushQueue(): void {
    while (this.queue.length > 0) {
      const pending = this.queue.shift();
      if (pending) {
        this.sendPending(pending);
      }
    }
  }

  private rejectAllPending(error: JetBusError): void {
    for (const pending of this.pending.values()) {
      if (pending.timeout) {
        clearTimeout(pending.timeout);
      }
      pending.reject(error);
    }
    this.pending.clear();
    while (this.queue.length > 0) {
      const next = this.queue.shift();
      if (next?.timeout) {
        clearTimeout(next.timeout);
      }
      next?.reject(error);
    }
  }

  private scheduleReconnect(): void {
    if (this.closed) {
      return;
    }
    const delay = this.reconnectDelay;
    if (this.options.enableDebugLogs) {
      console.debug(`[jetbus] reconnecting in ${delay}ms`);
    }
    setTimeout(() => {
      if (this.closed) {
        return;
      }
      this.connect().catch((err) => {
        if (this.options.enableDebugLogs) {
          console.error('[jetbus] reconnect error', err);
        }
        this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.options.reconnectMaxDelay);
        this.scheduleReconnect();
      });
    }, delay);
    this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.options.reconnectMaxDelay);
  }

  private handleMessage(text: string): void {
    let message: any;
    try {
      message = JSON.parse(text);
    } catch (error) {
      if (this.options.enableDebugLogs) {
        console.warn('[jetbus] failed to parse message', error);
      }
      return;
    }

    if (message?.params && typeof message.params === 'object') {
      const path = message.params.path as string;
      if (path) {
        const valueField = message.params.value;
        let raw = '';
        if (typeof valueField === 'string') {
          raw = valueField;
        } else if (valueField !== undefined) {
          raw = JSON.stringify(valueField);
        }
        this.cache.set(path, raw);
        this.notifyWaiters(path, raw);
        const event = eventTypeFromString(message.params.event);
        this.emit('data', { path, value: raw, event });
        return;
      }
    }

    if (typeof message?.id === 'number') {
      const pending = this.pending.get(message.id);
      if (pending) {
        if (pending.timeout) {
          clearTimeout(pending.timeout);
        }
        this.pending.delete(message.id);
        pending.resolve(message);
      }
      return;
    }
  }

  private notifyWaiters(path: string, raw: string): void {
    const set = this.waiters.get(path);
    if (!set || set.size === 0) {
      return;
    }
    for (const waiter of Array.from(set)) {
      if (waiter.predicate(raw)) {
        if (waiter.timeout) {
          clearTimeout(waiter.timeout);
        }
        waiter.resolve(true);
        set.delete(waiter);
      }
    }
    if (set.size === 0) {
      this.waiters.delete(path);
    }
  }

  private removeWaiter(path: string, waiter: Waiter): void {
    const set = this.waiters.get(path);
    if (!set) {
      return;
    }
    set.delete(waiter);
    if (set.size === 0) {
      this.waiters.delete(path);
    }
  }
}

export type { DataEvent };
