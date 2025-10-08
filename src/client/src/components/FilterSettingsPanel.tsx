import { FormEvent, useCallback, useEffect, useRef, useState } from "react";
import { useJetBus } from "../context/JetBusContext";
import type { FilterMode, FilterStageConfig, FiltersResponse } from "../types/filters";
import { cn } from "../utils/cn";

const FILTER_OPTIONS: Array<{ value: FilterMode; label: string }> = [
  { value: "NoFilter", label: "Sem filtro" },
  { value: "FIRCombFilter", label: "FIR Comb Filter" },
  { value: "FIRMovingAverage", label: "FIR Moving Average" }
];

const MODE_DESCRIPTIONS: Record<FilterMode, string> = {
  NoFilter: "Desativa o filtro digital neste estágio.",
  FIRCombFilter: "Remove componentes periódicas do sinal com um filtro comb FIR.",
  FIRMovingAverage: "Suaviza ruídos de alta frequência com média móvel FIR."
};

const STAGES = [2, 3, 4, 5] as const;
type Stage = (typeof STAGES)[number];

type StageFormState = {
  mode: FilterMode;
  cutOffFrequency: string;
};

type StageStatus = {
  state: "idle" | "loading" | "success" | "error";
  message?: string;
};

type StageFormMap = Partial<Record<Stage, StageFormState>>;
type StageStatusMap = Partial<Record<Stage, StageStatus>>;
type StageTimeoutMap = Partial<Record<Stage, ReturnType<typeof setTimeout>>>;

async function requestJson<T>(input: RequestInfo | URL, init?: RequestInit): Promise<T> {
  const response = await fetch(input, init);
  let payload: unknown;
  try {
    payload = await response.json();
  } catch {
    payload = undefined;
  }
  if (!response.ok) {
    const message =
      payload && typeof payload === "object" && payload !== null && "error" in payload
        ? String((payload as { error?: string }).error ?? `Erro HTTP ${response.status}`)
        : `Erro HTTP ${response.status}`;
    throw new Error(message);
  }
  return (payload ?? {}) as T;
}

function mapStagesToForm(stages: FilterStageConfig[]): StageFormMap {
  const map: StageFormMap = {};
  STAGES.forEach((stage) => {
    const entry = stages.find((candidate) => candidate.stage === stage);
    if (entry) {
      map[stage] = {
        mode: entry.mode,
        cutOffFrequency: String(entry.cutOffFrequency ?? 0)
      };
    } else {
      map[stage] = {
        mode: "NoFilter",
        cutOffFrequency: "0"
      };
    }
  });
  return map;
}

interface FilterSettingsPanelProps {
  className?: string;
}

function FilterSettingsPanel({ className }: FilterSettingsPanelProps): JSX.Element {
  const { notify, state } = useJetBus();
  const connected = Boolean(state.connected);
  const [loading, setLoading] = useState<boolean>(false);
  const [formState, setFormState] = useState<StageFormMap>({});
  const [statuses, setStatuses] = useState<StageStatusMap>({});
  const [error, setError] = useState<string | null>(null);
  const resetTimers = useRef<StageTimeoutMap>({});

  const clearStageTimer = useCallback((stage: Stage) => {
    const timer = resetTimers.current[stage];
    if (timer) {
      clearTimeout(timer);
      delete resetTimers.current[stage];
    }
  }, []);

  const scheduleStatusReset = useCallback((stage: Stage) => {
    clearStageTimer(stage);
    resetTimers.current[stage] = setTimeout(() => {
      setStatuses((prev) => {
        const next = { ...prev };
        delete next[stage];
        return next;
      });
      delete resetTimers.current[stage];
    }, 4000);
  }, [clearStageTimer]);

  useEffect(() => () => {
    STAGES.forEach((stage) => {
      const timer = resetTimers.current[stage];
      if (timer) {
        clearTimeout(timer);
      }
    });
  }, []);

  const applyConfiguration = useCallback((stages: FilterStageConfig[]) => {
    setFormState(mapStagesToForm(stages));
  }, []);

  const fetchFilters = useCallback(async () => {
    if (!connected) {
      setFormState({});
      setStatuses({});
      setError(null);
      setLoading(false);
      return;
    }
    setLoading(true);
    setError(null);
    try {
      const response = await requestJson<FiltersResponse>("/api/filters");
      applyConfiguration(response.stages);
      setStatuses({});
    } catch (err) {
      const message = err instanceof Error ? err.message : "Não foi possível carregar os filtros.";
      setError(message);
      notify("error", "Filtros digitais", message);
    } finally {
      setLoading(false);
    }
  }, [applyConfiguration, connected, notify]);

  useEffect(() => {
    void fetchFilters();
  }, [fetchFilters]);

  const handleModeChange = useCallback((stage: Stage, value: FilterMode) => {
    setFormState((prev) => {
      const current = prev[stage] ?? { mode: value, cutOffFrequency: value === "NoFilter" ? "0" : "" };
      return {
        ...prev,
        [stage]: {
          mode: value,
          cutOffFrequency: value === "NoFilter" ? "0" : current.cutOffFrequency || "0"
        }
      };
    });
  }, []);

  const handleFrequencyChange = useCallback((stage: Stage, value: string) => {
    setFormState((prev) => {
      const current = prev[stage] ?? { mode: "NoFilter", cutOffFrequency: "0" };
      return {
        ...prev,
        [stage]: {
          mode: current.mode,
          cutOffFrequency: value
        }
      };
    });
  }, []);

  const handleSubmit = useCallback(
    async (event: FormEvent<HTMLFormElement>, stage: Stage) => {
      event.preventDefault();
      if (!connected) {
        return;
      }
      const form = formState[stage] ?? { mode: "NoFilter", cutOffFrequency: "0" };
      if (form.mode !== "NoFilter") {
        const trimmed = form.cutOffFrequency.trim();
        if (trimmed.length === 0) {
          setStatuses((prev) => ({
            ...prev,
            [stage]: { state: "error", message: "Informe a frequência de corte." }
          }));
          scheduleStatusReset(stage);
          return;
        }
        const numeric = Number(trimmed);
        if (!Number.isFinite(numeric) || numeric < 0) {
          setStatuses((prev) => ({
            ...prev,
            [stage]: { state: "error", message: "Frequência inválida." }
          }));
          scheduleStatusReset(stage);
          return;
        }
      }

      const frequencyValue = form.mode === "NoFilter" ? 0 : Number(form.cutOffFrequency.trim());
      if (form.mode !== "NoFilter" && (!Number.isFinite(frequencyValue) || frequencyValue < 0)) {
        setStatuses((prev) => ({
          ...prev,
          [stage]: { state: "error", message: "Frequência inválida." }
        }));
        scheduleStatusReset(stage);
        return;
      }

      clearStageTimer(stage);
      setStatuses((prev) => ({ ...prev, [stage]: { state: "loading" } }));

      try {
        const response = await requestJson<FiltersResponse>("/api/filters", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            stage,
            mode: form.mode,
            cutOffFrequency: frequencyValue
          })
        });
        applyConfiguration(response.stages);
        setStatuses((prev) => ({
          ...prev,
          [stage]: { state: "success", message: "Configuração aplicada." }
        }));
        scheduleStatusReset(stage);
      } catch (err) {
        const message = err instanceof Error ? err.message : "Falha ao aplicar a configuração.";
        setStatuses((prev) => ({
          ...prev,
          [stage]: { state: "error", message }
        }));
        scheduleStatusReset(stage);
        notify("error", "Filtros digitais", message);
      }
    },
    [applyConfiguration, clearStageTimer, connected, formState, notify, scheduleStatusReset]
  );

  return (
    <section className={cn("w-full", className)}>
      <div className="panel flex h-full flex-col gap-6">
        <div className="flex flex-wrap items-start justify-between gap-4">
          <div>
            <h2 className="text-xl font-semibold text-slate-100">Filtros Digitais</h2>
            <p className="text-sm text-slate-300">
              Configure o modo e a frequência de corte de cada estágio do processamento DSE.
            </p>
          </div>
          <button
            type="button"
            onClick={() => void fetchFilters()}
            disabled={!connected || loading}
            className={cn(
              "rounded-xl border border-slate-500/50 bg-slate-900/70 px-4 py-2 text-sm font-medium text-slate-200 transition",
              (!connected || loading) && "cursor-not-allowed opacity-40"
            )}
          >
            Atualizar
          </button>
        </div>

        {!connected && (
          <div className="rounded-xl border border-slate-500/40 bg-slate-900/70 px-4 py-3 text-sm text-slate-300">
            Conecte-se ao dispositivo para visualizar e alterar as configurações de filtro.
          </div>
        )}

        {error && (
          <div className="rounded-xl border border-rose-500/40 bg-rose-500/10 px-4 py-3 text-sm text-rose-200">
            {error}
          </div>
        )}

        {connected && loading && (
          <p className="text-sm text-slate-300">Carregando configurações atuais...</p>
        )}

        <div className="grid grid-cols-1 gap-5 md:grid-cols-2">
          {STAGES.map((stage) => {
            const form = formState[stage] ?? {
              mode: connected ? "NoFilter" : "NoFilter",
              cutOffFrequency: connected ? "0" : ""
            };
            const status = statuses[stage];
            const isApplying = status?.state === "loading";
            const disableControls = !connected || loading || isApplying;
            const frequencyDisabled = disableControls || form.mode === "NoFilter";
            const frequencyPlaceholder = form.mode === "NoFilter" ? "—" : "mHz";
            const frequencyValue = form.mode === "NoFilter" ? (connected ? "0" : "") : form.cutOffFrequency;

            return (
              <form
                key={stage}
                onSubmit={(event) => void handleSubmit(event, stage)}
                className="flex flex-col gap-4 rounded-2xl border border-slate-500/30 bg-slate-900/70 p-5 shadow-inner shadow-slate-900/30"
              >
                <div className="flex items-center justify-between gap-2">
                  <h3 className="text-lg font-semibold text-slate-100">Estágio {stage}</h3>
                  {status?.state === "success" && (
                    <span className="text-sm font-medium text-emerald-300">{status.message ?? "Configuração aplicada."}</span>
                  )}
                  {status?.state === "error" && (
                    <span className="text-sm font-medium text-rose-300">{status.message}</span>
                  )}
                  {status?.state === "loading" && (
                    <span className="text-sm font-medium text-slate-300">Aplicando...</span>
                  )}
                </div>

                <label className="flex flex-col gap-2">
                  <span className="text-sm font-medium text-slate-300">Tipo de filtro</span>
                  <select
                    value={form.mode}
                    onChange={(event) => handleModeChange(stage, event.target.value as FilterMode)}
                    disabled={disableControls}
                    className={cn(
                      "rounded-xl border border-slate-600/50 bg-slate-950/80 px-4 py-2 text-sm text-slate-100 shadow-inner focus:outline focus:outline-2 focus:outline-offset-2 focus:outline-sky-400",
                      disableControls && "cursor-not-allowed opacity-60"
                    )}
                  >
                    {FILTER_OPTIONS.map((option) => (
                      <option key={option.value} value={option.value}>
                        {option.label}
                      </option>
                    ))}
                  </select>
                  <span className="text-xs text-slate-400">{MODE_DESCRIPTIONS[form.mode]}</span>
                </label>

                <label className="flex flex-col gap-2">
                  <span className="text-sm font-medium text-slate-300">Frequência de corte (mHz)</span>
                  <input
                    type="number"
                    inputMode="numeric"
                    min={0}
                    step={1}
                    value={frequencyValue}
                    placeholder={frequencyPlaceholder}
                    onChange={(event) => handleFrequencyChange(stage, event.target.value)}
                    disabled={frequencyDisabled}
                    className={cn(
                      "rounded-xl border border-slate-600/50 bg-slate-950/80 px-4 py-2 text-sm text-slate-100 shadow-inner placeholder:text-slate-500 focus:outline focus:outline-2 focus:outline-offset-2 focus:outline-sky-400",
                      frequencyDisabled && "cursor-not-allowed opacity-60"
                    )}
                  />
                  <span className="text-xs text-slate-400">
                    Valores em milihertz. Use 0 para deixar o estágio sem corte adicional.
                  </span>
                </label>

                <div className="flex items-center justify-between gap-3">
                  <button
                    type="submit"
                    disabled={disableControls}
                    className={cn(
                      "rounded-xl bg-gradient-to-r from-sky-400 to-indigo-500 px-4 py-2 text-sm font-semibold text-slate-900 shadow-brand-lg transition-transform hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-sky-400",
                      disableControls && "cursor-not-allowed opacity-40"
                    )}
                  >
                    Aplicar alterações
                  </button>
                </div>
              </form>
            );
          })}
        </div>
      </div>
    </section>
  );
}

export default FilterSettingsPanel;
