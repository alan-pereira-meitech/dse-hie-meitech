import { describe, expect, it } from "vitest";
import { ProcessData } from "../src/jetbus/processData";
import { commands } from "../src/jetbus/commands";
import { TareMode } from "../src/dse/types";

describe("ProcessData", () => {
  it("interprets cached values correctly", () => {
    const cache = new Map<string, string>();

    // Status base value
    let weightStatus = 0;
    weightStatus |= 1 << 1; // scale alarm
    weightStatus |= 2 << 2; // limit status -> overload
    weightStatus |= 1 << 6; // manual tare present
    weightStatus |= 0 << 7; // weight type manual
    weightStatus |= 1 << 10; // zero required
    weightStatus |= 1 << 11; // center of zero
    weightStatus |= 0 << 12; // inside zero false

    cache.set(commands.cia461WeightStatus().path, weightStatus.toString());
    cache.set(commands.cia461Decimals().path, "2");
    cache.set(commands.cia461Unit().path, (0x00020000).toString(10)); // kg
    cache.set(commands.cia461NetValue().path, "12345");
    cache.set(commands.cia461GrossValue().path, "15000");
    cache.set(commands.cia461TareValue().path, "2655");
    cache.set(commands.imdApplicationMode().path, "0");

    const processData = new ProcessData();
    processData.update(cache);

    expect(processData.weight().net).toBeCloseTo(123.45, 6);
    expect(processData.weight().gross).toBeCloseTo(150.0, 6);
    expect(processData.weight().tare).toBeCloseTo(26.55, 6);
    expect(processData.printableWeight().net).toBe("123.45");
    expect(processData.unit()).toBe("kg");
    expect(processData.decimals()).toBe(2);
    expect(processData.tareMode()).toBe(TareMode.Tare);
    expect(processData.weightStable()).toBe(true);
    expect(processData.zeroRequired()).toBe(true);
    expect(processData.centerOfZero()).toBe(true);
    expect(processData.insideZero()).toBe(false);
    expect(processData.legalForTrade()).toBe(true);
    expect(processData.overload()).toBe(true);
    expect(processData.underload()).toBe(false);
    expect(processData.generalScaleError()).toBe(false);
    expect(processData.scaleAlarm()).toBe(true);
  });
});
