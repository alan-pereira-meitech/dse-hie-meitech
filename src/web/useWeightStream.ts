import { useEffect } from 'react';
import { WeightStreamPayload } from './types.js';

export function useWeightStream(onPayload: (payload: WeightStreamPayload) => void): void {
  useEffect(() => {
    let socket: WebSocket | null = null;
    let reconnectTimer: number | null = null;

    const scheduleReconnect = () => {
      if (reconnectTimer !== null) {
        return;
      }
      reconnectTimer = window.setTimeout(() => {
        reconnectTimer = null;
        openSocket();
      }, 2000);
    };

    const openSocket = () => {
      if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
        return;
      }
      const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
      socket = new WebSocket(`${protocol}://${window.location.host}/process`);
      socket.addEventListener('message', (event) => {
        try {
          const payload = JSON.parse(event.data) as WeightStreamPayload;
          if (payload.type === 'weight') {
            onPayload(payload);
          }
        } catch (error) {
          console.error('Erro ao processar stream', error);
        }
      });
      socket.addEventListener('close', () => {
        if (socket) {
          socket = null;
        }
        scheduleReconnect();
      });
      socket.addEventListener('error', () => {
        socket?.close();
      });
    };

    openSocket();

    return () => {
      if (socket) {
        socket.close();
        socket = null;
      }
      if (reconnectTimer !== null) {
        window.clearTimeout(reconnectTimer);
        reconnectTimer = null;
      }
    };
  }, [onPayload]);
}
