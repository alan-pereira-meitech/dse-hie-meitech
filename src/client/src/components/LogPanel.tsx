import { useEffect, useMemo, useRef, useState, type CSSProperties } from "react";
import { useJetBus } from "../context/JetBusContext";
import type { LogEntry } from "../types/log";
import { cn } from "../utils/cn";

interface LogPanelProps {
  className?: string;
  style?: React.CSSProperties;
}

const HIGHLIGHT_DURATION = 2000;
const PANEL_MAX_HEIGHT = "min(100%, calc(100vh - 220px))";

function LogPanel({ className, style }: LogPanelProps): JSX.Element {
  const { logs, clearLogs, latestLogId } = useJetBus();
  const containerRef = useRef<HTMLDivElement>(null);
  const [autoScroll, setAutoScroll] = useState(true);
  const [userPaused, setUserPaused] = useState(false);
  const [highlightId, setHighlightId] = useState<string | null>(null);

  const combinedStyle = useMemo<CSSProperties>(() => ({
    maxHeight: PANEL_MAX_HEIGHT,
    ...style
  }), [style]);

  const visibleLogs = useMemo(() => logs.slice().reverse(), [logs]);

  useEffect(() => {
    if (!latestLogId) return;
    setHighlightId(latestLogId);
    if (autoScroll) {
      requestAnimationFrame(() => {
        if (containerRef.current) {
          containerRef.current.scrollTo({ top: containerRef.current.scrollHeight, behavior: "smooth" });
        }
      });
    }
    const timeout = window.setTimeout(() => setHighlightId(null), HIGHLIGHT_DURATION);
    return () => window.clearTimeout(timeout);
  }, [latestLogId, autoScroll]);

  useEffect(() => {
    if (!autoScroll) {
      return;
    }
    const container = containerRef.current;
    if (!container) {
      return;
    }
    container.scrollTop = container.scrollHeight;
  }, [visibleLogs, autoScroll]);

  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const handleScroll = () => {
      if (!container) return;
      const { scrollTop, scrollHeight, clientHeight } = container;
      const atBottom = scrollHeight - (scrollTop + clientHeight) < 80;
      if (userPaused) {
        return;
      }
      setAutoScroll(atBottom);
    };

    container.addEventListener("scroll", handleScroll);
    return () => container.removeEventListener("scroll", handleScroll);
  }, [userPaused]);

  const handleCopy = () => {
    const text = visibleLogs
      .map((log) => `[${log.level.toUpperCase()}] ${log.timestamp} - ${log.title}: ${log.message}`)
      .join("\n");
    void navigator.clipboard.writeText(text);
  };

  const handleResumeAutoScroll = () => {
    setUserPaused(false);
    setAutoScroll(true);
    if (containerRef.current) {
      containerRef.current.scrollTo({ top: containerRef.current.scrollHeight, behavior: "smooth" });
    }
  };

  const handlePauseAutoScroll = () => {
    setUserPaused(true);
    setAutoScroll(false);
  };

  return (
    <section
      className={cn(
        "panel flex h-full min-h-0 flex-col gap-4 overflow-hidden border-slate-500/40 bg-slate-900/70",
        className
      )}
      style={combinedStyle}
    >
      <div className="flex shrink-0 flex-wrap items-center justify-between gap-3">
        <div>
          <h2 className="text-xl font-semibold text-slate-100">Logs</h2>
          <p className="text-sm text-slate-400">Retornos das ações executadas em tempo real.</p>
        </div>
        <div className="flex flex-wrap gap-2">
          <button
            type="button"
            onClick={clearLogs}
            className="rounded-2xl border border-slate-500/40 bg-slate-900/80 px-4 py-2 text-sm text-slate-200 hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-slate-200"
          >
            Limpar
          </button>
          <button
            type="button"
            onClick={handleCopy}
            className="rounded-2xl border border-slate-500/40 bg-slate-900/80 px-4 py-2 text-sm text-slate-200 hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-slate-200"
          >
            Copiar
          </button>
          {autoScroll ? (
            <button
              type="button"
              onClick={handlePauseAutoScroll}
              className="rounded-2xl border border-slate-500/40 bg-slate-900/80 px-4 py-2 text-sm text-slate-200 hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-slate-200"
            >
              Pausar autoscroll
            </button>
          ) : (
            <button
              type="button"
              onClick={handleResumeAutoScroll}
              className="rounded-2xl border border-sky-400/40 bg-sky-500/20 px-4 py-2 text-sm font-semibold text-sky-200 hover:-translate-y-0.5 focus-visible:outline focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-sky-300"
            >
              Retomar autoscroll
            </button>
          )}
        </div>
      </div>
      <div
        ref={containerRef}
        role="log"
        aria-live="polite"
        className="flex-1 min-h-0 overflow-y-auto rounded-2xl border border-slate-500/30 bg-slate-950/30 px-4 py-4"
      >
        {visibleLogs.length === 0 ? (
          <p className="text-sm text-slate-400">Nenhuma ação executada ainda.</p>
        ) : (
          <ul className="space-y-4 text-sm">
            {visibleLogs.map((log) => {
              const isHighlighted = highlightId === log.id;
              return (
                <li
                  key={log.id}
                  className={cn(
                    "rounded-2xl border border-transparent bg-slate-900/80 p-4 text-slate-200 transition",
                    log.level === "success" && "border-emerald-500/30 text-emerald-100",
                    log.level === "error" && "border-rose-500/30 text-rose-100",
                    log.level === "info" && "border-sky-500/30 text-sky-100",
                    isHighlighted && "ring-2 ring-sky-400/80"
                  )}
                >
                  <div className="flex items-center justify-between gap-2 text-xs text-slate-400">
                    <span className="uppercase tracking-wide">{log.level}</span>
                    <time>{new Date(log.timestamp).toLocaleTimeString()}</time>
                  </div>
                  <p className="mt-2 font-semibold text-slate-100">{log.title}</p>
                  <p className="text-sm text-slate-200">{log.message}</p>
                </li>
              );
            })}
          </ul>
        )}
      </div>
    </section>
  );
}

export default LogPanel;
