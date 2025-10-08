export interface PrintableWeightValues {
  net: string;
  gross: string;
  tare: string;
}

export interface ProcessDataSnapshot {
  printableWeight: PrintableWeightValues;
  unit: string;
  decimals: number;
  tareMode: number;
  weightStable: boolean;
  centerOfZero: boolean;
  insideZero: boolean;
  zeroRequired: boolean;
  scaleRange: number;
  legalForTrade: boolean;
  underload: boolean;
  overload: boolean;
  higherSafeLoadLimit: boolean;
  generalScaleError: boolean;
  scaleAlarm: boolean;
}

export interface DeviceSnapshot {
  connected: boolean;
  processData?: ProcessDataSnapshot;
}

export interface WeightStreamPayload {
  type: 'weight';
  connected: boolean;
  net?: string;
  gross?: string;
  tare?: string;
  unit?: string;
  decimals?: number;
  stable: boolean;
}

export type DeviceFunctionKind = 'getter' | 'command' | 'setter';

export interface DeviceFunctionInput {
  name: string;
  label: string;
  type: 'string' | 'number';
  placeholder?: string;
}

export interface DeviceFunctionMeta {
  name: string;
  label: string;
  description: string;
  kind: DeviceFunctionKind;
  method: string;
  inputs?: DeviceFunctionInput[];
}
