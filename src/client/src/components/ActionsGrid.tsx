import { FormEvent, useEffect, useMemo, useRef, useState } from "react";
import { useJetBus } from "../context/JetBusContext";
import type { DeviceActionDefinition } from "../types/device";
import type { LogEntry } from "../types/log";
import { cn } from "../utils/cn";
import { useDebouncedValue } from "../hooks/useDebouncedValue";

type ActionCategory = "Básicos" | "Calibração" | "Parâmetros" | "Leituras" | "Outros";

interface ActionMetadata {
  category: ActionCategory;
  tags: string[];
  description?: string;
}

const ACTION_METADATA: Record<string, ActionMetadata> = {
  zero: { category: "Básicos", tags: ["zero", "tare", "baseline"], description: "Zera a balança imediatamente." },
  tare: { category: "Básicos", tags: ["tare", "peso líquido"], description: "Aplica a tara automática." },
  setGross: { category: "Básicos", tags: ["gross", "bruto"], description: "Alterna a leitura para peso bruto." },
  recordWeight: { category: "Básicos", tags: ["registro", "log"], description: "Registra o peso atual na memória." },
  adjustZeroSignal: { category: "Calibração", tags: ["calibração", "zero"], description: "Calibra o sinal de zero." },
  adjustNominalSignal: { category: "Calibração", tags: ["calibração", "nominal"], description: "Calibra o sinal nominal." },
  adjustNominalSignalWithCalibrationWeight: {
    category: "Calibração",
    tags: ["calibração", "peso", "nominal"],
    description: "Ajusta o sinal nominal usando um peso de calibração informado."
  },
  calculateAdjustment: {
    category: "Calibração",
    tags: ["ajuste", "calibração", "mvv"],
    description: "Calcula os ajustes a partir dos valores em mV/V."
  },
  setUnit: {
    category: "Parâmetros",
    tags: ["unidade", "kg", "lb"],
    description: "Define a unidade de medição (kg, g, lb, etc.)."
  },
  setManualTare: {
    category: "Parâmetros",
    tags: ["tare", "manual"],
    description: "Define um valor manual de tara."
  },
  setMaximumCapacity: {
    category: "Parâmetros",
    tags: ["capacidade", "escala"],
    description: "Ajusta a capacidade máxima da balança."
  },
  setZeroSignal: {
    category: "Parâmetros",
    tags: ["zero", "sinal"],
    description: "Define manualmente o sinal de zero."
  },
  setNominalSignal: {
    category: "Parâmetros",
    tags: ["nominal", "sinal"],
    description: "Define manualmente o sinal nominal."
  },
  saveAllParameters: { category: "Parâmetros", tags: ["salvar", "persistir"], description: "Salva todos os parâmetros na balança." },
  restoreDefaultParameters: {
    category: "Parâmetros",
    tags: ["reset", "factory"],
    description: "Restaura os valores padrão de fábrica."
  },
  serialNumber: { category: "Leituras", tags: ["identificação", "serial"], description: "Lê o número de série." },
  identification: { category: "Leituras", tags: ["identificação", "modelo"], description: "Obtém a identificação textual do dispositivo." },
  firmwareVersion: { category: "Leituras", tags: ["firmware", "versão"], description: "Consulta a versão de firmware." },
  weightStep: { category: "Leituras", tags: ["step", "resolução"], description: "Retorna o passo de peso configurado." },
  scaleRange: { category: "Leituras", tags: ["escala", "range"], description: "Lê o range de escala configurado." },
  maximumCapacity: { category: "Leituras", tags: ["capacidade", "máximo"], description: "Consulta a capacidade máxima." },
  zeroValue: { category: "Leituras", tags: ["zero", "valor"], description: "Lê o valor atual de zero." },
  zeroSignal: { category: "Leituras", tags: ["zero", "sinal"], description: "Lê o sinal de zero." },
  nominalSignal: { category: "Leituras", tags: ["nominal", "sinal"], description: "Lê o sinal nominal." }
};

const CATEGORY_ORDER: Record<ActionCategory, number> = {
  "Básicos": 0,
  "Calibração": 1,
  "Parâmetros": 2,
  "Leituras": 3,
  "Outros": 4
};

const PANEL_MAX_HEIGHT = "min(100%, calc(100vh - 220px))";

interface ActionsGridProps {
  className?: string;
}

type ActionExecutionState = {
  status: "idle" | "loading" | "success" | "error";
  message?: string;
  logEntry?: LogEntry;
};

function ActionsGrid({ className }: ActionsGridProps): JSX.Element {
  const { actions, executeAction, state } = useJetBus();

  const [query, setQuery] = useState("");
  const debouncedQuery = useDebouncedValue(query, 250);
  const [category, setCategory] = useState<ActionCategory | "all">("all");
  const [sortBy, setSortBy] = useState<"category" | "alpha">("category");
  const [actionStates, setActionStates] = useState<Record<string, ActionExecutionState>>({});
  const resetTimers = useRef<Record<string, ReturnType<typeof setTimeout>>>({});

  useEffect(() => () => {
    Object.values(resetTimers.current).forEach((timer) => clearTimeout(timer));
  }, []);

  const categories = useMemo(() => {
    const set = new Set<ActionCategory>();
    actions.forEach((action) => {
      const meta = ACTION_METADATA[action.id];
      if (meta) {
        set.add(meta.category);
      }
    });
    return ["all" as const, ...Array.from(set.values()).sort((a, b) => CATEGORY_ORDER[a] - CATEGORY_ORDER[b])];
  }, [actions]);

  const filteredActions = useMemo(() => {
    const normalizedQuery = debouncedQuery.trim().toLowerCase();
    return actions
      .filter((action) => {
        const meta = ACTION_METADATA[action.id] ?? { category: "Outros", tags: [] };
        if (category !== "all" && meta.category !== category) {
          return false;
        }
        if (!normalizedQuery) {
          return true;
        }
        const haystack = [action.label, action.id, ...(meta.tags ?? [])].join(" ").toLowerCase();
        return haystack.includes(normalizedQuery);
      })
      .sort((a, b) => {
        if (sortBy === "alpha") {
          return a.label.localeCompare(b.label);
        }
        const metaA = ACTION_METADATA[a.id] ?? { category: "Outros", tags: [] };
        const metaB = ACTION_METADATA[b.id] ?? { category: "Outros", tags: [] };
        const categoryDiff = CATEGORY_ORDER[metaA.category] - CATEGORY_ORDER[metaB.category];
        if (categoryDiff !== 0) {
          return categoryDiff;
        }
        return a.label.localeCompare(b.label);
      });
  }, [actions, category, debouncedQuery, sortBy]);

  const handleSubmit = async (event: FormEvent<HTMLFormElement>, action: DeviceActionDefinition) => {
    event.preventDefault();
    const form = event.currentTarget;
    const formData = new FormData(form);
    const payload: Record<string, unknown> = {};
    let validationError = "";

    action.parameters?.forEach((parameter) => {
      const raw = formData.get(parameter.name);
      if (parameter.type === "number") {
        const value = Number(raw);
        if (!Number.isFinite(value)) {
          validationError = `${parameter.label} inválido`;
        } else {
          payload[parameter.name] = value;
        }
      } else {
        const value = String(raw ?? "").trim();
        if (!value) {
          validationError = `${parameter.label} não pode ficar vazio`;
        } else {
          payload[parameter.name] = value;
        }
      }
    });

    if (validationError) {
      setActionStates((prev) => ({
        ...prev,
        [action.id]: { status: "error", message: validationError }
      }));
      return;
    }

    setActionStates((prev) => ({ ...prev, [action.id]: { status: "loading" } }));

    try {
      await executeAction(action.id, payload);
      const msg = "Comando executado com sucesso";
      setActionStates((prev) => ({
        ...prev,
        [action.id]: { status: "success", message: msg }
      }));
    } catch (error) {
      const err = error as Error & { logEntry?: LogEntry };
      setActionStates((prev) => ({
        ...prev,
        [action.id]: {
          status: "error",
          message: err.message,
          logEntry: err.logEntry
        }
      }));
    } finally {
      if (resetTimers.current[action.id]) {
        clearTimeout(resetTimers.current[action.id]);
      }
      resetTimers.current[action.id] = setTimeout(() => {
        setActionStates((prev) => ({ ...prev, [action.id]: { status: "idle" } }));
      }, 5500);
      form.reset();
    }
  };

  return (
    <section className={cn("flex h-full min-h-0 flex-col", className)}>
      <div
        className="panel flex h-full min-h-0 flex-col gap-6 overflow-hidden"
        style={{ maxHeight: PANEL_MAX_HEIGHT }}
      >
        <div className="flex shrink-0 flex-wrap items-end gap-4 rounded-2xl border border-slate-500/30 bg-slate-900/70 p-4">
          <label className="flex flex-1 flex-col gap-2 text-sm text-slate-300">
            Busca
            <input
              type="search"
              placeholder="Procurar comando"
              value={query}
              onChange={(event) => setQuery(event.target.value)}
              className="h-14 rounded-2xl border border-slate-500/40 bg-slate-900/80 px-4 text-base text-slate-100 shadow-inner shadow-slate-900/40 focus-visible:outline focus-visible:outline-2 focus-visible:outline-sky-400"
            />
          </label>
          <label className="flex flex-col gap-2 text-sm text-slate-300">
            Categoria
            <select
              value={category}
              onChange={(event) => setCategory(event.target.value as ActionCategory | "all")}
              className="h-14 w-56 rounded-2xl border border-slate-500/40 bg-slate-900/80 px-4 text-base text-slate-100 focus-visible:outline focus-visible:outline-2 focus-visible:outline-sky-400"
            >
              {categories.map((cat) => (
                <option key={cat} value={cat}>
                  {cat === "all" ? "Todas as categorias" : cat}
                </option>
              ))}
            </select>
          </label>
          <label className="flex flex-col gap-2 text-sm text-slate-300">
            Ordenar
            <select
              value={sortBy}
              onChange={(event) => setSortBy(event.target.value as "category" | "alpha")}
              className="h-14 w-44 rounded-2xl border border-slate-500/40 bg-slate-900/80 px-4 text-base text-slate-100 focus-visible:outline focus-visible:outline-2 focus-visible:outline-sky-400"
            >
              <option value="category">Por categoria</option>
              <option value="alpha">A–Z</option>
            </select>
          </label>
        </div>

        <div className="grid flex-1 min-h-0 grid-cols-1 gap-5 overflow-y-auto pr-2 md:grid-cols-2 2xl:grid-cols-3">
          {filteredActions.map((action) => {
            const meta = ACTION_METADATA[action.id] ?? { category: "Outros", tags: [] };
            const currentState = actionStates[action.id] ?? { status: "idle" };
            const isLoading = currentState.status === "loading";

            return (
              <form
                key={action.id}
                onSubmit={(event) => {
                  void handleSubmit(event, action);
                }}
                aria-busy={isLoading}
                className={cn(
                  "flex h-full flex-col justify-between gap-4 rounded-2xl border border-slate-500/30 bg-slate-900/70 p-5 text-slate-200 transition-colors",
                  currentState.status === "success" && "border-emerald-400/40",
                  currentState.status === "error" && "border-rose-400/50"
                )}
              >
                <div className="flex flex-col gap-3">
                  <div className="flex items-start justify-between gap-2">
                    <div>
                      <h3 className="text-lg font-semibold text-slate-100">{action.label}</h3>
                      <p className="text-sm text-slate-400">{meta.description}</p>
                    </div>
                    <span className="rounded-full border border-slate-500/40 px-3 py-1 text-xs uppercase tracking-wide text-slate-300">
                      {meta.category}
                    </span>
                  </div>

                  {action.parameters?.map((parameter) => (
                    <label key={parameter.name} className="flex flex-col gap-2 text-sm text-slate-300">
                      {parameter.label}
                      <input
                        name={parameter.name}
                        placeholder={parameter.placeholder ?? ""}
                        type={parameter.type === "number" ? "number" : "text"}
                        required
                        disabled={isLoading || !state.connected}
                        className="h-14 rounded-2xl border border-slate-500/40 bg-slate-900/80 px-4 text-base text-slate-100 shadow-inner shadow-slate-900/40 focus-visible:outline focus-visible:outline-2 focus-visible:outline-sky-400 disabled:cursor-not-allowed disabled:opacity-40"
                      />
                    </label>
                  ))}
                </div>

                <div className="flex flex-col gap-3">
                  {currentState.status === "loading" && (
                    <div className="flex items-center gap-2 text-sm text-sky-300" role="status" aria-live="polite">
                      <span className="h-3 w-3 animate-spin rounded-full border-2 border-current border-t-transparent" />
                      Executando comando...
                    </div>
                  )}

                  {currentState.status === "success" && currentState.message && (
                    <div className="rounded-2xl border border-emerald-500/40 bg-emerald-500/10 px-4 py-3 text-sm text-emerald-200" aria-live="polite">
                      {currentState.message}
                    </div>
                  )}

                  {currentState.status === "error" && currentState.message && (
                    <div className="rounded-2xl border border-rose-500/40 bg-rose-500/10 px-4 py-3 text-sm text-rose-200" aria-live="assertive">
                      {currentState.message}
                    </div>
                  )}

                  <button
                    type="submit"
                    disabled={isLoading || !state.connected}
                    className="h-14 rounded-2xl bg-gradient-to-r from-sky-400 to-indigo-500 text-base font-semibold text-slate-900 shadow-brand-lg transition-transform hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-sky-300 disabled:cursor-not-allowed disabled:opacity-40"
                  >
                    {action.returnsValue ? "Consultar" : "Executar"}
                  </button>
                </div>
              </form>
            );
          })}

          {filteredActions.length === 0 && (
            <div className="flex flex-col items-center justify-center rounded-2xl border border-slate-500/30 bg-slate-900/70 p-10 text-center text-slate-300">
              <p className="text-lg font-semibold">Nenhum comando encontrado</p>
              <p className="mt-2 text-sm">Ajuste a busca ou selecione outra categoria.</p>
            </div>
          )}
        </div>
      </div>
    </section>
  );
}

export default ActionsGrid;
