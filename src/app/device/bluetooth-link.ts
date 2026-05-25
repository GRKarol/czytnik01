/**
 * Web Bluetooth transport — bonus dla Androida (Chrome).
 * NIE działa na iOS Safari (Apple nie wspiera Web Bluetooth od lat).
 *
 * UUID-y serwisu/charakterystyk są placeholderami — do uzgodnienia
 * z firmware. Komendy idą jako bajty na characteristic CMD, eventy
 * przychodzą jako notifications na characteristic EVENT.
 */

import { DEVICE_BLE_SERVICE_UUID } from "../../shared/config";
import type { DeviceCommand, DeviceEvent } from "../../shared/device-protocol";
import { encodeCommand, parseEvent } from "../../shared/device-protocol";
import type { DeviceLink, TransportInfo } from "./device-link";

// Placeholder UUID-y — dokończymy kiedy firmware będzie miał BLE.
const CMD_CHAR_UUID = "f10e7e11-f10e-7e10-f10e-7e10f10e7e10";
const EVT_CHAR_UUID = "f10e7e12-f10e-7e10-f10e-7e10f10e7e10";

export class BluetoothLink implements DeviceLink {
  private device: BluetoothDevice | null = null;
  private cmdChar: BluetoothRemoteGATTCharacteristic | null = null;
  private evtChar: BluetoothRemoteGATTCharacteristic | null = null;
  private handlers = new Set<(ev: DeviceEvent) => void>();
  private buffer = "";
  readonly transport: TransportInfo = { kind: "bluetooth", label: "Bluetooth" };

  get connected(): boolean {
    return this.device?.gatt?.connected ?? false;
  }

  static isSupported(): boolean {
    return typeof navigator !== "undefined" && "bluetooth" in navigator;
  }

  async connect(): Promise<void> {
    if (!BluetoothLink.isSupported()) {
      throw new Error("Web Bluetooth nie jest wspierany w tej przeglądarce.");
    }
    const device = await navigator.bluetooth.requestDevice({
      filters: [{ services: [DEVICE_BLE_SERVICE_UUID] }],
    });
    const server = await device.gatt!.connect();
    const service = await server.getPrimaryService(DEVICE_BLE_SERVICE_UUID);
    const cmdChar = await service.getCharacteristic(CMD_CHAR_UUID);
    const evtChar = await service.getCharacteristic(EVT_CHAR_UUID);

    await evtChar.startNotifications();
    evtChar.addEventListener("characteristicvaluechanged", this.onNotify);

    this.device = device;
    this.cmdChar = cmdChar;
    this.evtChar = evtChar;
  }

  async disconnect(): Promise<void> {
    try {
      this.evtChar?.removeEventListener("characteristicvaluechanged", this.onNotify);
      await this.evtChar?.stopNotifications();
      this.device?.gatt?.disconnect();
    } finally {
      this.device = null;
      this.cmdChar = null;
      this.evtChar = null;
      this.buffer = "";
    }
  }

  async send(cmd: DeviceCommand): Promise<void> {
    if (!this.cmdChar) throw new Error("Nie połączono z urządzeniem.");
    const bytes = encodeCommand(cmd);
    const buf = new ArrayBuffer(bytes.byteLength);
    new Uint8Array(buf).set(bytes);
    await this.cmdChar.writeValueWithoutResponse(buf);
  }

  onEvent(handler: (ev: DeviceEvent) => void): () => void {
    this.handlers.add(handler);
    return () => this.handlers.delete(handler);
  }

  private onNotify = (e: Event) => {
    const ch = e.target as BluetoothRemoteGATTCharacteristic;
    const value = ch.value;
    if (!value) return;
    const chunk = new TextDecoder().decode(value.buffer);
    this.buffer += chunk;
    let nl: number;
    while ((nl = this.buffer.indexOf("\n")) >= 0) {
      const line = this.buffer.slice(0, nl);
      this.buffer = this.buffer.slice(nl + 1);
      const ev = parseEvent(line);
      if (ev) for (const h of this.handlers) h(ev);
    }
  };
}
