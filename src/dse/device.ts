import { JetBusClient, JetBusOptions } from '../jetbus/client.js';
import { commands, Command } from '../jetbus/commands.js';
import { ProcessData, ProcessDataSnapshot } from '../jetbus/processData.js';
import { doubleToDigit } from '../jetbus/measurementUtils.js';
import { JetBusError } from '../jetbus/types.js';
import { TareMode } from './types.js';

const SCALE_COMMAND_CALIBRATE_ZERO = 2053923171;
const SCALE_COMMAND_CALIBRATE_NOMINAL = 1852596579;
const SCALE_COMMAND_EXIT_CALIBRATE = 1953069157;
const SCALE_COMMAND_TARE = 1701994868;
const SCALE_COMMAND_ZERO = 1869768058;
const SCALE_COMMAND_SET_GROSS = 1936683623;
const DEFAULT_COMMAND_TIMEOUT = 10_000;
const CONVERSION_FACTOR_MVV_TO_D = 1_000_000;

const DEFAULT_FETCH_PATHS = [
  '6002/02',
  '6012/01',
  '6013/01',
  '6015/01',
  '6016/01',
  '601A/01',
  '6113/01',
  '611C/01',
  '611C/02',
  '611C/03',
  '6141/02',
  '6142/00',
  '6143/00',
  '6144/00',
  '6153/00'
];

enum CommandStatus {
  Ongoing = 1634168417,
  Ok = 1801543519,
  ErrorE1 = 826629983,
  ErrorE2 = 843407199,
  ErrorE3 = 860184415
}

export interface DeviceOptions {
  clientOptions: JetBusOptions;
  autoFetchProcessData?: boolean;
  processDataInterval?: number;
}

export interface DeviceSnapshot {
  connected: boolean;
  processData: ProcessDataSnapshot;
}

type ProcessDataCallback = (snapshot: ProcessDataSnapshot) => void;

export class Device {
  private readonly options: DeviceOptions;
  private readonly client: JetBusClient;
  private readonly processData = new ProcessData();
  private processCallback?: ProcessDataCallback;
  private connected = false;
  private readonly subscriptions = new Map<string, string>();

  constructor(options: DeviceOptions) {
    this.options = options;
    this.client = new JetBusClient(options.clientOptions);
    this.client.on('data', () => {
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
    await this.client.connect();
    for (const path of DEFAULT_FETCH_PATHS) {
      const token = await this.client.fetch(path);
      this.subscriptions.set(path, token);
    }
    const pdPath = commands.cia461NetValue().path;
    const ready = await this.client.waitFor(pdPath, (value) => value.length > 0, 5_000);
    if (!ready) {
      throw new JetBusError(`Timeout waiting for process data at path ${pdPath}`);
    }
    this.refreshProcessData();
    this.connected = true;
  }

  async disconnect(): Promise<void> {
    if (!this.connected) {
      await this.client.disconnect();
      return;
    }
    for (const [path, token] of this.subscriptions) {
      void path;
      await this.client.unfetch(token);
    }
    this.subscriptions.clear();
    await this.client.disconnect();
    this.connected = false;
  }

  isConnected(): boolean {
    return this.connected;
  }

  snapshot(): DeviceSnapshot {
    return {
      connected: this.connected,
      processData: this.processData.toJSON()
    };
  }

  netWeight(): number {
    return this.processData.getWeight().net;
  }

  grossWeight(): number {
    return this.processData.getWeight().gross;
  }

  tareWeight(): number {
    return this.processData.getWeight().tare;
  }

  unit(): string {
    return this.processData.getUnit();
  }

  decimals(): number {
    return this.processData.getDecimals();
  }

  tareMode(): TareMode {
    return this.processData.getTareMode();
  }

  weightStable(): boolean {
    return this.processData.isWeightStable();
  }

  zeroRequired(): boolean {
    return this.processData.isZeroRequired();
  }

  centerOfZero(): boolean {
    return this.processData.isCenterOfZero();
  }

  insideZero(): boolean {
    return this.processData.isInsideZero();
  }

  legalForTrade(): boolean {
    return this.processData.isLegalForTrade();
  }

  underload(): boolean {
    return this.processData.isUnderload();
  }

  overload(): boolean {
    return this.processData.isOverload();
  }

  higherSafeLoadLimit(): boolean {
    return this.processData.isHigherSafeLoadLimit();
  }

  generalScaleError(): boolean {
    return this.processData.hasGeneralScaleError();
  }

  scaleAlarm(): boolean {
    return this.processData.hasScaleAlarm();
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
    const scaleZeroD = Math.round(scaleZeroMvv * CONVERSION_FACTOR_MVV_TO_D);
    const capacityD = Math.round((scaleZeroMvv + capacityMvv) * CONVERSION_FACTOR_MVV_TO_D);
    await this.writeInt(commands.ldwZeroValue(), scaleZeroD);
    await this.writeInt(commands.lwtNominalValue(), capacityD);
  }

  private refreshProcessData(): void {
    const snapshot = this.client.snapshot();
    this.processData.update(snapshot);
    if (this.processCallback) {
      this.processCallback(this.processData.toJSON());
    }
  }

  private async ensureValue(command: Command, timeout: number): Promise<string> {
    const cached = this.client.readCached(command.path);
    if (cached !== undefined && cached !== '') {
      return cached;
    }
    const token = await this.client.fetch(command.path);
    try {
      const ready = await this.client.waitFor(command.path, (value) => value.length > 0, timeout);
      if (!ready) {
        throw new JetBusError(`Timeout while waiting for value on path ${command.path}`);
      }
      const value = this.client.readCached(command.path);
      if (value === undefined) {
        throw new JetBusError(`Value not available for path ${command.path}`);
      }
      return value;
    } finally {
      await this.client.unfetch(token);
    }
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
      throw new JetBusError('Timeout waiting for scale command to start');
    }
    const finished = await this.waitForStatus(CommandStatus.Ok, DEFAULT_COMMAND_TIMEOUT);
    if (!finished) {
      throw new JetBusError('Timeout waiting for scale command to finish');
    }
    const status = await this.readStatus();
    if (status !== CommandStatus.Ok) {
      throw new JetBusError('Scale command finished with error');
    }
  }

  private async waitForStatus(desired: CommandStatus, timeout: number): Promise<boolean> {
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
    return ok;
  }

  private async readStatus(): Promise<CommandStatus> {
    const raw = await this.ensureValue(commands.cia461ScaleCommandStatus(), DEFAULT_COMMAND_TIMEOUT);
    const value = commands.cia461ScaleCommandStatus().toInt(raw);
    return value as CommandStatus;
  }

  private unitCodeFromString(unit: string): number {
    switch (unit) {
      case 'kg':
        return 0x00020000;
      case 'g':
        return 0x004B0000;
      case 't':
        return 0x004C0000;
      case 'lb':
        return 0x00A60000;
      case 'N':
        return 0x00210000;
      default:
        throw new JetBusError(`Unsupported unit: ${unit}`);
    }
  }
}
