export enum ApplicationMode {
  Standard = 0,
  Checkweigher = 1,
  Filler = 2
}

export enum TareMode {
  None = "None",
  Tare = "Tare",
  PresetTare = "PresetTare"
}

export interface WeightValues {
  net: number;
  gross: number;
  tare: number;
}

export interface PrintableWeightValues {
  net: string;
  gross: string;
  tare: string;
}

export enum FilterType {
  NoFilter = 0,
  FIRCombFilter = 13089,
  FIRMovingAverage = 13105
}
