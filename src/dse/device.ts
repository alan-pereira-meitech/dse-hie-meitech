import { JetBusClient, JetBusClientOptions } from "../jetbus/client";
import { Command, commands } from "../jetbus/commands";
import { ProcessData } from "../jetbus/processData";
import { doubleToDigit } from "../jetbus/measurementUtils";
import { JetBusError } from "../jetbus/types";
import { TareMode } from "./types";

const SCALE_COMMAND_CALIBRATE_ZERO = 2053923171;
const SCALE_COMMAND_CALIBRATE_NOMINAL = 1852596579;
const SCALE_COMMAND_TARE = 1701994868;
const SCALE_COMMAND_ZERO = 1869768058;
const SCALE_COMMAND_SET_GROSS = 1936683623;
const DEFAULT_COMMAND_TIMEOUT = 10_000;
const PROCESS_DATA_TIMEOUT = 5_000;
const MVV_TO_D_CONVERSION = 1_000_000;

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
