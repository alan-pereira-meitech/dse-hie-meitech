import type { Device } from './device.js';

export type DeviceFunctionKind = 'getter' | 'command' | 'setter';

export interface DeviceFunctionInput {
  name: string;
  label: string;
  type: 'string' | 'number';
  placeholder?: string;
}

export interface DeviceFunctionMeta {
  name: string;
  label: string;
  description: string;
  kind: DeviceFunctionKind;
  method: keyof Device;
  inputs?: DeviceFunctionInput[];
}

export const DEVICE_FUNCTIONS: DeviceFunctionMeta[] = [
  {
    name: 'net_weight',
    label: 'Net Weight',
    description: 'Peso líquido atual.',
    kind: 'getter',
    method: 'netWeight'
  },
  {
    name: 'gross_weight',
    label: 'Gross Weight',
    description: 'Peso bruto atual.',
    kind: 'getter',
    method: 'grossWeight'
  },
  {
    name: 'tare_weight',
    label: 'Tare Weight',
    description: 'Valor atual da tara.',
    kind: 'getter',
    method: 'tareWeight'
  },
  {
    name: 'unit',
    label: 'Unit',
    description: 'Unidade configurada.',
    kind: 'getter',
    method: 'unit'
  },
  {
    name: 'decimals',
    label: 'Decimals',
    description: 'Quantidade de casas decimais.',
    kind: 'getter',
    method: 'decimals'
  },
  {
    name: 'tare_mode',
    label: 'Tare Mode',
    description: 'Modo de tara calculado.',
    kind: 'getter',
    method: 'tareMode'
  },
  {
    name: 'weight_stable',
    label: 'Weight Stable',
    description: 'Indica se o peso está estável.',
    kind: 'getter',
    method: 'weightStable'
  },
  {
    name: 'zero_required',
    label: 'Zero Required',
    description: 'Sinaliza necessidade de zeragem.',
    kind: 'getter',
    method: 'zeroRequired'
  },
  {
    name: 'center_of_zero',
    label: 'Center of Zero',
    description: 'Indica se está no centro do zero.',
    kind: 'getter',
    method: 'centerOfZero'
  },
  {
    name: 'inside_zero',
    label: 'Inside Zero',
    description: 'Indica se está dentro da zona de zero.',
    kind: 'getter',
    method: 'insideZero'
  },
  {
    name: 'legal_for_trade',
    label: 'Legal for Trade',
    description: 'Sinalização de uso legal.',
    kind: 'getter',
    method: 'legalForTrade'
  },
  {
    name: 'underload',
    label: 'Underload',
    description: 'Indica condição de carga abaixo do mínimo.',
    kind: 'getter',
    method: 'underload'
  },
  {
    name: 'overload',
    label: 'Overload',
    description: 'Indica condição de sobrecarga.',
    kind: 'getter',
    method: 'overload'
  },
  {
    name: 'higher_safe_load_limit',
    label: 'Higher Safe Load Limit',
    description: 'Estado de carga acima do limite seguro.',
    kind: 'getter',
    method: 'higherSafeLoadLimit'
  },
  {
    name: 'general_scale_error',
    label: 'General Scale Error',
    description: 'Erro geral reportado pela balança.',
    kind: 'getter',
    method: 'generalScaleError'
  },
  {
    name: 'scale_alarm',
    label: 'Scale Alarm',
    description: 'Alarme da balança.',
    kind: 'getter',
    method: 'scaleAlarm'
  },
  {
    name: 'weight_step',
    label: 'Weight Step',
    description: 'Resolução de peso em dígitos.',
    kind: 'getter',
    method: 'weightStep'
  },
  {
    name: 'scale_range',
    label: 'Scale Range',
    description: 'Faixa de escala atual.',
    kind: 'getter',
    method: 'scaleRange'
  },
  {
    name: 'maximum_capacity',
    label: 'Maximum Capacity',
    description: 'Capacidade máxima configurada.',
    kind: 'getter',
    method: 'maximumCapacity'
  },
  {
    name: 'zero_value',
    label: 'Zero Value',
    description: 'Valor do zero em dígitos.',
    kind: 'getter',
    method: 'zeroValue'
  },
  {
    name: 'zero_signal',
    label: 'Zero Signal',
    description: 'Sinal de zero em mV/V.',
    kind: 'getter',
    method: 'zeroSignal'
  },
  {
    name: 'nominal_signal',
    label: 'Nominal Signal',
    description: 'Sinal nominal em mV/V.',
    kind: 'getter',
    method: 'nominalSignal'
  },
  {
    name: 'identification',
    label: 'Identification',
    description: 'Identificação do dispositivo.',
    kind: 'getter',
    method: 'identification'
  },
  {
    name: 'firmware_version',
    label: 'Firmware Version',
    description: 'Versão de firmware.',
    kind: 'getter',
    method: 'firmwareVersion'
  },
  {
    name: 'serial_number',
    label: 'Serial Number',
    description: 'Número de série do dispositivo.',
    kind: 'getter',
    method: 'serialNumber'
  },
  {
    name: 'set_unit',
    label: 'Set Unit',
    description: 'Define a unidade da balança.',
    kind: 'setter',
    method: 'setUnit',
    inputs: [
      { name: 'unitCode', label: 'Código da unidade (kg, g, t, lb, N)', type: 'string', placeholder: 'kg' }
    ]
  },
  {
    name: 'set_manual_tare',
    label: 'Set Manual Tare',
    description: 'Configura manualmente a tara.',
    kind: 'setter',
    method: 'setManualTare',
    inputs: [
      { name: 'value', label: 'Valor da tara', type: 'number', placeholder: '0.0' }
    ]
  },
  {
    name: 'set_maximum_capacity',
    label: 'Set Maximum Capacity',
    description: 'Atualiza a capacidade máxima.',
    kind: 'setter',
    method: 'setMaximumCapacity',
    inputs: [
      { name: 'value', label: 'Capacidade', type: 'number', placeholder: '1000' }
    ]
  },
  {
    name: 'set_zero_signal',
    label: 'Set Zero Signal',
    description: 'Define o sinal de zero.',
    kind: 'setter',
    method: 'setZeroSignal',
    inputs: [
      { name: 'value', label: 'Zero (mV/V)', type: 'number', placeholder: '0' }
    ]
  },
  {
    name: 'set_nominal_signal',
    label: 'Set Nominal Signal',
    description: 'Define o sinal nominal.',
    kind: 'setter',
    method: 'setNominalSignal',
    inputs: [
      { name: 'value', label: 'Nominal (mV/V)', type: 'number', placeholder: '2' }
    ]
  },
  {
    name: 'save_all_parameters',
    label: 'Save All Parameters',
    description: 'Salva todos os parâmetros na memória.',
    kind: 'command',
    method: 'saveAllParameters'
  },
  {
    name: 'restore_default_parameters',
    label: 'Restore Default Parameters',
    description: 'Restaura parâmetros de fábrica.',
    kind: 'command',
    method: 'restoreDefaultParameters'
  },
  {
    name: 'zero',
    label: 'Zero',
    description: 'Executa o comando de zeragem.',
    kind: 'command',
    method: 'zero'
  },
  {
    name: 'tare',
    label: 'Tare',
    description: 'Executa o comando de tara.',
    kind: 'command',
    method: 'tare'
  },
  {
    name: 'set_gross',
    label: 'Set Gross',
    description: 'Retorna para modo bruto.',
    kind: 'command',
    method: 'setGross'
  },
  {
    name: 'record_weight',
    label: 'Record Weight',
    description: 'Registra o peso atual.',
    kind: 'command',
    method: 'recordWeight'
  },
  {
    name: 'adjust_zero_signal',
    label: 'Adjust Zero Signal',
    description: 'Inicia ajuste automático do zero.',
    kind: 'command',
    method: 'adjustZeroSignal'
  },
  {
    name: 'adjust_nominal_signal',
    label: 'Adjust Nominal Signal',
    description: 'Inicia ajuste automático nominal.',
    kind: 'command',
    method: 'adjustNominalSignal'
  },
  {
    name: 'adjust_nominal_signal_with_calibration_weight',
    label: 'Adjust Nominal Signal (with weight)',
    description: 'Ajuste nominal informando peso de calibração.',
    kind: 'command',
    method: 'adjustNominalSignalWithCalibrationWeight',
    inputs: [
      { name: 'weight', label: 'Peso de calibração', type: 'number', placeholder: '100' }
    ]
  },
  {
    name: 'calculate_adjustment',
    label: 'Calculate Adjustment',
    description: 'Calcula ajuste usando valores em mV/V.',
    kind: 'command',
    method: 'calculateAdjustment',
    inputs: [
      { name: 'scaleZeroMvv', label: 'Zero (mV/V)', type: 'number', placeholder: '0' },
      { name: 'capacityMvv', label: 'Capacidade (mV/V)', type: 'number', placeholder: '2' }
    ]
  }
];
