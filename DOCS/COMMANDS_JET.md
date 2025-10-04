# DSE Jet Command Map

The table below summarises the Jet/JetBus paths referenced by the legacy DSE implementation. Types are taken from `Hbm.Automation.API/Weighing/DSE/Jet/JetBusCommands.cs` and related classes. Access denotes the intended direction from a controller perspective (`R` = read, `W` = write).

| Path | Key / Semantic | Type | Access | Notes |
| ---- | -------------- | ---- | ------ | ----- |
| `6002/01` | `CIA461ScaleCommand` | `U32` | W | Command channel used for tare (`1701994868`), zero (`1869768058`), set gross (`1936683623`), calibration (`2053923171`, `1852596579`).
| `6002/02` | `CIA461ScaleCommandStatus` | `U32` | R | Returns status (`1634168417` = ongoing, `1801543519` = OK, other magic numbers indicate errors). Monitor until command completes.
| `6012/01` | `CIA461WeightStatus` | `U16` | R | Bit-coded weight status (centre of zero, motion, alarms, tare, range). Sub-paths `6012/01` bits map to separate JetBusCommands entries.
| `6013/01` | `CIA461Decimals` | `U08` | R | Decimal places used to convert integer digits to human readable values.
| `6014/01` | `CIA461UnitPrefixFixedParameters` | `U32` | R | Contains encoded unit/prefix flags.
| `6015/01` | `CIA461Unit` | `U32` | R/W | Unit selection (`0x004C0000` = t, `0x00020000` = kg, `0x004B0000` = g, `0x00A60000` = lb, `0x00210000` = N).
| `6016/01` | `CIA461WeightStep` | `U08` | R/W | Resolution in digits. Combined with decimals to format printable weight.
| `601A/01` | `CIA461OutputWeight` | `S32` | R | Net weight output value (scaled digits).
| `6113/01` | `CIA461ScaleMaximumCapacity` | `S32` | R/W | Maximum capacity of the scale.
| `611C/01` | `CIA461MultiIntervalRangeControl` | `U08` | R/W | Enables multi-interval ranges (0..3). Values align with CIA-461 profile.
| `611C/02` | `CIA461MultiLimit1` | `S32` | R/W | First transition limit in digits.
| `611C/03` | `CIA461MultiLimit2` | `S32` | R/W | Second transition limit in digits.
| `6142/00` | `CIA461ZeroValue` / `DSEZeroValue` | `S32` | R/W | Stored zero point.
| `6143/00` | `CIA461TareValue` | `S32` | R/W | Stored tare value (manual tare register).
| `6144/00` | `CIA461GrossValue` | `S32` | R | Gross weight in digits.
| `6152/00` | `CIA461CalibrationWeight` | `S32` | R/W | Calibration reference weight (digits).
| `6153/00` | `CIA461WeightMovingDetection` | `U08` | R/W | Motion detector enable threshold.
| `6020/01` | `CIA461ScaleSettings` | `S32` | R/W | Various configuration bits (range, mode). See CIA-461 specification.
| `6021/01` | `CIA461LocalGravityFactor` | `S32` | R/W | Gravity correction in ppm.
| `6050/01` | `CIA461SampleRate` | `U32` | R/W | Acquisition rate (samples per second).
| `6040/01` | `CIA461ScaleFilter` | `U16` | R/W | Selects low-pass filter family (critically damped, Bessel, Butterworth).
| `6040/02`..`6040/05` | `DSEFilterModeStage[2-5]` | `U32` | R/W | Select filter topology per stage (`0`=none, `13089`=FIR comb, `13105`=FIR moving average).
| `60A1/01` | `CIA461FilterCriticallyDampedFilterOrder` | `U08` | R/W | Filter order for critically damped low-pass.
| `60A1/02` | `DSELowPassCutOffFrequencyIIR` | `U32` | R/W | IIR cut-off frequency (Hz) when `LowPassFilterMode=IIR`.
| `3311/02` | `DSELowPassCutOffFrequencyFIR` | `U32` | R/W | FIR cut-off frequency (Hz) for stage 1.
| `3321/00`..`3324/00` | `DSECombFilterFrequencyStage[2-5]` | `U32` | R/W | Comb filter frequency selection for stages 2-5.
| `3331/00`..`3334/00` | `DSEMovAvFilterFrequencyStage[2-5]` | `U32` | R/W | Moving average window length for stages 2-5.
| `2110/06` | `LDWZeroValue` | `S32` | R/W | Zero signal of load cell (digits, mV/V scaled by `1e6`).
| `2110/07` | `LWTNominalValue` | `S32` | R/W | Nominal (full load) signal of load cell (digits, mV/V scaled by `1e6`).
| `2210/03` | `EWTEmptyWeight` | `S32` | R/W | Empty weight for batching applications.
| `2230/05` | `NDSFillingCounter` | `U16` | R | Filling cycle counter.
| `2240/02` | `RUNStartFilling` | `NIL` | W | Trigger start of filling cycle.

> **TODO**: The repository does not include the original Jet protocol reference frames. Validate JSON field names (`type`, `method`, `params`, `value`) against official Jet/JetBus documentation before deploying to production systems.

