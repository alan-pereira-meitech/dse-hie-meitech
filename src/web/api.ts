import { DeviceFunctionMeta, DeviceOptionsResponse, DeviceSnapshot } from './types.js';

async function fetchJson<T>(input: RequestInfo, init?: RequestInit): Promise<T> {
  const response = await fetch(input, init);
  if (!response.ok) {
    const error = await response.json().catch(() => ({}));
    const message = typeof (error as { error?: unknown }).error === 'string'
      ? (error as { error?: string }).error
      : response.statusText;
    throw new Error(message);
  }
  return response.json() as Promise<T>;
}

export async function connectDevice(url: string): Promise<{ connected: boolean; url: string }>
{
  return fetchJson('/api/device/connect', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ url })
  });
}

export async function disconnectDevice(): Promise<{ connected: boolean }> {
  return fetchJson('/api/device/disconnect', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  });
}

export async function fetchStatus(): Promise<DeviceSnapshot> {
  return fetchJson<DeviceSnapshot>('/api/device/status');
}

export async function fetchOptions(): Promise<DeviceOptionsResponse> {
  return fetchJson<DeviceOptionsResponse>('/api/device/options');
}

export async function fetchFunctions(): Promise<DeviceFunctionMeta[]> {
  return fetchJson<DeviceFunctionMeta[]>('/api/device/functions');
}

export async function invokeFunction(name: string, payload: Record<string, unknown>): Promise<{ name: string; result: unknown }>
{
  return fetchJson(`/api/device/functions/${name}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
}
