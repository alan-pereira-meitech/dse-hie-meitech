import { ApplicationMode, PrintableWeightValues, TareMode, WeightValues } from '../dse/types.js';
import { commands, Command } from './commands.js';
import { digitToDouble } from './measurementUtils.js';

export interface ProcessDataSnapshot {
  applicationMode: ApplicationMode;
  weight: WeightValues;
  printableWeight: PrintableWeightValues;
  unit: string;
  decimals: number;
  tareMode: TareMode;
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

export class ProcessData {
  private applicationMode: ApplicationMode = ApplicationMode.Standard;
  private weight: WeightValues = { net: 0, gross: 0, tare: 0 };
  private printableWeight: PrintableWeightValues = { net: '0', gross: '0', tare: '0' };
  private unit = '';
  private decimals = 0;
  private tareMode: TareMode = TareMode.None;
  private weightStable = false;
  private centerOfZero = false;
  private insideZero = false;
  private zeroRequired = false;
  private scaleRange = 0;
  private legalForTrade = false;
  private underload = false;
  private overload = false;
  private higherSafeLoadLimit = false;
  private generalScaleError = false;
  private scaleAlarm = false;

  update(cache: Map<string, string>): void {
    this.applicationMode = this.readInt(cache, commands.imdApplicationMode(), ApplicationMode.Standard) as ApplicationMode;

    this.generalScaleError = this.readInt(cache, commands.cia461WeightStatusGeneralWeightError(), 0) !== 0;
    this.scaleAlarm = this.readInt(cache, commands.cia461WeightStatusScaleAlarm(), 0) !== 0;

    const limitStatus = this.readInt(cache, commands.cia461WeightStatusLimitStatus(), 0);
    this.underload = limitStatus === 1;
    this.overload = limitStatus === 2;
    this.higherSafeLoadLimit = limitStatus === 3;

    const manualTare = this.readInt(cache, commands.cia461WeightStatusManualTare(), 0);
    const weightType = this.readInt(cache, commands.cia461WeightStatusWeightType(), 0);
    this.tareMode = this.evaluateTareMode(manualTare, weightType);

    this.weightStable = this.readInt(cache, commands.cia461WeightStatusWeightMoving(), 0) === 0;
    this.legalForTrade = this.readInt(cache, commands.cia461WeightStatusScaleSealIsOpen(), 0) === 0;
    this.scaleRange = this.readInt(cache, commands.cia461WeightStatusScaleRange(), 0);
    this.zeroRequired = this.readInt(cache, commands.cia461WeightStatusZeroRequired(), 0) !== 0;
    this.centerOfZero = this.readInt(cache, commands.cia461WeightStatusCenterOfZero(), 0) !== 0;
    this.insideZero = this.readInt(cache, commands.cia461WeightStatusInsideZero(), 0) !== 0;

    this.decimals = this.readInt(cache, commands.cia461Decimals(), 0);
    this.unit = this.unitFromId(this.readInt(cache, commands.cia461Unit(), 0));

    const netRaw = this.readInt(cache, commands.cia461NetValue(), 0);
    const grossRaw = this.readInt(cache, commands.cia461GrossValue(), 0);
    const tareRaw = this.readInt(cache, commands.cia461TareValue(), 0);

    this.weight = {
      net: digitToDouble(netRaw, this.decimals),
      gross: digitToDouble(grossRaw, this.decimals),
      tare: digitToDouble(tareRaw, this.decimals)
    };

    this.updatePrintable();
  }

  toJSON(): ProcessDataSnapshot {
    return {
      applicationMode: this.applicationMode,
      weight: { ...this.weight },
      printableWeight: { ...this.printableWeight },
      unit: this.unit,
      decimals: this.decimals,
      tareMode: this.tareMode,
      weightStable: this.weightStable,
      centerOfZero: this.centerOfZero,
      insideZero: this.insideZero,
      zeroRequired: this.zeroRequired,
      scaleRange: this.scaleRange,
      legalForTrade: this.legalForTrade,
      underload: this.underload,
      overload: this.overload,
      higherSafeLoadLimit: this.higherSafeLoadLimit,
      generalScaleError: this.generalScaleError,
      scaleAlarm: this.scaleAlarm
    };
  }

  getWeight(): WeightValues {
    return this.weight;
  }

  getPrintableWeight(): PrintableWeightValues {
    return this.printableWeight;
  }

  getUnit(): string {
    return this.unit;
  }

  getDecimals(): number {
    return this.decimals;
  }

  getTareMode(): TareMode {
    return this.tareMode;
  }

  isWeightStable(): boolean {
    return this.weightStable;
  }

  isCenterOfZero(): boolean {
    return this.centerOfZero;
  }

  isInsideZero(): boolean {
    return this.insideZero;
  }

  isZeroRequired(): boolean {
    return this.zeroRequired;
  }

  getScaleRange(): number {
    return this.scaleRange;
  }

  isLegalForTrade(): boolean {
    return this.legalForTrade;
  }

  isUnderload(): boolean {
    return this.underload;
  }

  isOverload(): boolean {
    return this.overload;
  }

  isHigherSafeLoadLimit(): boolean {
    return this.higherSafeLoadLimit;
  }

  hasGeneralScaleError(): boolean {
    return this.generalScaleError;
  }

  hasScaleAlarm(): boolean {
    return this.scaleAlarm;
  }

  private findValue(cache: Map<string, string>, command: Command): string | undefined {
    return cache.get(command.path);
  }

  private readInt(cache: Map<string, string>, command: Command, fallback = 0): number {
    const value = this.findValue(cache, command);
    if (value === undefined) {
      return fallback;
    }
    try {
      return command.toInt(value);
    } catch {
      return fallback;
    }
  }

  private unitFromId(id: number): string {
    switch (id) {
      case 0x00020000:
        return 'kg';
      case 0x004B0000:
        return 'g';
      case 0x004C0000:
        return 't';
      case 0x00A60000:
        return 'lb';
      case 0x00210000:
        return 'N';
      default:
        return '';
    }
  }

  private evaluateTareMode(tare: number, presetTare: number): TareMode {
    if (tare > 0) {
      if (presetTare > 0) {
        return TareMode.PresetTare;
      }
      return TareMode.Tare;
    }
    return TareMode.None;
  }

  private updatePrintable(): void {
    const formatter = new Intl.NumberFormat(undefined, {
      minimumFractionDigits: this.decimals,
      maximumFractionDigits: this.decimals,
      useGrouping: false
    });
    this.printableWeight = {
      net: formatter.format(this.weight.net),
      gross: formatter.format(this.weight.gross),
      tare: formatter.format(this.weight.tare)
    };
  }
}
