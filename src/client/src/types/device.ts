export interface WeightValues {
  net: number;
  gross: number;
  tare: number;
}

export interface PrintableWeightValues {
  net: string;
  gross: string;
  tare: string;
}

export interface ProcessStatus {
  weightStable: boolean;
  zeroRequired: boolean;
  centerOfZero: boolean;
  insideZero: boolean;
  legalForTrade: boolean;
  underload: boolean;
  overload: boolean;
  higherSafeLoadLimit: boolean;
  generalScaleError: boolean;
  scaleAlarm: boolean;
}

export interface ProcessDataSnapshot {
  timestamp: string;
  weight: WeightValues;
  printableWeight: PrintableWeightValues;
  unit: string;
  decimals: number;
  tareMode: string;
  status: ProcessStatus;
}

export interface DeviceState {
  connected: boolean;
  snapshot: ProcessDataSnapshot | null;
}

export type ActionParameterType = "number" | "string";

export interface ActionParameter {
  name: string;
  label: string;
  type: ActionParameterType;
  placeholder?: string;
}

export interface DeviceActionDefinition {
  id: string;
  label: string;
  description?: string;
  parameters?: ActionParameter[];
  returnsValue?: boolean;
}

export interface ApiActionsResponse {
  actions: DeviceActionDefinition[];
}
