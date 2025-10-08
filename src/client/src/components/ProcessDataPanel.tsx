import { Link } from "react-router-dom";
import { useJetBus } from "../context/JetBusContext";
import { cn } from "../utils/cn";

interface ProcessDataPanelProps {
  className?: string;
  density?: "comfortable" | "spacious";
  cardCols?: {
    base?: number;
    xl?: number;
  };
  showSettingsCta?: boolean;
}

const DENSITY_MAP = {
  comfortable: {
    gap: "gap-4",
    badge: "px-4 py-2 text-sm",
    value: "text-3xl",
    helper: "text-sm"
  },
  spacious: {
    gap: "gap-5",
    badge: "px-5 py-2.5 text-base",
    value: "text-4xl",
    helper: "text-base"
  }
} as const;

function ProcessDataPanel({
  className,
  density = "spacious",
  cardCols = { base: 2, xl: 4 },
  showSettingsCta = false
}: ProcessDataPanelProps): JSX.Element {
  const { state } = useJetBus();
  const snapshot = state.snapshot;
  const styles = DENSITY_MAP[density];

  const metrics = [
    {
      label: "Net",
      value: snapshot?.printableWeight.net ?? "—",
      helper: snapshot?.unit ? `${snapshot.printableWeight.net} ${snapshot.unit}` : snapshot?.printableWeight.net ?? "—"
    },
    {
      label: "Gross",
      value: snapshot?.printableWeight.gross ?? "—",
      helper: snapshot?.unit ? `${snapshot.printableWeight.gross} ${snapshot.unit}` : snapshot?.printableWeight.gross ?? "—"
    },
    {
      label: "Tare",
      value: snapshot?.printableWeight.tare ?? "—",
      helper: snapshot?.unit ? `${snapshot.printableWeight.tare} ${snapshot.unit}` : snapshot?.printableWeight.tare ?? "—"
    },
    {
      label: "Unidade",
      value: snapshot?.unit ?? "—",
      helper: `Decimais: ${snapshot?.decimals ?? 0}`
    }
  ];

  const statusEntries = [
    { label: `Modo de tara: ${snapshot?.tareMode ?? "Nenhum"}`, active: snapshot?.tareMode && snapshot.tareMode !== "Nenhum" && snapshot.tareMode !== "None" },
    { label: "Peso estável", active: Boolean(snapshot?.status.weightStable) },
    { label: "Requer zero", active: Boolean(snapshot?.status.zeroRequired) },
    { label: "Centro de zero", active: Boolean(snapshot?.status.centerOfZero) },
    { label: "Dentro da zona zero", active: Boolean(snapshot?.status.insideZero) },
    { label: "Pronto para uso comercial", active: Boolean(snapshot?.status.legalForTrade) },
    { label: "Sobrecarga", active: Boolean(snapshot?.status.overload) },
    { label: "Subcarga", active: Boolean(snapshot?.status.underload) },
    { label: "Limite seguro elevado", active: Boolean(snapshot?.status.higherSafeLoadLimit) },
    { label: "Erro geral", active: Boolean(snapshot?.status.generalScaleError) },
    { label: "Alarme de balança", active: Boolean(snapshot?.status.scaleAlarm) }
  ];

  return (
    <section className={cn("w-full", className)}>
      <div className="panel flex h-full flex-col gap-6">
        <div className="flex flex-wrap items-start justify-between gap-4">
          <div>
            <h2 className="text-xl font-semibold text-slate-100">Dados de Processo</h2>
            <p className="text-sm text-slate-300">Acompanhe os valores em tempo real com atualização automática.</p>
          </div>
        </div>
        <div
          className={cn(
            "grid",
            styles.gap,
            cardCols.base === 3 ? "grid-cols-1 md:grid-cols-2 xl:grid-cols-3" : "grid-cols-1 md:grid-cols-2",
            cardCols.xl === 4 ? "xl:grid-cols-4" : "xl:grid-cols-3"
          )}
        >
          {metrics.map((metric) => (
            <div key={metric.label} className="rounded-2xl border border-slate-500/30 bg-slate-900/70 p-6 shadow-inner shadow-slate-900/30">
              <span className="text-sm text-slate-400">{metric.label}</span>
              <p className={cn("font-semibold text-slate-100", styles.value)}>{metric.value}</p>
              <p className={cn("text-slate-400", styles.helper)}>{metric.helper}</p>
            </div>
          ))}
        </div>
        <div className={cn("flex flex-wrap", styles.gap)}>
          {statusEntries.map((entry) => (
            <span
              key={entry.label}
              className={cn(
                "status-pill",
                styles.badge,
                entry.active ? "active" : "bg-slate-800/80 text-slate-400"
              )}
            >
              {entry.label}
            </span>
          ))}
        </div>
      </div>
    </section>
  );
}

export default ProcessDataPanel;
