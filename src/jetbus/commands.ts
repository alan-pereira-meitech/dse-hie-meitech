import { DataType, JetBusError } from './types.js';

export interface Command {
  name: string;
  type: DataType;
  path: string;
  bitIndex: number;
  bitLength: number;
  toInt(raw: string): number;
  toString(raw: string): string;
}

function extractBit(value: number, bitIndex: number, bitLength: number): number {
  let bitMask = 0xffff;
  switch (bitLength) {
    case 0:
      bitMask = 0xffff;
      break;
    case 1:
      bitMask = 0x1;
      break;
    case 2:
      bitMask = 0x3;
      break;
    case 3:
      bitMask = 0x7;
      break;
    case 4:
      bitMask = 0xf;
      break;
    default:
      bitMask = (1 << bitLength) - 1;
      break;
  }
  const mask = bitMask << bitIndex;
  return (value & mask) >> bitIndex;
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
    toInt(raw: string) {
      if (type === DataType.Bit) {
        const numeric = Number.parseInt(raw, 10);
        if (Number.isNaN(numeric)) {
          return 0;
        }
        return extractBit(numeric, bitIndex, bitLength);
      }
      const value = Number.parseInt(raw, 10);
      if (Number.isNaN(value)) {
        throw new JetBusError(`Cannot convert value "${raw}" to integer for command ${name}`);
      }
      return value;
    },
    toString(raw: string) {
      if (type === DataType.Bit) {
        return String(this.toInt(raw));
      }
      return raw;
    }
  };
}

const registry: Command[] = [
  makeCommand('CIA461NetValue', DataType.S32, '601A/01'),
  makeCommand('CIA461GrossValue', DataType.S32, '6144/00'),
  makeCommand('CIA461TareValue', DataType.S32, '6143/00'),
  makeCommand('CIA461Decimals', DataType.U08, '6013/01'),
  makeCommand('CIA461Unit', DataType.U32, '6015/01', 16, 8),
  makeCommand('CIA461WeightStatus', DataType.U16, '6012/01'),
  makeCommand('CIA461WeightStatusGeneralWeightError', DataType.Bit, '6012/01', 0, 1),
  makeCommand('CIA461WeightStatusScaleAlarm', DataType.Bit, '6012/01', 1, 1),
  makeCommand('CIA461WeightStatusLimitStatus', DataType.Bit, '6012/01', 2, 2),
  makeCommand('CIA461WeightStatusWeightMoving', DataType.Bit, '6012/01', 4, 1),
  makeCommand('CIA461WeightStatusScaleSealIsOpen', DataType.Bit, '6012/01', 5, 1),
  makeCommand('CIA461WeightStatusManualTare', DataType.Bit, '6012/01', 6, 1),
  makeCommand('CIA461WeightStatusWeightType', DataType.Bit, '6012/01', 7, 1),
  makeCommand('CIA461WeightStatusScaleRange', DataType.Bit, '6012/01', 8, 2),
  makeCommand('CIA461WeightStatusZeroRequired', DataType.Bit, '6012/01', 10, 1),
  makeCommand('CIA461WeightStatusCenterOfZero', DataType.Bit, '6012/01', 11, 1),
  makeCommand('CIA461WeightStatusInsideZero', DataType.Bit, '6012/01', 12, 1),
  makeCommand('CIA461ScaleCommand', DataType.U32, '6002/01'),
  makeCommand('CIA461ScaleCommandStatus', DataType.U32, '6002/02'),
  makeCommand('CIA461ScaleMaximumCapacity', DataType.S32, '6113/01'),
  makeCommand('CIA461MultiIntervalRangeControl', DataType.U08, '611C/01'),
  makeCommand('CIA461MultiLimit1', DataType.S32, '611C/02'),
  makeCommand('CIA461MultiLimit2', DataType.S32, '611C/03'),
  makeCommand('CIA461WeightStep', DataType.U08, '6016/01'),
  makeCommand('CIA461ZeroValue', DataType.S32, '6142/00'),
  makeCommand('CIA461CalibrationWeight', DataType.S32, '6152/00'),
  makeCommand('CIA461SaveAllParameters', DataType.U32, '1010/01'),
  makeCommand('DSERestoreAllDefaultParameters', DataType.U32, '1011/03'),
  makeCommand('DSESerialNumber', DataType.U32, '4280/04'),
  makeCommand('DSEIdentification', DataType.Ascii, '1008/00'),
  makeCommand('DSEFirmwareVersion', DataType.Ascii, '100A/00'),
  makeCommand('DSEZeroSignal', DataType.S32, '6150/00'),
  makeCommand('DSENominalSignal', DataType.S32, '6151/00'),
  makeCommand('LDWZeroValue', DataType.S32, '2110/06'),
  makeCommand('LWTNominalValue', DataType.S32, '2110/07'),
  makeCommand('IMDApplicationMode', DataType.U08, '2010/07'),
  makeCommand('STORecordWeight', DataType.U08, '2040/05'),
  makeCommand('CIA461ScaleCommandStatusRaw', DataType.U32, '6002/02')
];

const byName = new Map<string, Command>();
const byPath = new Map<string, Command>();

for (const command of registry) {
  byName.set(command.name, command);
  if (!byPath.has(command.path)) {
    byPath.set(command.path, command);
  }
}

function lookup(name: string): Command {
  const command = byName.get(name);
  if (!command) {
    throw new JetBusError(`Unknown command: ${name}`);
  }
  return command;
}

export const commands = {
  cia461NetValue: () => lookup('CIA461NetValue'),
  cia461GrossValue: () => lookup('CIA461GrossValue'),
  cia461TareValue: () => lookup('CIA461TareValue'),
  cia461Decimals: () => lookup('CIA461Decimals'),
  cia461Unit: () => lookup('CIA461Unit'),
  cia461WeightStatus: () => lookup('CIA461WeightStatus'),
  cia461WeightStatusGeneralWeightError: () => lookup('CIA461WeightStatusGeneralWeightError'),
  cia461WeightStatusScaleAlarm: () => lookup('CIA461WeightStatusScaleAlarm'),
  cia461WeightStatusLimitStatus: () => lookup('CIA461WeightStatusLimitStatus'),
  cia461WeightStatusWeightMoving: () => lookup('CIA461WeightStatusWeightMoving'),
  cia461WeightStatusScaleSealIsOpen: () => lookup('CIA461WeightStatusScaleSealIsOpen'),
  cia461WeightStatusManualTare: () => lookup('CIA461WeightStatusManualTare'),
  cia461WeightStatusWeightType: () => lookup('CIA461WeightStatusWeightType'),
  cia461WeightStatusScaleRange: () => lookup('CIA461WeightStatusScaleRange'),
  cia461WeightStatusZeroRequired: () => lookup('CIA461WeightStatusZeroRequired'),
  cia461WeightStatusCenterOfZero: () => lookup('CIA461WeightStatusCenterOfZero'),
  cia461WeightStatusInsideZero: () => lookup('CIA461WeightStatusInsideZero'),
  cia461ScaleCommand: () => lookup('CIA461ScaleCommand'),
  cia461ScaleCommandStatus: () => lookup('CIA461ScaleCommandStatus'),
  cia461ScaleMaximumCapacity: () => lookup('CIA461ScaleMaximumCapacity'),
  cia461MultiIntervalRangeControl: () => lookup('CIA461MultiIntervalRangeControl'),
  cia461MultiLimit1: () => lookup('CIA461MultiLimit1'),
  cia461MultiLimit2: () => lookup('CIA461MultiLimit2'),
  cia461WeightStep: () => lookup('CIA461WeightStep'),
  cia461ZeroValue: () => lookup('CIA461ZeroValue'),
  cia461CalibrationWeight: () => lookup('CIA461CalibrationWeight'),
  cia461SaveAllParameters: () => lookup('CIA461SaveAllParameters'),
  dseRestoreDefaults: () => lookup('DSERestoreAllDefaultParameters'),
  dseSerialNumber: () => lookup('DSESerialNumber'),
  dseIdentification: () => lookup('DSEIdentification'),
  dseFirmwareVersion: () => lookup('DSEFirmwareVersion'),
  dseZeroSignal: () => lookup('DSEZeroSignal'),
  dseNominalSignal: () => lookup('DSENominalSignal'),
  ldwZeroValue: () => lookup('LDWZeroValue'),
  lwtNominalValue: () => lookup('LWTNominalValue'),
  stoRecordWeight: () => lookup('STORecordWeight'),
  imdApplicationMode: () => lookup('IMDApplicationMode'),
  cia461ScaleCommandStatusRaw: () => lookup('CIA461ScaleCommandStatusRaw')
} as const;

export function allCommands(): Command[] {
  return registry.slice();
}

export function findByPath(path: string): Command | undefined {
  return byPath.get(path);
}
