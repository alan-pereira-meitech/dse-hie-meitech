const deviceUrl = process.env.DEVICE_WS_URL ?? "ws://192.168.1.243/jet/canopen";
const port = Number.parseInt(process.env.PORT ?? "3000", 10);
const enableDebugLogs = String(process.env.JETBUS_DEBUG ?? "").toLowerCase() === "true";

export const config = {
  deviceUrl,
  port,
  enableDebugLogs
};
