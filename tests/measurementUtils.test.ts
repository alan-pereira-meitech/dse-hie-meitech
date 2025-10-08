import { describe, expect, it } from "vitest";
import { digitToDouble, doubleToDigit, stringToBool } from "../src/jetbus/measurementUtils";

describe("measurementUtils", () => {
  it("converts digits to doubles respecting decimals", () => {
    expect(digitToDouble(12345, 2)).toBeCloseTo(123.45, 6);
    expect(digitToDouble(-9876, 3)).toBeCloseTo(-9.876, 6);
  });

  it("converts doubles to digits respecting decimals", () => {
    expect(doubleToDigit(123.45, 2)).toBe(12345);
    expect(doubleToDigit(-9.876, 3)).toBe(-9876);
  });

  it("parses string to boolean", () => {
    expect(stringToBool("0")).toBe(false);
    expect(stringToBool("1")).toBe(true);
    expect(stringToBool("anything")).toBe(true);
  });
});
