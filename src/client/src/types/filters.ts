export type FilterMode = "NoFilter" | "FIRCombFilter" | "FIRMovingAverage";

export interface FilterStageConfig {
  stage: number;
  mode: FilterMode;
  cutOffFrequency: number;
}

export interface FiltersResponse {
  stages: FilterStageConfig[];
}
