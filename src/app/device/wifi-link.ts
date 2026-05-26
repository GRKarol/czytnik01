/**
 * WiFi transport — telefon łączy się do AP urządzenia (np. "Flower-AB12CD")
 * a app gada z `http://192.168.4.1` po HTTP (komendy) + WebSocket (eventy).
 *
 * Działa na iOS i Androidzie (bo to zwykłe fetch + WebSocket), nie wymaga
 * Web Serial ani Web Bluetooth. Wymaga jednak, żeby klient ręcznie
 * przełączył sieć WiFi telefonu — instrukcję pokazujemy w wizardzie.
 */

import { DEVICE_AP_BASE_URL } from "../../shared/config";
import type { DeviceCommand, DeviceEvent } from "../../shared/device-protocol";
import { parseEvent } from "../../shared/device-protocol";
import type { DeviceLink, TransportInfo } from "./device-link";

export interface WifiLinkOptions {
  /** Bazowy URL urządzenia. Domyślnie `http://192.168.4.1`. */
  baseUrl?: string;
}

export class WifiLink implements DeviceLink {
  private socket: WebSocket | null = null;
  private handlers = new Set<(ev: DeviceEvent) => void>();
  private isConnected = false;
  readonly transport: TransportInfo = { kind: "wifi", label: "WiFi" };

  constructor(private opts: WifiLinkOptions = {}) {}

  private get base(): string {
    return this.opts.baseUrl ?? DEVICE_AP_BASE_URL;
  }

  get connected(): boolean {
    return this.isConnected;
  }

  async connect(): Promise<void> {
    // 1. Sprawdź czy urządzenie odpowiada na /hello.
    const hello = await fetch(`${this.base}/api/hello`, { method: "GET" }).catch(() => null);
    if (!hello || !hello.ok) {
      // Najczęstsza przyczyna gdy aplikacja jest hostowana na HTTPS
      // (grkarol.github.io): mixed content block — przeglądarka odmawia
      // wykonania HTTP requestu z HTTPS strony. CORS tu nie pomoże, bo
      // request nigdy nie opuszcza klienta. Workaround: otworzyć
      // http://192.168.4.1/ bezpośrednio w przeglądarce telefonu i użyć
      // zakładki "Update" w Companion UI.
      const isHttps = typeof location !== "undefined" && location.protocol === "https:";
      throw new Error(
        isHttps
          ? `Przeglądarka zablokowała połączenie HTTPS → HTTP. Otwórz w telefonie ${this.base}/ bezpośrednio (Chrome/Safari), tam wgrywaj firmware i książki.`
          : `Nie udało się złapać urządzenia pod ${this.base}. Czy telefon jest podłączony do sieci urządzenia (Flower-…)?`,
      );
    }

    // 2. Otwórz WebSocket na eventy.
    const wsUrl = this.base.replace(/^http/, "ws") + "/api/events";
    this.socket = new WebSocket(wsUrl);
    await new Promise<void>((resolve, reject) => {
      const s = this.socket!;
      const onOpen = () => {
        s.removeEventListener("error", onErr);
        resolve();
      };
      const onErr = () => {
        s.removeEventListener("open", onOpen);
        reject(new Error("Nie udało się otworzyć kanału eventów (WebSocket)."));
      };
      s.addEventListener("open", onOpen, { once: true });
      s.addEventListener("error", onErr, { once: true });
    });

    this.socket.addEventListener("message", (e) => {
      const ev = parseEvent(typeof e.data === "string" ? e.data : "");
      if (ev) for (const h of this.handlers) h(ev);
    });
    this.socket.addEventListener("close", () => {
      this.isConnected = false;
    });

    this.isConnected = true;
  }

  async disconnect(): Promise<void> {
    this.socket?.close();
    this.socket = null;
    this.isConnected = false;
  }

  async send(cmd: DeviceCommand): Promise<void> {
    const res = await fetch(`${this.base}/api/cmd`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify(cmd),
    });
    if (!res.ok) {
      throw new Error(`Urządzenie odrzuciło komendę (${res.status}).`);
    }
  }

  onEvent(handler: (ev: DeviceEvent) => void): () => void {
    this.handlers.add(handler);
    return () => this.handlers.delete(handler);
  }

  static isSupported(): boolean {
    return typeof fetch === "function" && typeof WebSocket === "function";
  }
}
