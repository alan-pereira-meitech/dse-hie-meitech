import React, { createContext, useCallback, useContext, useEffect, useMemo, useState } from "react";
import type {
  ApiActionsResponse,
  DeviceActionDefinition,
  DeviceState,
  ProcessDataSnapshot
} from "../types/device";
import type { LogEntry, LogLevel } from "../types/log";

interface JetBusContextValue {
  state: DeviceState;
  actions: DeviceActionDefinition[];
  logs: LogEntry[];
  latestLogId: string | null;
  loading: boolean;
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  executeAction(actionId: string, payload: Record<string, unknown>): Promise<{ result?: unknown; logEntry: LogEntry }>;
  clearLogs(): void;
  notify(level: LogLevel, title: string, message: string): LogEntry;
}

const JetBusContext = createContext<JetBusContextValue | undefined>(undefined);

const LOG_LIMIT = 250;

function createLog(level: LogLevel, title: string, message: string): LogEntry {
  return {
    id: `${Date.now()}-${Math.random().toString(16).slice(2)}`,
    level,
    title,
    message,
    timestamp: new Date().toISOString()
  };
}

async function apiGet<T>(url: string): Promise<T> {
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`Erro HTTP ${response.status}`);
  }
  return response.json() as Promise<T>;
}

async function apiPost<T>(url: string, body?: unknown): Promise<T> {
  const response = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: body ? JSON.stringify(body) : undefined
  });
  if (!response.ok) {
    let errorMessage = `Erro HTTP ${response.status}`;
    try {
      const payload = (await response.json()) as { error?: string };
      if (payload.error) {
        errorMessage = payload.error;
      }
    } catch {
      // ignore errors ao interpretar resposta de erro
    }
    throw new Error(errorMessage);
  }
  return response.json() as Promise<T>;
}

export const JetBusProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [state, setState] = useState<DeviceState>({ connected: false, snapshot: null });
  const [actions, setActions] = useState<DeviceActionDefinition[]>([]);
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const [latestLogId, setLatestLogId] = useState<string | null>(null);
  const [loading, setLoading] = useState<boolean>(true);

  const appendLog = useCallback((entry: LogEntry): LogEntry => {
    setLogs((prev) => {
      const updated = [entry, ...prev];
      return updated.length > LOG_LIMIT ? updated.slice(0, LOG_LIMIT) : updated;
    });
    setLatestLogId(entry.id);
    return entry;
  }, []);

  const notify = useCallback(
    (level: LogLevel, title: string, message: string): LogEntry => {
      const entry = createLog(level, title, message);
      return appendLog(entry);
    },
    [appendLog]
  );

  const handleProcessData = useCallback((snapshot: ProcessDataSnapshot) => {
    setState((prev) => ({ ...prev, snapshot }));
  }, []);

  const handleStateUpdate = useCallback((next: DeviceState) => {
    setState(next);
  }, []);

  useEffect(() => {
    let cancelled = false;

    async function bootstrap() {
      try {
        const [actionsResponse, currentState] = await Promise.all([
          apiGet<ApiActionsResponse>("/api/actions").catch((error) => {
            notify("error", "Ações", (error as Error).message);
            return { actions: [] } satisfies ApiActionsResponse;
          }),
          apiGet<DeviceState>("/api/state")
        ]);
        if (cancelled) return;
        setActions(actionsResponse.actions ?? []);
        setState(currentState);
      } catch (error) {
        if (!cancelled) {
          notify("error", "Inicialização", (error as Error).message);
        }
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    }

    bootstrap();

    return () => {
      cancelled = true;
    };
  }, [notify]);

  useEffect(() => {
    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    const socket = new WebSocket(`${protocol}://${window.location.host}/ws/stream`);

    socket.addEventListener("message", (event) => {
      try {
        const message = JSON.parse(event.data) as { type: string; payload: unknown };
        if (message.type === "state") {
          handleStateUpdate(message.payload as DeviceState);
        }
        if (message.type === "processData") {
          handleProcessData(message.payload as ProcessDataSnapshot);
        }
      } catch (error) {
        notify("error", "WebSocket", (error as Error).message);
      }
    });

    socket.addEventListener("close", () => {
      notify("info", "WebSocket", "Conexão encerrada");
    });

    socket.addEventListener("error", () => {
      notify("error", "WebSocket", "Erro na conexão");
    });

    return () => {
      socket.close();
    };
  }, [handleProcessData, handleStateUpdate, notify]);

  const connect = useCallback(async () => {
    try {
      const nextState = await apiPost<DeviceState>("/api/connect");
      setState(nextState);
      notify("success", "Conexão", "Conectado ao dispositivo");
    } catch (error) {
      notify("error", "Conexão", (error as Error).message);
      throw error;
    }
  }, [notify]);

  const disconnect = useCallback(async () => {
    try {
      const nextState = await apiPost<DeviceState>("/api/disconnect");
      setState(nextState);
      notify("info", "Conexão", "Desconectado do dispositivo");
    } catch (error) {
      notify("error", "Conexão", (error as Error).message);
      throw error;
    }
  }, [notify]);

  const executeAction = useCallback(
    async (actionId: string, payload: Record<string, unknown>) => {
      try {
        const response = await apiPost<{ result?: unknown }>("/api/action", { action: actionId, payload });
        const { result } = response ?? {};
        const resultText = (() => {
          if (result === null || result === undefined) {
            return "Comando executado com sucesso";
          }
          if (typeof result === "string") {
            return result;
          }
          return JSON.stringify(result);
        })();
        const logEntry = notify("success", actionId, resultText);
        return { result, logEntry };
      } catch (error) {
        const err = error instanceof Error ? error : new Error(String(error));
        const logEntry = notify("error", actionId, err.message);
        throw Object.assign(err, { logEntry });
      }
    },
    [notify]
  );

  const clearLogs = useCallback(() => {
    setLogs([]);
    setLatestLogId(null);
  }, []);

  const value = useMemo<JetBusContextValue>(
    () => ({
      state,
      actions,
      logs,
      latestLogId,
      loading,
      connect,
      disconnect,
      executeAction,
      clearLogs,
      notify
    }),
    [actions, clearLogs, connect, disconnect, executeAction, latestLogId, loading, logs, notify, state]
  );

  return <JetBusContext.Provider value={value}>{children}</JetBusContext.Provider>;
};

export function useJetBus(): JetBusContextValue {
  const context = useContext(JetBusContext);
  if (!context) {
    throw new Error("useJetBus deve ser utilizado dentro de JetBusProvider");
  }
  return context;
}
