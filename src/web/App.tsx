import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { connectDevice, disconnectDevice, fetchFunctions, fetchStatus, invokeFunction } from './api.js';
import { FunctionCard } from './components/FunctionCard.js';
import { DeviceFunctionMeta, DeviceSnapshot, ProcessDataSnapshot, WeightStreamPayload } from './types.js';
import { useWeightStream } from './useWeightStream.js';
import './styles.css';

const DEFAULT_URL = 'ws://127.0.0.1/jet/canopen';

const EMPTY_WEIGHT: WeightStreamPayload = {
  type: 'weight',
  connected: false,
  stable: false
};

function formatStatusText(stable: boolean): string {
  return stable ? 'peso estável' : 'peso em movimento';
}

function formatValue(value?: string): string {
  return value ?? '—';
}

function buildProcessEntries(data: ProcessDataSnapshot) {
  return [
    { label: 'Tare', value: `${data.printableWeight.tare} ${data.unit}` },
    { label: 'Unidade', value: data.unit || '—' },
    { label: 'Decimais', value: data.decimals },
    { label: 'Modo de Tara', value: data.tareMode },
    { label: 'Peso Estável', value: data.weightStable ? 'Sim' : 'Não' },
    { label: 'Zero Requerido', value: data.zeroRequired ? 'Sim' : 'Não' },
    { label: 'Centro do Zero', value: data.centerOfZero ? 'Sim' : 'Não' },
    { label: 'Dentro do Zero', value: data.insideZero ? 'Sim' : 'Não' },
    { label: 'Legal para Uso', value: data.legalForTrade ? 'Sim' : 'Não' },
    { label: 'Subcarga', value: data.underload ? 'Sim' : 'Não' },
    { label: 'Sobrecarga', value: data.overload ? 'Sim' : 'Não' },
    { label: 'Limite Seguro', value: data.higherSafeLoadLimit ? 'Acima' : 'OK' },
    { label: 'Erro Geral', value: data.generalScaleError ? 'Sim' : 'Não' },
    { label: 'Alarme', value: data.scaleAlarm ? 'Ativo' : 'Normal' }
  ];
}

export function App() {
  const [deviceUrl, setDeviceUrl] = useState(DEFAULT_URL);
  const [connected, setConnected] = useState(false);
  const [message, setMessage] = useState('');
  const [functions, setFunctions] = useState<DeviceFunctionMeta[]>([]);
  const [snapshot, setSnapshot] = useState<DeviceSnapshot | null>(null);
  const [weights, setWeights] = useState<WeightStreamPayload>(EMPTY_WEIGHT);
  const refreshTimer = useRef<number | null>(null);

  const handleWeightMessage = useCallback((payload: WeightStreamPayload) => {
    setWeights(payload);
  }, []);

  useWeightStream(handleWeightMessage);

  const updateSnapshot = useCallback(async () => {
    try {
      const status = await fetchStatus();
      setSnapshot(status);
      setConnected(status.connected);
    } catch (error) {
      setMessage((error as Error).message);
    }
  }, []);

  useEffect(() => {
    fetchFunctions()
      .then(setFunctions)
      .catch((error) => {
        console.error('Erro ao carregar funções', error);
      });
    updateSnapshot().catch((error) => {
      console.error('Erro ao carregar status', error);
    });
  }, [updateSnapshot]);

  useEffect(() => {
    if (connected) {
      updateSnapshot().catch(() => undefined);
      refreshTimer.current = window.setInterval(() => {
        updateSnapshot().catch(() => undefined);
      }, 2000);
    } else if (refreshTimer.current !== null) {
      window.clearInterval(refreshTimer.current);
      refreshTimer.current = null;
    }
    return () => {
      if (refreshTimer.current !== null) {
        window.clearInterval(refreshTimer.current);
        refreshTimer.current = null;
      }
    };
  }, [connected, updateSnapshot]);

  const handleConnect = useCallback(async () => {
    try {
      setMessage('Conectando...');
      const url = deviceUrl.trim() || DEFAULT_URL;
      await connectDevice(url);
      setConnected(true);
      setMessage('Conectado com sucesso.');
      await updateSnapshot();
    } catch (error) {
      setConnected(false);
      setMessage((error as Error).message);
    }
  }, [deviceUrl, updateSnapshot]);

  const handleDisconnect = useCallback(async () => {
    try {
      await disconnectDevice();
      setConnected(false);
      setSnapshot({ connected: false });
      setWeights(EMPTY_WEIGHT);
      setMessage('Desconectado.');
    } catch (error) {
      setMessage((error as Error).message);
    }
  }, []);

  const handleExecute = useCallback(async (meta: DeviceFunctionMeta, payload: Record<string, unknown>) => {
    const response = await invokeFunction(meta.name, payload);
    if (response.result === null || response.result === undefined) {
      return 'OK';
    }
    if (typeof response.result === 'string') {
      return response.result;
    }
    return JSON.stringify(response.result);
  }, []);

  const processData = snapshot?.processData;

  const configFunctions = useMemo(
    () => functions.filter((item) => item.kind === 'setter'),
    [functions]
  );
  const commandFunctions = useMemo(
    () => functions.filter((item) => item.kind === 'command'),
    [functions]
  );

  const liveUnit = weights.unit ? ` ${weights.unit}` : '';
  const statusText = weights.connected ? formatStatusText(weights.stable) : 'stream inativo';

  return (
    <div className="app">
      <header>
        <h1>DSE JetBus Controle Web</h1>
      </header>
      <main>
        <section>
          <div className="connection-controls">
            <div className="status-dot">
              <span className={`status-indicator${connected ? ' connected' : ''}`}></span>
              <span>{connected ? 'Conectado' : 'Desconectado'}</span>
            </div>
            <label htmlFor="url">URL do JetBus</label>
            <input
              id="url"
              type="text"
              value={deviceUrl}
              onChange={(event) => setDeviceUrl(event.target.value)}
              placeholder={DEFAULT_URL}
            />
            <div className="button-row">
              <button onClick={handleConnect} disabled={connected}>Conectar</button>
              <button className="secondary" onClick={handleDisconnect} disabled={!connected}>Desconectar</button>
            </div>
            <p className="message">{message}</p>
          </div>
        </section>
        <section>
          <h2>Dados de Processo</h2>
          <div className="live-weights">
            <div className="weight-card">
              <span className="metric-label">Net Weight</span>
              <span className="metric-value">{weights.connected ? formatValue(weights.net) : '—'}</span>
              <span className="weight-unit">{weights.connected ? liveUnit : ''}</span>
              <span className="weight-status">{statusText}</span>
            </div>
            <div className="weight-card">
              <span className="metric-label">Gross Weight</span>
              <span className="metric-value">{weights.connected ? formatValue(weights.gross) : '—'}</span>
              <span className="weight-unit">{weights.connected ? liveUnit : ''}</span>
              <span className="weight-status">{statusText}</span>
            </div>
          </div>
          <div className="process-grid">
            {processData && snapshot?.connected ? (
              buildProcessEntries(processData).map((entry) => (
                <div key={entry.label} className="metric-card">
                  <span className="metric-label">{entry.label}</span>
                  <span className="metric-value">{entry.value}</span>
                </div>
              ))
            ) : (
              <p>Nenhum dado disponível.</p>
            )}
          </div>
        </section>
        <section className="function-section">
          <h2>Funções disponíveis</h2>
          <div className="functions">
            {configFunctions.length > 0 && (
              <>
                <h3>Parâmetros de Configuração</h3>
                {configFunctions.map((meta) => (
                  <FunctionCard
                    key={meta.name}
                    meta={meta}
                    actionLabel="Aplicar"
                    disabled={!connected}
                    onRun={handleExecute}
                  />
                ))}
              </>
            )}
            {commandFunctions.length > 0 && (
              <>
                <h3>Funções Operacionais</h3>
                {commandFunctions.map((meta) => (
                  <FunctionCard
                    key={meta.name}
                    meta={meta}
                    actionLabel="Executar"
                    disabled={!connected}
                    onRun={handleExecute}
                  />
                ))}
              </>
            )}
          </div>
        </section>
      </main>
      <footer>
        Desenvolvido em TypeScript + Node.js + React. C++ original mantido apenas como referência.
      </footer>
    </div>
  );
}
