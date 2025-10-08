export enum DataType {
  Nil = "nil",
  Bit = "bit",
  U08 = "u8",
  U16 = "u16",
  U32 = "u32",
  S16 = "s16",
  S32 = "s32",
  Ascii = "ascii"
}

export enum JetEventType {
  Add = "add",
  Fetch = "fetch",
  Change = "change",
  Remove = "remove",
  Unknown = "unknown"
}

export function eventTypeFromString(value: string): JetEventType {
  switch (value) {
    case "add":
      return JetEventType.Add;
    case "fetch":
      return JetEventType.Fetch;
    case "change":
      return JetEventType.Change;
    case "remove":
      return JetEventType.Remove;
    default:
      return JetEventType.Unknown;
  }
}

export class JetBusError extends Error {
  constructor(message: string) {
    super(message);
    this.name = "JetBusError";
  }
}
