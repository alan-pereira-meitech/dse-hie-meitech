export type LogLevel = "info" | "success" | "error";

export interface LogEntry {
  id: string;
  level: LogLevel;
  title: string;
  message: string;
  timestamp: string;
}
