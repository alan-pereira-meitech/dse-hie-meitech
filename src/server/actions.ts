export type ParameterType = "number" | "string";

export interface ActionParameter {
  name: string;
  label: string;
  type: ParameterType;
  placeholder?: string;
}

export interface DeviceActionDefinition {
  id: string;
  label: string;
  description?: string;
  parameters?: ActionParameter[];
  returnsValue?: boolean;
}

export const deviceActions: DeviceActionDefinition[] = [
  { id: "zero", label: "Zero" },
  { id: "tare", label: "Tare" },
  { id: "setGross", label: "Set Gross Mode" },
  { id: "recordWeight", label: "Record Weight" },
  { id: "adjustZeroSignal", label: "Adjust Zero Signal" },
  { id: "adjustNominalSignal", label: "Adjust Nominal Signal" },
  {
    id: "adjustNominalSignalWithCalibrationWeight",
    label: "Adjust Nominal Signal (Calibration Weight)",
    parameters: [{ name: "weight", label: "Calibration Weight", type: "number", placeholder: "e.g. 20.5" }]
  },
  {
    id: "calculateAdjustment",
    label: "Calculate Adjustment",
    parameters: [
      { name: "scaleZeroMvv", label: "Scale Zero (mV/V)", type: "number", placeholder: "e.g. 1.23" },
      { name: "capacityMvv", label: "Capacity (mV/V)", type: "number", placeholder: "e.g. 2.34" }
    ]
  },
  {
    id: "setUnit",
    label: "Set Unit",
    parameters: [{ name: "unit", label: "Unit", type: "string", placeholder: "kg | g | t | lb | N" }]
  },
  {
    id: "setManualTare",
    label: "Set Manual Tare",
    parameters: [{ name: "value", label: "Value", type: "number", placeholder: "e.g. 3.5" }]
  },
  {
    id: "setMaximumCapacity",
    label: "Set Maximum Capacity",
    parameters: [{ name: "value", label: "Value", type: "number", placeholder: "e.g. 1500" }]
  },
  {
    id: "setZeroSignal",
    label: "Set Zero Signal",
    parameters: [{ name: "value", label: "Value", type: "number", placeholder: "e.g. 100" }]
  },
  {
    id: "setNominalSignal",
    label: "Set Nominal Signal",
    parameters: [{ name: "value", label: "Value", type: "number", placeholder: "e.g. 200" }]
  },
  { id: "saveAllParameters", label: "Save All Parameters" },
  { id: "restoreDefaultParameters", label: "Restore Default Parameters" },
  { id: "serialNumber", label: "Read Serial Number", returnsValue: true },
  { id: "identification", label: "Read Identification", returnsValue: true },
  { id: "firmwareVersion", label: "Read Firmware Version", returnsValue: true },
  { id: "weightStep", label: "Read Weight Step", returnsValue: true },
  { id: "scaleRange", label: "Read Scale Range", returnsValue: true },
  { id: "maximumCapacity", label: "Read Maximum Capacity", returnsValue: true },
  { id: "zeroValue", label: "Read Zero Value", returnsValue: true },
  { id: "zeroSignal", label: "Read Zero Signal", returnsValue: true },
  { id: "nominalSignal", label: "Read Nominal Signal", returnsValue: true }
];
