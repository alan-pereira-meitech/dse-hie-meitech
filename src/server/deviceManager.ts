import { Device, DeviceOptions, FilterStage } from "../dse/device";
import { FilterType, PrintableWeightValues, TareMode, WeightValues } from "../dse/types";
import { ProcessData } from "../jetbus/processData";
import { JetBusError } from "../jetbus/types";

export interface ProcessDataSnapshot {
  timestamp: string;
  weight: WeightValues;
  printableWeight: PrintableWeightValues;
  unit: string;
  decimals: number;
  tareMode: TareMode;
  status: {
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
  };
}

export interface DeviceState {
  connected: boolean;
  snapshot: ProcessDataSnapshot | null;
}

export type DeviceEventListener = (snapshot: ProcessDataSnapshot) => void;

export type FilterTypeName = "NoFilter" | "FIRCombFilter" | "FIRMovingAverage";

export interface FilterStageState {
  stage: FilterStage;
  mode: FilterTypeName;
  cutOffFrequency: number;
}

export interface FilterStageUpdateInput {
  mode?: string;
  cutOffFrequency?: number | string;
}

const FILTER_STAGE_VALUES: readonly FilterStage[] = [2, 3, 4, 5];

const FILTER_TYPE_NAME_BY_VALUE: Record<FilterType, FilterTypeName> = {
  [FilterType.NoFilter]: "NoFilter",
  [FilterType.FIRCombFilter]: "FIRCombFilter",
  [FilterType.FIRMovingAverage]: "FIRMovingAverage"
};

const FILTER_TYPE_VALUE_BY_NAME: Record<FilterTypeName, FilterType> = {
  NoFilter: FilterType.NoFilter,
  FIRCombFilter: FilterType.FIRCombFilter,
  FIRMovingAverage: FilterType.FIRMovingAverage
};

export class DeviceManager {
  private readonly device: Device;
  private readonly listeners = new Set<DeviceEventListener>();
  private latestSnapshot: ProcessDataSnapshot | null = null;

  constructor(options: DeviceOptions) {
    this.device = new Device(options);
    this.device.setProcessDataCallback((data) => this.handleProcessData(data));
  }

  onData(listener: DeviceEventListener): () => void {
    this.listeners.add(listener);
    if (this.latestSnapshot) {
      listener(this.latestSnapshot);
    }
    return () => this.listeners.delete(listener);
  }

  async connect(): Promise<DeviceState> {
    await this.device.connect();
    this.captureSnapshot();
    return this.getState();
  }

  async disconnect(): Promise<DeviceState> {
    await this.device.disconnect();
    this.latestSnapshot = null;
    return this.getState();
  }

  getState(): DeviceState {
    return {
      connected: this.device.isConnected(),
      snapshot: this.latestSnapshot
    };
  }

  async performAction(action: string, payload: Record<string, unknown> = {}): Promise<unknown> {
    switch (action) {
      case "zero":
        await this.requireConnection();
        return this.device.zero();
      case "tare":
        await this.requireConnection();
        return this.device.tare();
      case "setGross":
        await this.requireConnection();
        return this.device.setGross();
      case "recordWeight":
        await this.requireConnection();
        return this.device.recordWeight();
      case "adjustZeroSignal":
        await this.requireConnection();
        return this.device.adjustZeroSignal();
      case "adjustNominalSignal":
        await this.requireConnection();
        return this.device.adjustNominalSignal();
      case "adjustNominalSignalWithCalibrationWeight": {
        await this.requireConnection();
        const weight = Number(payload.weight);
        this.assertNumber(weight, "weight");
        return this.device.adjustNominalSignalWithCalibrationWeight(weight);
      }
      case "calculateAdjustment": {
        await this.requireConnection();
        const zero = Number(payload.scaleZeroMvv);
        const capacity = Number(payload.capacityMvv);
        this.assertNumber(zero, "scaleZeroMvv");
        this.assertNumber(capacity, "capacityMvv");
        return this.device.calculateAdjustment(zero, capacity);
      }
      case "setUnit": {
        await this.requireConnection();
        const unit = String(payload.unit ?? "").trim();
        if (!unit) {
          throw new JetBusError("unit is required");
        }
        return this.device.setUnit(unit);
      }
      case "setManualTare": {
        await this.requireConnection();
        const value = Number(payload.value);
        this.assertNumber(value, "value");
        return this.device.setManualTare(value);
      }
      case "setMaximumCapacity": {
        await this.requireConnection();
        const value = Number(payload.value);
        this.assertNumber(value, "value");
        return this.device.setMaximumCapacity(value);
      }
      case "setZeroSignal": {
        await this.requireConnection();
        const value = Number(payload.value);
        this.assertNumber(value, "value");
        return this.device.setZeroSignal(value);
      }
      case "setNominalSignal": {
        await this.requireConnection();
        const value = Number(payload.value);
        this.assertNumber(value, "value");
        return this.device.setNominalSignal(value);
      }
      case "saveAllParameters":
        await this.requireConnection();
        return this.device.saveAllParameters();
      case "restoreDefaultParameters":
        await this.requireConnection();
        return this.device.restoreDefaultParameters();
      case "serialNumber":
        await this.requireConnection();
        return this.device.serialNumber();
      case "identification":
        await this.requireConnection();
        return this.device.identification();
      case "firmwareVersion":
        await this.requireConnection();
        return this.device.firmwareVersion();
      case "weightStep":
        await this.requireConnection();
        return this.device.weightStep();
      case "scaleRange":
        await this.requireConnection();
        return this.device.scaleRange();
      case "maximumCapacity":
        await this.requireConnection();
        return this.device.maximumCapacity();
      case "zeroValue":
        await this.requireConnection();
        return this.device.zeroValue();
      case "zeroSignal":
        await this.requireConnection();
        return this.device.zeroSignal();
      case "nominalSignal":
        await this.requireConnection();
        return this.device.nominalSignal();
      default:
        throw new JetBusError(`Unknown action: ${action}`);
    }
  }

  async getFilterConfiguration(): Promise<FilterStageState[]> {
    await this.requireConnection();
    const configuration = await this.device.filterStagesConfiguration();
    return configuration.map((stageConfig) => ({
      stage: stageConfig.stage,
      mode: FILTER_TYPE_NAME_BY_VALUE[stageConfig.mode],
      cutOffFrequency: stageConfig.cutOffFrequency
    }));
  }

  async updateFilterStage(stage: number, configuration: FilterStageUpdateInput): Promise<void> {
    await this.requireConnection();
    if (!FILTER_STAGE_VALUES.includes(stage as FilterStage)) {
      throw new JetBusError("stage must be one of 2, 3, 4 or 5");
    }

    const update: { mode?: FilterType; cutOffFrequency?: number } = {};

    if (configuration.mode !== undefined) {
      const normalized = String(configuration.mode) as FilterTypeName;
      const mapped = FILTER_TYPE_VALUE_BY_NAME[normalized];
      if (mapped === undefined) {
        throw new JetBusError(`Unsupported filter mode: ${configuration.mode}`);
      }
      update.mode = mapped;
    }

    if (configuration.cutOffFrequency !== undefined) {
      const numeric = Number(configuration.cutOffFrequency);
      this.assertNumber(numeric, "cutOffFrequency");
      update.cutOffFrequency = numeric;
    }

    if (Object.keys(update).length === 0) {
      return;
    }

    await this.device.configureFilterStage(stage as FilterStage, update);
  }

  private async requireConnection(): Promise<void> {
    if (!this.device.isConnected()) {
      throw new JetBusError("Device is not connected");
    }
  }

  private assertNumber(value: number, field: string): void {
    if (!Number.isFinite(value)) {
      throw new JetBusError(`${field} must be a valid number`);
    }
  }

  private handleProcessData(data: ProcessData): void {
    this.captureSnapshot(data);
    if (!this.latestSnapshot) {
      return;
    }
    for (const listener of this.listeners) {
      listener(this.latestSnapshot);
    }
  }

  private captureSnapshot(data?: ProcessData): void {
    const source = data ?? this.device.processDataSnapshot();
    const weight = source.weight();
    const printable = source.printableWeight();
    this.latestSnapshot = {
      timestamp: new Date().toISOString(),
      weight: { ...weight },
      printableWeight: { ...printable },
      unit: source.unit(),
      decimals: source.decimals(),
      tareMode: source.tareMode(),
      status: {
        weightStable: source.weightStable(),
        zeroRequired: source.zeroRequired(),
        centerOfZero: source.centerOfZero(),
        insideZero: source.insideZero(),
        legalForTrade: source.legalForTrade(),
        underload: source.underload(),
        overload: source.overload(),
        higherSafeLoadLimit: source.higherSafeLoadLimit(),
        generalScaleError: source.generalScaleError(),
        scaleAlarm: source.scaleAlarm()
      }
    };
  }

  private describeTareMode(mode: TareMode): string {
    switch (mode) {
      case TareMode.Tare:
        return "Tara";
      case TareMode.PresetTare:
        return "Tara pré-ajustada";
      default:
        return "Nenhum";
    }
  }
}
