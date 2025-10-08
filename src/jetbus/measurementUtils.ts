export function digitToDouble(value: number, decimals: number): number {
  const factor = Math.pow(10, decimals);
  return value / factor;
}

export function doubleToDigit(value: number, decimals: number): number {
  const factor = Math.pow(10, decimals);
  return Math.round(value * factor);
}

export function stringToBool(value: string): boolean {
  return value !== '0';
}
