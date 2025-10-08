import { Link } from "react-router-dom";
import { useJetBus } from "../context/JetBusContext";
import { formatTime } from "../utils/format";
import { cn } from "../utils/cn";

type ConnectionCardSize = "md" | "lg";

const SIZE_STYLES: Record<
  ConnectionCardSize,
  { wrapperGap: string; stackGap: string; button: string; status: string; text: string }
> = {
  md: {
    wrapperGap: "gap-4",
    stackGap: "gap-3",
    button: "h-12 px-5 text-base",
    status: "text-sm",
    text: "text-sm"
  },
  lg: {
    wrapperGap: "gap-6",
    stackGap: "gap-4",
    button: "h-14 px-6 text-lg",
    status: "text-base",
    text: "text-base"
  }
};

interface ConnectionCardProps {
  className?: string;
  size?: ConnectionCardSize;
  showSettingsLink?: boolean;
  settingsPath?: string; // caso seu route seja diferente de /actions
}


// Renderiza o status de conexão
function ConnectionStatus({ isConnected, styles }: { isConnected: boolean; styles: any }) {
  return (
    <span
      aria-live="polite"
      className={cn(
        "rounded-full px-4 py-1.5 font-medium",
        styles.status,
        isConnected ? "bg-emerald-400/20 text-emerald-300" : "bg-rose-400/20 text-rose-300"
      )}
    >
      {isConnected ? "Conectado" : "Desconectado"}
    </span>
  );
}

// Botão de conectar
function ConnectButton({ isBusy, isConnected, onConnect, styles }: {
  isBusy: boolean;
  isConnected: boolean;
  onConnect: () => void;
  styles: any;
}) {
  return (
    <button
      type="button"
      disabled={isBusy || isConnected}
      aria-busy={isBusy && !isConnected}
      onClick={onConnect}
      title="Estabelecer conexão com o dispositivo"
      className={cn(
        "rounded-2xl bg-gradient-to-r from-sky-400 to-indigo-500 font-semibold text-slate-900 shadow-brand-lg transition-transform hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-sky-400 disabled:cursor-not-allowed disabled:opacity-40",
        styles.button
      )}
    >
      {isBusy && !isConnected ? "Conectando..." : "Conectar"}
    </button>
  );
}

// Botão de desconectar
function DisconnectButton({ isBusy, isConnected, onDisconnect, styles }: {
  isBusy: boolean;
  isConnected: boolean;
  onDisconnect: () => void;
  styles: any;
}) {
  return (
    <button
      type="button"
      disabled={isBusy || !isConnected}
      aria-busy={isBusy && isConnected}
      onClick={onDisconnect}
      title="Encerrar sessão com o dispositivo"
      className={cn(
        "rounded-2xl border border-slate-500/50 bg-slate-900/70 font-semibold text-slate-100 transition-transform hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-slate-200 disabled:cursor-not-allowed disabled:opacity-40",
        styles.button
      )}
    >
      {isBusy && isConnected ? "Encerrando..." : "Desconectar"}
    </button>
  );
}

function ConnectionCard({
  className,
  size = "lg",
  showSettingsLink = false,
  settingsPath = "/actions"
}: ConnectionCardProps): JSX.Element {
  const { state, connect, disconnect, loading } = useJetBus();
  const styles = SIZE_STYLES[size];
  const isConnected = Boolean(state.connected);
  const isBusy = Boolean(loading);
  // const lastUpdate = state.snapshot?.timestamp ? formatTime(state.snapshot.timestamp) : "—";

  return (
    <section className={className}>
      <div className={cn("panel grid lg:grid-cols-[minmax(320px,360px)_minmax(0,1fr)]", styles.wrapperGap)}>
        {/* Coluna esquerda */}
        <div className={cn("flex flex-col", styles.stackGap)}>
          <div className="flex items-center justify-between gap-3">
            <h2 className="text-xl font-semibold text-slate-100">Conexão</h2>
            <ConnectionStatus isConnected={isConnected} styles={styles} />
          </div>
          <div className="flex flex-wrap items-center gap-3">
            <ConnectButton isBusy={isBusy} isConnected={isConnected} onConnect={connect} styles={styles} />
            <DisconnectButton isBusy={isBusy} isConnected={isConnected} onDisconnect={disconnect} styles={styles} />
          </div>
        </div>
      </div>
    </section>
  );
}

export default ConnectionCard;
