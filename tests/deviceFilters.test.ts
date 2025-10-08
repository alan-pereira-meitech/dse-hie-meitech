import { beforeEach, describe, expect, it } from "vitest";
import { Device } from "../src/dse/device";
import { FilterType } from "../src/dse/types";
import { commands } from "../src/jetbus/commands";

class FakeJetBusClient {
  public readonly cache = new Map<string, string>();
  public readonly setCalls: Array<{ path: string; value: number }> = [];

  connect(): void {}

  disconnect(): void {}

  setDataCallback(): void {}

  setFetchCallback(): void {}

  snapshot(): Map<string, string> {
    return new Map(this.cache);
  }

  readCached(path: string): string | undefined {
    return this.cache.get(path);
  }

  async set(path: string, value: unknown): Promise<void> {
    const numeric = typeof value === "number" ? value : Number(value);
    this.setCalls.push({ path, value: numeric });
    this.cache.set(path, String(value));
  }

  async fetch(): Promise<string> {
    return "token";
  }

  async unfetch(): Promise<void> {}

  async waitFor(): Promise<boolean> {
    return true;
  }
}

function createDevice(fakeClient: FakeJetBusClient): Device {
  const device = new Device({
    clientOptions: { url: "ws://localhost" },
    autoFetchProcessData: false
  });
  (device as unknown as { client: FakeJetBusClient }).client = fakeClient;
  return device;
}

describe("Device digital filters", () => {
  let fakeClient: FakeJetBusClient;
  let device: Device;

  beforeEach(() => {
    fakeClient = new FakeJetBusClient();
    FILTER_STAGE_PATHS.forEach((path) => {
      fakeClient.cache.set(path, "0");
    });
    device = createDevice(fakeClient);
  });

  it("normalizes stored filter codes when reading stage mode", async () => {
    fakeClient.cache.set(commands.dseFilterModeStage2().path, "13090");

    const mode = await device.filterStage2Mode();

    expect(mode).toBe(FilterType.FIRCombFilter);
  });

  it("selects the lowest available code for the desired filter type", async () => {
    fakeClient.cache.set(commands.dseFilterModeStage2().path, "13089");
    fakeClient.cache.set(commands.dseFilterModeStage4().path, "13090");

    await device.setFilterStage3Mode(FilterType.FIRCombFilter);

    expect(fakeClient.setCalls.at(-1)).toEqual({
      path: commands.dseFilterModeStage3().path,
      value: 13091
    });
  });

  it("returns the configured cut-off frequency for moving average filters", async () => {
    fakeClient.cache.set(commands.dseFilterModeStage4().path, "13107");
    fakeClient.cache.set(commands.dseMovAvFilterFrequencyStage4().path, "120");

    const frequency = await device.filterCutOffFrequencyStage4();

    expect(frequency).toBe(120);
  });

  it("writes the comb filter frequency register when applicable", async () => {
    fakeClient.cache.set(commands.dseFilterModeStage2().path, "13089");

    await device.setFilterCutOffFrequencyStage2(200);

    expect(fakeClient.setCalls.at(-1)).toEqual({
      path: commands.dseCombFilterFrequencyStage2().path,
      value: 200
    });
  });

  it("returns zero frequency when no filter is active", async () => {
    const frequency = await device.filterCutOffFrequencyStage3();

    expect(frequency).toBe(0);
  });

  it("aggregates the configuration of all filter stages", async () => {
    fakeClient.cache.set(commands.dseFilterModeStage2().path, "13089");
    fakeClient.cache.set(commands.dseCombFilterFrequencyStage2().path, "180");
    fakeClient.cache.set(commands.dseFilterModeStage3().path, "13105");
    fakeClient.cache.set(commands.dseMovAvFilterFrequencyStage3().path, "90");
    fakeClient.cache.set(commands.dseFilterModeStage5().path, "13108");
    fakeClient.cache.set(commands.dseMovAvFilterFrequencyStage5().path, "60");

    const configuration = await device.filterStagesConfiguration();

    expect(configuration).toEqual([
      { stage: 2, mode: FilterType.FIRCombFilter, cutOffFrequency: 180 },
      { stage: 3, mode: FilterType.FIRMovingAverage, cutOffFrequency: 90 },
      { stage: 4, mode: FilterType.NoFilter, cutOffFrequency: 0 },
      { stage: 5, mode: FilterType.FIRMovingAverage, cutOffFrequency: 60 }
    ]);
  });

  it("updates mode and frequency in a single call", async () => {
    await device.configureFilterStage(3, {
      mode: FilterType.FIRMovingAverage,
      cutOffFrequency: 120
    });

    const lastCalls = fakeClient.setCalls.slice(-2);

    expect(lastCalls[0]).toEqual({
      path: commands.dseFilterModeStage3().path,
      value: 13105
    });
    expect(lastCalls[1]).toEqual({
      path: commands.dseMovAvFilterFrequencyStage3().path,
      value: 120
    });
  });
});

const FILTER_STAGE_PATHS = [
  commands.dseFilterModeStage2().path,
  commands.dseFilterModeStage3().path,
  commands.dseFilterModeStage4().path,
  commands.dseFilterModeStage5().path
];
