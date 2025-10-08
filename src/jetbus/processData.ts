import { ApplicationMode, PrintableWeightValues, TareMode, WeightValues } from "../dse/types";
import { commands, Command } from "./commands";
import { digitToDouble } from "./measurementUtils";

export class ProcessData {
  private applicationModeValue = ApplicationMode.Standard;
  private weightValue: WeightValues = { net: 0, gross: 0, tare: 0 };
  private printableWeightValue: PrintableWeightValues = { net: "0", gross: "0", tare: "0" };
  private unitValue = "";
  private decimalsValue = 0;
  private tareModeValue = TareMode.None;
  private weightStableValue = false;
  private centerOfZeroValue = false;
  private insideZeroValue = false;
  private zeroRequiredValue = false;
  private scaleRangeValue = 0;
  private legalForTradeValue = false;
  private underloadValue = false;
  private overloadValue = false;
  private higherSafeLoadLimitValue = false;
  private generalScaleErrorValue = false;
  private scaleAlarmValue = false;

  update(cache: ReadonlyMap<string, string>): void {
    const readInt = (command: Command, fallback = 0): number => {
      const raw = cache.get(command.path);
      if (raw === undefined) {
        return fallback;
      }
      try {
        return command.toInt(raw);
      } catch {
        return fallback;
      }
    };

    this.applicationModeValue = readInt(commands.imdApplicationMode(), ApplicationMode.Standard) as ApplicationMode;

    this.generalScaleErrorValue = readInt(commands.cia461WeightStatusGeneralWeightError(), 0) !== 0;
    this.scaleAlarmValue = readInt(commands.cia461WeightStatusScaleAlarm(), 0) !== 0;

    const limitStatus = readInt(commands.cia461WeightStatusLimitStatus(), 0);
    this.underloadValue = limitStatus === 1;
    this.overloadValue = limitStatus === 2;
    this.higherSafeLoadLimitValue = limitStatus === 3;

    const manualTare = readInt(commands.cia461WeightStatusManualTare(), 0);
    const weightType = readInt(commands.cia461WeightStatusWeightType(), 0);
    this.tareModeValue = this.evaluateTareMode(manualTare, weightType);

    this.weightStableValue = readInt(commands.cia461WeightStatusWeightMoving(), 0) === 0;
    this.legalForTradeValue = readInt(commands.cia461WeightStatusScaleSealIsOpen(), 0) === 0;
    this.scaleRangeValue = readInt(commands.cia461WeightStatusScaleRange(), 0);
    this.zeroRequiredValue = readInt(commands.cia461WeightStatusZeroRequired(), 0) !== 0;
    this.centerOfZeroValue = readInt(commands.cia461WeightStatusCenterOfZero(), 0) !== 0;
    this.insideZeroValue = readInt(commands.cia461WeightStatusInsideZero(), 0) !== 0;

    this.decimalsValue = readInt(commands.cia461Decimals(), 0);
    this.unitValue = this.unitFromId(readInt(commands.cia461Unit(), 0));

    const netRaw = readInt(commands.cia461NetValue(), 0);
    const grossRaw = readInt(commands.cia461GrossValue(), 0);
    const tareRaw = readInt(commands.cia461TareValue(), 0);

    this.weightValue = {
      net: digitToDouble(netRaw, this.decimalsValue),
      gross: digitToDouble(grossRaw, this.decimalsValue),
      tare: digitToDouble(tareRaw, this.decimalsValue)
    };

    this.printableWeightValue = this.buildPrintable(this.weightValue, this.decimalsValue);
  }

  applicationMode(): ApplicationMode {
    return this.applicationModeValue;
  }

  weight(): WeightValues {
    return this.weightValue;
  }

  printableWeight(): PrintableWeightValues {
    return this.printableWeightValue;
  }

  unit(): string {
    return this.unitValue;
  }

  decimals(): number {
    return this.decimalsValue;
  }

  tareMode(): TareMode {
    return this.tareModeValue;
  }

  weightStable(): boolean {
    return this.weightStableValue;
  }

  centerOfZero(): boolean {
    return this.centerOfZeroValue;
  }

  insideZero(): boolean {
    return this.insideZeroValue;
  }

  zeroRequired(): boolean {
    return this.zeroRequiredValue;
  }

  scaleRange(): number {
    return this.scaleRangeValue;
  }

  legalForTrade(): boolean {
    return this.legalForTradeValue;
  }

  underload(): boolean {
    return this.underloadValue;
  }

  overload(): boolean {
    return this.overloadValue;
  }

  higherSafeLoadLimit(): boolean {
    return this.higherSafeLoadLimitValue;
  }

  generalScaleError(): boolean {
    return this.generalScaleErrorValue;
  }

  scaleAlarm(): boolean {
    return this.scaleAlarmValue;
  }

  private unitFromId(id: number): string {
    switch (id) {
      case 0x00020000:
        return "kg";
      case 0x004B0000:
        return "g";
      case 0x004C0000:
        return "t";
      case 0x00A60000:
        return "lb";
      case 0x00210000:
        return "N";
      default:
        return "";
    }
  }

  private evaluateTareMode(tare: number, presetTare: number): TareMode {
    if (tare > 0) {
      return presetTare > 0 ? TareMode.PresetTare : TareMode.Tare;
    }
    return TareMode.None;
  }

  private buildPrintable(weight: WeightValues, decimals: number): PrintableWeightValues {
    const format = (value: number) => value.toFixed(decimals);
    return {
      net: format(weight.net),
      gross: format(weight.gross),
      tare: format(weight.tare)
    };
  }
}
