import type { DeviceCommand, DeviceEvent } from "../../shared/device-protocol";

export type TransportKind = "wifi" | "bluetooth" | "serial";

export interface TransportInfo {
  kind: TransportKind;
  /** Human-readable label, np. "WiFi (Flower-AB12)" / "Bluetooth" / "USB" */
  label: string;
}

export interface DeviceLink {
  readonly transport: TransportInfo;
  readonly connected: boolean;
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  send(cmd: DeviceCommand): Promise<void>;
  onEvent(handler: (ev: DeviceEvent) => void): () => void;
  /**
   * Wywoływane, gdy stan połączenia zmieni się SAM (np. WiFi padło albo
   * auto-reconnect się udał) — nie przy jawnym connect()/disconnect()
   * wywołanym z UI. Pozwala appce pokazać "rozłączono" zamiast cicho
   * zawiesić się w stanie "połączono" z martwym linkiem.
   */
  onStatusChange(handler: (connected: boolean) => void): () => void;
}

export type LinkFactory = () => DeviceLink;
