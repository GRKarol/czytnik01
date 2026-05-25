/**
 * Protokół komunikacji z urządzeniem czytnik01.
 *
 * Tutaj zdefiniuj polecenia i format ramek, kiedy będzie znany firmware.
 * Na razie szkielet z miejscami na rozszerzenie.
 *
 * Sugerowany format: JSON Lines po Web Serial @ 115200 baud
 *   ↓ host → device:   {"cmd": "ping"}\n
 *   ↑ device → host:   {"ev": "pong", "ts": 12345}\n
 */

export interface DeviceCommand {
  cmd: string;
  [key: string]: unknown;
}

export interface DeviceEvent {
  ev: string;
  [key: string]: unknown;
}

export const PING: DeviceCommand = { cmd: "ping" };

export function encodeCommand(cmd: DeviceCommand): Uint8Array {
  return new TextEncoder().encode(JSON.stringify(cmd) + "\n");
}

export function parseEvent(line: string): DeviceEvent | null {
  const trimmed = line.trim();
  if (!trimmed) return null;
  try {
    const obj = JSON.parse(trimmed);
    if (typeof obj === "object" && obj !== null && typeof obj.ev === "string") {
      return obj as DeviceEvent;
    }
  } catch {
    /* fall through */
  }
  return null;
}
