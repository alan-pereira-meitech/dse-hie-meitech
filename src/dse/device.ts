import { JetBusClient, JetBusClientOptions } from "../jetbus/client";
import { Command, commands } from "../jetbus/commands";
import { ProcessData } from "../jetbus/processData";
import { doubleToDigit } from "../jetbus/measurementUtils";
import { JetBusError } from "../jetbus/types";
import { FilterType, TareMode } from "./types";

const SCALE_COMMAND_CALIBRATE_ZERO = 2053923171;
const SCALE_COMMAND_CALIBRATE_NOMINAL = 1852596579;
const SCALE_COMMAND_TARE = 1701994868;
const SCALE_COMMAND_ZERO = 1869768058;
const SCALE_COMMAND_SET_GROSS = 1936683623;
const DEFAULT_COMMAND_TIMEOUT = 10_000;
const PROCESS_DATA_TIMEOUT = 5_000;
const MVV_TO_D_CONVERSION = 1_000_000;

export type FilterStage = 2 | 3 | 4 | 5;

const FILTER_STAGE_ORDER: readonly FilterStage[] = [2, 3, 4, 5];

const FILTER_STAGE_COMMANDS: Record<FilterStage, Command> = {
  2: commands.dseFilterModeStage2(),
  3: commands.dseFilterModeStage3(),
  4: commands.dseFilterModeStage4(),
  5: commands.dseFilterModeStage5()
};

const COMB_FILTER_FREQUENCY_COMMANDS: Record<FilterStage, Command> = {
  2: commands.dseCombFilterFrequencyStage2(),
  3: commands.dseCombFilterFrequencyStage3(),
  4: commands.dseCombFilterFrequencyStage4(),
  5: commands.dseCombFilterFrequencyStage5()
};

const MOVING_AVERAGE_FREQUENCY_COMMANDS: Record<FilterStage, Command> = {
  2: commands.dseMovAvFilterFrequencyStage2(),
  3: commands.dseMovAvFilterFrequencyStage3(),
  4: commands.dseMovAvFilterFrequencyStage4(),
  5: commands.dseMovAvFilterFrequencyStage5()
};

const FILTER_TYPE_CODES: Record<number, readonly number[]> = {
  [FilterType.FIRCombFilter]: [13089, 13090, 13091, 13092],
  [FilterType.FIRMovingAverage]: [13105, 13106, 13107, 13108]
};

const DEFAULT_FETCH_PATHS = [
  "6002/02",
  "6012/01",
  "6013/01",
  "6015/01",
  "6016/01",
  "601A/01",
  "6113/01",
  "611C/01",
  "611C/02",
  "611C/03",
  "6141/02",
  "6142/00",
  "6143/00",
  "6144/00",
  "6153/00"
] as const;

export interface FilterStageConfiguration {
  stage: FilterStage;
  mode: FilterType;
  cutOffFrequency: number;
}

export interface DeviceOptions {
  clientOptions: JetBusClientOptions;
  processDataInterval?: number;
  autoFetchProcessData?: boolean;
}

export type ProcessDataCallback = (data: ProcessData) => void;

enum CommandStatus {
  Ongoing = 1634168417,
  Ok = 1801543519,
  ErrorE1 = 826629983,
  ErrorE2 = 843407199,
  ErrorE3 = 860184415
}

export class Device {
  private readonly options: DeviceOptions;
  private readonly client: JetBusClient;
  private readonly processData = new ProcessData();
  private processCallback?: ProcessDataCallback;
  private connected = false;
  private subscriptions = new Map<string, string>();

  constructor(options: DeviceOptions) {
    this.options = options;
    this.client = new JetBusClient(options.clientOptions);
    this.client.setDataCallback(() => {
      this.refreshProcessData();
    });
  }

  setProcessDataCallback(callback: ProcessDataCallback): void {
    this.processCallback = callback;
  }

  async connect(): Promise<void> {
    if (this.connected) {
      return;
    }
    this.client.connect();

    for (const path of DEFAULT_FETCH_PATHS) {
      const token = await this.client.fetch(path);
      this.subscriptions.set(path, token);
    }

    const processPath = commands.cia461NetValue().path;
    await this.client.fetch(processPath);
    const ready = await this.client.waitFor(
      processPath,
      (value) => value.length > 0,
      PROCESS_DATA_TIMEOUT
    );
    if (!ready) {
      throw new JetBusError(`Timeout waiting for process data at path: ${processPath}`);
    }

    this.refreshProcessData();
    this.connected = true;
  }

  async disconnect(): Promise<void> {
    if (!this.connected) {
      this.client.disconnect();
      return;
    }
    const tasks: Promise<void>[] = [];
    for (const [path, token] of this.subscriptions) {
      if (token) {
        tasks.push(this.client.unfetch(token));
      }
    }
    this.subscriptions.clear();
    await Promise.all(tasks);
    this.client.disconnect();
    this.connected = false;
  }

  isConnected(): boolean {
    return this.connected;
  }

  processDataSnapshot(): ProcessData {
    return this.processData;
  }

  netWeight(): number {
    return this.processData.weight().net;
  }

  grossWeight(): number {
    return this.processData.weight().gross;
  }

  tareWeight(): number {
    return this.processData.weight().tare;
  }

  unit(): string {
    return this.processData.unit();
  }

  decimals(): number {
    return this.processData.decimals();
  }

  tareMode(): TareMode {
    return this.processData.tareMode();
  }

  weightStable(): boolean {
    return this.processData.weightStable();
  }

  zeroRequired(): boolean {
    return this.processData.zeroRequired();
  }

  centerOfZero(): boolean {
    return this.processData.centerOfZero();
  }

  insideZero(): boolean {
    return this.processData.insideZero();
  }

  legalForTrade(): boolean {
    return this.processData.legalForTrade();
  }

  underload(): boolean {
    return this.processData.underload();
  }

  overload(): boolean {
    return this.processData.overload();
  }

  higherSafeLoadLimit(): boolean {
    return this.processData.higherSafeLoadLimit();
  }

  generalScaleError(): boolean {
    return this.processData.generalScaleError();
  }

  scaleAlarm(): boolean {
    return this.processData.scaleAlarm();
  }

  async weightStep(): Promise<number> {
    return this.readInt(commands.cia461WeightStep());
  }

  async scaleRange(): Promise<number> {
    return this.readInt(commands.cia461MultiIntervalRangeControl());
  }

  async maximumCapacity(): Promise<number> {
    return this.readInt(commands.cia461ScaleMaximumCapacity());
  }

  async zeroValue(): Promise<number> {
    return this.readInt(commands.cia461ZeroValue());
  }

  async zeroSignal(): Promise<number> {
    return this.readInt(commands.dseZeroSignal());
  }

  async nominalSignal(): Promise<number> {
    return this.readInt(commands.dseNominalSignal());
  }

  async identification(): Promise<string> {
    return this.ensureValue(commands.dseIdentification(), DEFAULT_COMMAND_TIMEOUT);
  }

  async firmwareVersion(): Promise<string> {
    return this.ensureValue(commands.dseFirmwareVersion(), DEFAULT_COMMAND_TIMEOUT);
  }

  async serialNumber(): Promise<number> {
    return this.readInt(commands.dseSerialNumber());
  }

  async setUnit(unitCode: string): Promise<void> {
    const unitValue = this.unitCodeFromString(unitCode);
    await this.writeInt(commands.cia461Unit(), unitValue);
  }

  async setManualTare(value: number): Promise<void> {
    const digits = doubleToDigit(value, this.decimals());
    await this.writeInt(commands.cia461TareValue(), digits);
  }

  async setMaximumCapacity(value: number): Promise<void> {
    await this.writeInt(commands.cia461ScaleMaximumCapacity(), value);
  }

  async setZeroSignal(value: number): Promise<void> {
    await this.writeInt(commands.dseZeroSignal(), value);
  }

  async setNominalSignal(value: number): Promise<void> {
    await this.writeInt(commands.dseNominalSignal(), value);
  }

  async filterStage2Mode(): Promise<FilterType> {
    return this.getFilterStageMode(2);
  }

  async setFilterStage2Mode(type: FilterType): Promise<void> {
    await this.setFilterStageMode(2, type);
  }

  async filterStage3Mode(): Promise<FilterType> {
    return this.getFilterStageMode(3);
  }

  async setFilterStage3Mode(type: FilterType): Promise<void> {
    await this.setFilterStageMode(3, type);
  }

  async filterStage4Mode(): Promise<FilterType> {
    return this.getFilterStageMode(4);
  }

  async setFilterStage4Mode(type: FilterType): Promise<void> {
    await this.setFilterStageMode(4, type);
  }

  async filterStage5Mode(): Promise<FilterType> {
    return this.getFilterStageMode(5);
  }

  async setFilterStage5Mode(type: FilterType): Promise<void> {
    await this.setFilterStageMode(5, type);
  }

  async filterCutOffFrequencyStage2(): Promise<number> {
    return this.getFilterCutOffFrequency(2);
  }

  async setFilterCutOffFrequencyStage2(value: number): Promise<void> {
    await this.setFilterCutOffFrequency(2, value);
  }

  async filterCutOffFrequencyStage3(): Promise<number> {
    return this.getFilterCutOffFrequency(3);
  }

  async setFilterCutOffFrequencyStage3(value: number): Promise<void> {
    await this.setFilterCutOffFrequency(3, value);
  }

  async filterCutOffFrequencyStage4(): Promise<number> {
    return this.getFilterCutOffFrequency(4);
  }

  async setFilterCutOffFrequencyStage4(value: number): Promise<void> {
    await this.setFilterCutOffFrequency(4, value);
  }

  async filterCutOffFrequencyStage5(): Promise<number> {
    return this.getFilterCutOffFrequency(5);
  }

  async setFilterCutOffFrequencyStage5(value: number): Promise<void> {
    await this.setFilterCutOffFrequency(5, value);
  }

  async filterStagesConfiguration(): Promise<FilterStageConfiguration[]> {
    const configurations = await Promise.all(
      FILTER_STAGE_ORDER.map(async (stage) => ({
        stage,
        mode: await this.getFilterStageMode(stage),
        cutOffFrequency: await this.getFilterCutOffFrequency(stage)
      }))
    );
    return configurations;
  }

  async configureFilterStage(
    stage: FilterStage,
    configuration: { mode?: FilterType; cutOffFrequency?: number }
  ): Promise<void> {
    if (configuration.mode !== undefined) {
      await this.setFilterStageMode(stage, configuration.mode);
    }
    if (configuration.cutOffFrequency !== undefined) {
      await this.setFilterCutOffFrequency(stage, configuration.cutOffFrequency);
    }
  }

  async saveAllParameters(): Promise<void> {
    await this.writeInt(commands.cia461SaveAllParameters(), 0);
  }

  async restoreDefaultParameters(): Promise<void> {
    await this.writeInt(commands.dseRestoreDefaults(), 0x6c6f6164);
  }

  async zero(): Promise<void> {
    await this.sendScaleCommand(SCALE_COMMAND_ZERO);
  }

  async tare(): Promise<void> {
    await this.sendScaleCommand(SCALE_COMMAND_TARE);
  }

  async setGross(): Promise<void> {
    await this.sendScaleCommand(SCALE_COMMAND_SET_GROSS);
  }

  async recordWeight(): Promise<void> {
    await this.writeInt(commands.stoRecordWeight(), SCALE_COMMAND_TARE);
  }

  async adjustZeroSignal(): Promise<boolean> {
    await this.sendScaleCommand(SCALE_COMMAND_CALIBRATE_ZERO);
    return this.waitForStatus(CommandStatus.Ok, DEFAULT_COMMAND_TIMEOUT);
  }

  async adjustNominalSignal(): Promise<boolean> {
    await this.sendScaleCommand(SCALE_COMMAND_CALIBRATE_NOMINAL);
    return this.waitForStatus(CommandStatus.Ok, DEFAULT_COMMAND_TIMEOUT);
  }

  async adjustNominalSignalWithCalibrationWeight(weight: number): Promise<boolean> {
    const digits = doubleToDigit(weight, this.decimals());
    await this.writeInt(commands.cia461CalibrationWeight(), digits);
    await this.sendScaleCommand(SCALE_COMMAND_CALIBRATE_NOMINAL);
    return this.waitForStatus(CommandStatus.Ok, DEFAULT_COMMAND_TIMEOUT);
  }

  async calculateAdjustment(scaleZeroMvv: number, capacityMvv: number): Promise<void> {
    const scaleZeroD = Math.round(scaleZeroMvv * MVV_TO_D_CONVERSION);
    const capacityD = Math.round((scaleZeroMvv + capacityMvv) * MVV_TO_D_CONVERSION);
    await this.writeInt(commands.ldwZeroValue(), scaleZeroD);
    await this.writeInt(commands.lwtNominalValue(), capacityD);
  }

  private async getFilterStageMode(stage: FilterStage): Promise<FilterType> {
    const command = FILTER_STAGE_COMMANDS[stage];
    const value = await this.readInt(command);
    return this.normalizeFilterType(value);
  }

  private async setFilterStageMode(stage: FilterStage, type: FilterType): Promise<void> {
    const command = FILTER_STAGE_COMMANDS[stage];
    const currentValue = await this.readInt(command);
    const currentType = this.normalizeFilterType(currentValue);
    if (currentType === type) {
      return;
    }
    const valueToWrite = await this.determineFilterCode(stage, type);
    await this.writeInt(command, valueToWrite);
  }

  private async determineFilterCode(stage: FilterStage, type: FilterType): Promise<number> {
    if (type === FilterType.NoFilter) {
      return 0;
    }
    const candidates = FILTER_TYPE_CODES[type];
    if (!candidates || candidates.length === 0) {
      return 0;
    }
    const values = await Promise.all(
      FILTER_STAGE_ORDER.map((filterStage) => this.readInt(FILTER_STAGE_COMMANDS[filterStage]))
    );
    const stageIndex = FILTER_STAGE_ORDER.indexOf(stage);
    if (stageIndex >= 0) {
      values[stageIndex] = 0;
    }
    for (const candidate of candidates) {
      if (!values.includes(candidate)) {
        return candidate;
      }
    }
    return candidates[0];
  }

  private async getFilterCutOffFrequency(stage: FilterStage): Promise<number> {
    const mode = await this.getFilterStageMode(stage);
    switch (mode) {
      case FilterType.NoFilter:
        return 0;
      case FilterType.FIRCombFilter:
        return this.readInt(COMB_FILTER_FREQUENCY_COMMANDS[stage]);
      case FilterType.FIRMovingAverage:
        return this.readInt(MOVING_AVERAGE_FREQUENCY_COMMANDS[stage]);
      default:
        return 0;
    }
  }

  private async setFilterCutOffFrequency(stage: FilterStage, frequency: number): Promise<void> {
    const mode = await this.getFilterStageMode(stage);
    switch (mode) {
      case FilterType.FIRCombFilter:
        await this.writeInt(COMB_FILTER_FREQUENCY_COMMANDS[stage], frequency);
        break;
      case FilterType.FIRMovingAverage:
        await this.writeInt(MOVING_AVERAGE_FREQUENCY_COMMANDS[stage], frequency);
        break;
      default:
        break;
    }
  }

  private normalizeFilterType(value: number): FilterType {
    if (value > 13104) {
      return FilterType.FIRMovingAverage;
    }
    if (value > 13088 && value < 13094) {
      return FilterType.FIRCombFilter;
    }
    return FilterType.NoFilter;
  }

  private async ensureValue(command: Command, timeout: number): Promise<string> {
    const cached = this.client.readCached(command.path);
    if (cached) {
      return cached;
    }
    const token = await this.client.fetch(command.path);
    const ready = await this.client.waitFor(
      command.path,
      (value) => value.length > 0,
      timeout
    );
    await this.client.unfetch(token);
    if (!ready) {
      throw new JetBusError(`Timeout while waiting for value on path ${command.path}`);
    }
    const updated = this.client.readCached(command.path);
    if (!updated) {
      throw new JetBusError(`Value not available for path ${command.path}`);
    }
    return updated;
  }

  private async readInt(command: Command): Promise<number> {
    const raw = await this.ensureValue(command, DEFAULT_COMMAND_TIMEOUT);
    return command.toInt(raw);
  }

  private async writeInt(command: Command, value: number): Promise<void> {
    await this.client.set(command.path, value);
  }

  private async sendScaleCommand(value: number): Promise<void> {
    await this.writeInt(commands.cia461ScaleCommand(), value);
    const started = await this.waitForStatus(CommandStatus.Ongoing, DEFAULT_COMMAND_TIMEOUT);
    if (!started) {
      throw new JetBusError("Timeout waiting for scale command to start");
    }
    const finished = await this.waitForStatus(CommandStatus.Ok, DEFAULT_COMMAND_TIMEOUT);
    if (!finished) {
      throw new JetBusError("Timeout waiting for scale command to finish");
    }
  }

  private async waitForStatus(
    desired: CommandStatus,
    timeout: number
  ): Promise<boolean> {
    const statusCommand = commands.cia461ScaleCommandStatus();
    const ok = await this.client.waitFor(
      statusCommand.path,
      (value) => {
        try {
          const parsed = statusCommand.toInt(value);
          if (desired === CommandStatus.Ongoing && parsed === CommandStatus.Ok) {
            return true;
          }
          return parsed === desired;
        } catch {
          return false;
        }
      },
      timeout
    );
    if (!ok) {
      return false;
    }
    if (desired === CommandStatus.Ok) {
      const status = await this.readStatus();
      if (status !== CommandStatus.Ok) {
        throw new JetBusError("Scale command finished with error");
      }
    }
    return true;
  }

  private async readStatus(): Promise<CommandStatus> {
    const statusCommand = commands.cia461ScaleCommandStatus();
    const raw = await this.ensureValue(statusCommand, DEFAULT_COMMAND_TIMEOUT);
    const value = statusCommand.toInt(raw);
    return value as CommandStatus;
  }

  private refreshProcessData(): void {
    const snapshot = this.client.snapshot();
    this.processData.update(snapshot);
    if (this.processCallback) {
      this.processCallback(this.processData);
    }
  }

  private unitCodeFromString(unit: string): number {
    switch (unit) {
      case "kg":
        return 0x00020000;
      case "g":
        return 0x004B0000;
      case "t":
        return 0x004C0000;
      case "lb":
        return 0x00A60000;
      case "N":
        return 0x00210000;
      default:
        throw new JetBusError(`Unsupported unit: ${unit}`);
    }
  }
}
