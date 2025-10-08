import { DataType, JetBusError } from "./types";

export interface Command {
  readonly name: string;
  readonly type: DataType;
  readonly path: string;
  readonly bitIndex: number;
  readonly bitLength: number;
  toInt(raw: string): number;
  toString(raw: string): string;
}

type CommandMap = Map<string, Command>;

function extractBit(value: number, bitIndex: number, bitLength: number): number {
  if (bitLength === 0) return value;
  const mask = (1 << bitLength) - 1;
  return (value >> bitIndex) & mask;
}

function makeCommand(
  name: string,
  type: DataType,
  path: string,
  bitIndex = 0,
  bitLength = 0
): Command {
  return {
    name,
    type,
    path,
    bitIndex,
    bitLength,
    toInt(raw: string): number {
      const parsed = Number.parseInt(raw, 10);
      if (Number.isNaN(parsed)) {
        return 0;
      }
      if (type === DataType.Bit) {
        return extractBit(parsed, bitIndex, bitLength || 1);
      }
      return parsed;
    },
    toString(raw: string): string {
      if (type === DataType.Bit) {
        return this.toInt(raw).toString();
      }
      return raw;
    }
  };
}

const registry: Command[] = [];
const byName: CommandMap = new Map();
const byPath: Map<string, Command[]> = new Map();

function register(command: Command): Command {
  registry.push(command);
  byName.set(command.name, command);
  const commands = byPath.get(command.path) ?? [];
  commands.push(command);
  byPath.set(command.path, commands);
  return command;
}

function setupRegistry(): void {
  if (registry.length > 0) {
    return;
  }

  register(makeCommand("CIA461NetValue", DataType.S32, "601A/01"));
  register(makeCommand("CIA461GrossValue", DataType.S32, "6144/00"));
  register(makeCommand("CIA461TareValue", DataType.S32, "6143/00"));
  register(makeCommand("CIA461Decimals", DataType.U08, "6013/01"));
  register(makeCommand("CIA461Unit", DataType.U32, "6015/01", 16, 8));
  register(makeCommand("CIA461WeightStatus", DataType.U16, "6012/01"));
  register(makeCommand("CIA461WeightStatusGeneralWeightError", DataType.Bit, "6012/01", 0, 1));
  register(makeCommand("CIA461WeightStatusScaleAlarm", DataType.Bit, "6012/01", 1, 1));
  register(makeCommand("CIA461WeightStatusLimitStatus", DataType.Bit, "6012/01", 2, 2));
  register(makeCommand("CIA461WeightStatusWeightMoving", DataType.Bit, "6012/01", 4, 1));
  register(makeCommand("CIA461WeightStatusScaleSealIsOpen", DataType.Bit, "6012/01", 5, 1));
  register(makeCommand("CIA461WeightStatusManualTare", DataType.Bit, "6012/01", 6, 1));
  register(makeCommand("CIA461WeightStatusWeightType", DataType.Bit, "6012/01", 7, 1));
  register(makeCommand("CIA461WeightStatusScaleRange", DataType.Bit, "6012/01", 8, 2));
  register(makeCommand("CIA461WeightStatusZeroRequired", DataType.Bit, "6012/01", 10, 1));
  register(makeCommand("CIA461WeightStatusCenterOfZero", DataType.Bit, "6012/01", 11, 1));
  register(makeCommand("CIA461WeightStatusInsideZero", DataType.Bit, "6012/01", 12, 1));
  register(makeCommand("CIA461ScaleCommand", DataType.U32, "6002/01"));
  register(makeCommand("CIA461ScaleCommandStatus", DataType.U32, "6002/02"));
  register(makeCommand("CIA461ScaleMaximumCapacity", DataType.S32, "6113/01"));
  register(makeCommand("CIA461MultiIntervalRangeControl", DataType.U08, "611C/01"));
  register(makeCommand("CIA461MultiLimit1", DataType.S32, "611C/02"));
  register(makeCommand("CIA461MultiLimit2", DataType.S32, "611C/03"));
  register(makeCommand("CIA461WeightStep", DataType.U08, "6016/01"));
  register(makeCommand("CIA461ZeroValue", DataType.S32, "6142/00"));
  register(makeCommand("CIA461CalibrationWeight", DataType.S32, "6152/00"));
  register(makeCommand("CIA461SaveAllParameters", DataType.U32, "1010/01"));
  register(makeCommand("DSERestoreAllDefaultParameters", DataType.U32, "1011/03"));
  register(makeCommand("DSESerialNumber", DataType.U32, "4280/04"));
  register(makeCommand("DSEIdentification", DataType.Ascii, "1008/00"));
  register(makeCommand("DSEFirmwareVersion", DataType.Ascii, "100A/00"));
  register(makeCommand("DSEZeroSignal", DataType.S32, "6150/00"));
  register(makeCommand("DSENominalSignal", DataType.S32, "6151/00"));
  register(makeCommand("DSEFilterModeStage2", DataType.U32, "6040/02"));
  register(makeCommand("DSEFilterModeStage3", DataType.U32, "6040/03"));
  register(makeCommand("DSEFilterModeStage4", DataType.U32, "6040/04"));
  register(makeCommand("DSEFilterModeStage5", DataType.U32, "6040/05"));
  register(makeCommand("DSECombFilterFrequencyStage2", DataType.U32, "3321/00"));
  register(makeCommand("DSECombFilterFrequencyStage3", DataType.U32, "3322/00"));
  register(makeCommand("DSECombFilterFrequencyStage4", DataType.U32, "3323/00"));
  register(makeCommand("DSECombFilterFrequencyStage5", DataType.U32, "3324/00"));
  register(makeCommand("DSEMovAvFilterFrequencyStage2", DataType.U32, "3331/00"));
  register(makeCommand("DSEMovAvFilterFrequencyStage3", DataType.U32, "3332/00"));
  register(makeCommand("DSEMovAvFilterFrequencyStage4", DataType.U32, "3333/00"));
  register(makeCommand("DSEMovAvFilterFrequencyStage5", DataType.U32, "3334/00"));
  register(makeCommand("LDWZeroValue", DataType.S32, "2110/06"));
  register(makeCommand("LWTNominalValue", DataType.S32, "2110/07"));
  register(makeCommand("IMDApplicationMode", DataType.U08, "2010/07"));
  register(makeCommand("STORecordWeight", DataType.U08, "2040/05"));
}

setupRegistry();

function lookup(name: string): Command {
  const command = byName.get(name);
  if (!command) {
    throw new JetBusError(`Unknown command: ${name}`);
  }
  return command;
}

export const commands = {
  cia461NetValue: () => lookup("CIA461NetValue"),
  cia461GrossValue: () => lookup("CIA461GrossValue"),
  cia461TareValue: () => lookup("CIA461TareValue"),
  cia461Decimals: () => lookup("CIA461Decimals"),
  cia461Unit: () => lookup("CIA461Unit"),
  cia461WeightStatus: () => lookup("CIA461WeightStatus"),
  cia461WeightStatusGeneralWeightError: () => lookup("CIA461WeightStatusGeneralWeightError"),
  cia461WeightStatusScaleAlarm: () => lookup("CIA461WeightStatusScaleAlarm"),
  cia461WeightStatusLimitStatus: () => lookup("CIA461WeightStatusLimitStatus"),
  cia461WeightStatusWeightMoving: () => lookup("CIA461WeightStatusWeightMoving"),
  cia461WeightStatusScaleSealIsOpen: () => lookup("CIA461WeightStatusScaleSealIsOpen"),
  cia461WeightStatusManualTare: () => lookup("CIA461WeightStatusManualTare"),
  cia461WeightStatusWeightType: () => lookup("CIA461WeightStatusWeightType"),
  cia461WeightStatusScaleRange: () => lookup("CIA461WeightStatusScaleRange"),
  cia461WeightStatusZeroRequired: () => lookup("CIA461WeightStatusZeroRequired"),
  cia461WeightStatusCenterOfZero: () => lookup("CIA461WeightStatusCenterOfZero"),
  cia461WeightStatusInsideZero: () => lookup("CIA461WeightStatusInsideZero"),
  cia461ScaleCommand: () => lookup("CIA461ScaleCommand"),
  cia461ScaleCommandStatus: () => lookup("CIA461ScaleCommandStatus"),
  cia461ScaleMaximumCapacity: () => lookup("CIA461ScaleMaximumCapacity"),
  cia461MultiIntervalRangeControl: () => lookup("CIA461MultiIntervalRangeControl"),
  cia461MultiLimit1: () => lookup("CIA461MultiLimit1"),
  cia461MultiLimit2: () => lookup("CIA461MultiLimit2"),
  cia461WeightStep: () => lookup("CIA461WeightStep"),
  cia461ZeroValue: () => lookup("CIA461ZeroValue"),
  cia461CalibrationWeight: () => lookup("CIA461CalibrationWeight"),
  cia461SaveAllParameters: () => lookup("CIA461SaveAllParameters"),
  dseRestoreDefaults: () => lookup("DSERestoreAllDefaultParameters"),
  dseSerialNumber: () => lookup("DSESerialNumber"),
  dseIdentification: () => lookup("DSEIdentification"),
  dseFirmwareVersion: () => lookup("DSEFirmwareVersion"),
  dseZeroSignal: () => lookup("DSEZeroSignal"),
  dseNominalSignal: () => lookup("DSENominalSignal"),
  dseFilterModeStage2: () => lookup("DSEFilterModeStage2"),
  dseFilterModeStage3: () => lookup("DSEFilterModeStage3"),
  dseFilterModeStage4: () => lookup("DSEFilterModeStage4"),
  dseFilterModeStage5: () => lookup("DSEFilterModeStage5"),
  dseCombFilterFrequencyStage2: () => lookup("DSECombFilterFrequencyStage2"),
  dseCombFilterFrequencyStage3: () => lookup("DSECombFilterFrequencyStage3"),
  dseCombFilterFrequencyStage4: () => lookup("DSECombFilterFrequencyStage4"),
  dseCombFilterFrequencyStage5: () => lookup("DSECombFilterFrequencyStage5"),
  dseMovAvFilterFrequencyStage2: () => lookup("DSEMovAvFilterFrequencyStage2"),
  dseMovAvFilterFrequencyStage3: () => lookup("DSEMovAvFilterFrequencyStage3"),
  dseMovAvFilterFrequencyStage4: () => lookup("DSEMovAvFilterFrequencyStage4"),
  dseMovAvFilterFrequencyStage5: () => lookup("DSEMovAvFilterFrequencyStage5"),
  ldwZeroValue: () => lookup("LDWZeroValue"),
  lwtNominalValue: () => lookup("LWTNominalValue"),
  stoRecordWeight: () => lookup("STORecordWeight"),
  imdApplicationMode: () => lookup("IMDApplicationMode"),
  cia461ScaleCommandStatusRaw: () => lookup("CIA461ScaleCommandStatus")
};

export function allCommands(): readonly Command[] {
  return registry;
}

export function findByPath(path: string): Command | undefined {
  const commands = byPath.get(path);
  return commands?.[0];
}
