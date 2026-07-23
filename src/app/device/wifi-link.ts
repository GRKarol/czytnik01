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
import { pinToDeviceNetwork, unpinFromDeviceNetwork } from "./network-pin";

export interface WifiLinkOptions {
  /** Bazowy URL urządzenia. Domyślnie `http://192.168.4.1`. */
  baseUrl?: string;
}

// Keep-alive: sprawdzamy /api/hello co tyle ms. Zgodne z regułą
// niezawodności z docs/flower-companion-api.md ("keep-alive co 8s,
// wykrycie rozłączenia w <16s").
const KEEP_ALIVE_INTERVAL_MS = 8000;
const KEEP_ALIVE_TIMEOUT_MS = 3000;
// Backoff prób auto-reconnect po zerwaniu połączenia. Po wyczerpaniu
// appka zostaje w stanie rozłączonym — użytkownik może spróbować ręcznie.
const RECONNECT_DELAYS_MS = [1000, 2000, 4000, 8000, 8000];

export class WifiLink implements DeviceLink {
  private socket: WebSocket | null = null;
  private handlers = new Set<(ev: DeviceEvent) => void>();
  private statusHandlers = new Set<(connected: boolean) => void>();
  private isConnected = false;
  private keepAliveTimer: ReturnType<typeof setInterval> | null = null;
  private reconnecting = false;
  private manuallyDisconnected = false;
  readonly transport: TransportInfo = { kind: "wifi", label: "WiFi" };

  constructor(private opts: WifiLinkOptions = {}) {}

  private get base(): string {
    return this.opts.baseUrl ?? DEVICE_AP_BASE_URL;
  }

  get connected(): boolean {
    return this.isConnected;
  }

  async connect(): Promise<void> {
    this.manuallyDisconnected = false;
    await this.attemptConnect();
  }

  private async attemptConnect(): Promise<void> {
    // 1. Sprawdź czy urządzenie odpowiada na /hello.
    const hello = await fetch(`${this.base}/api/hello`, {
      method: "GET",
      signal: AbortSignal.timeout(KEEP_ALIVE_TIMEOUT_MS),
    }).catch((err) => {
      console.error("[wifi-link] /api/hello fetch failed:", err);
      return null;
    });
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

    // WebSocket na eventy jest bonusem, nie warunkiem połączenia — obecny
    // firmware (CompanionSyncManager.cpp) w ogóle nie implementuje
    // /api/events. Kierunek telefon→czytnik (zmiana ustawień) i tak działa
    // zwykłym HTTP PATCH, więc brak WS nie może blokować całego connect().
    // Zweryfikowane na fizycznym urządzeniu (patrz docs/roadmap.md, Faza 6).
    try {
      await this.openSocket();
    } catch (err) {
      console.warn(
        "[wifi-link] WebSocket eventów niedostępny — appka działa bez zdarzeń na żywo z czytnika:",
        err,
      );
      this.socket = null;
    }

    this.isConnected = true;
    this.startKeepAlive();
    void pinToDeviceNetwork();
  }

  private async openSocket(): Promise<void> {
    const wsUrl = this.base.replace(/^http/, "ws") + "/api/events";
    const socket = new WebSocket(wsUrl);
    this.socket = socket;
    await new Promise<void>((resolve, reject) => {
      const onOpen = () => {
        socket.removeEventListener("error", onErr);
        resolve();
      };
      const onErr = () => {
        socket.removeEventListener("open", onOpen);
        reject(new Error("Nie udało się otworzyć kanału eventów (WebSocket)."));
      };
      socket.addEventListener("open", onOpen, { once: true });
      socket.addEventListener("error", onErr, { once: true });
    });

    socket.addEventListener("message", (e) => {
      const ev = parseEvent(typeof e.data === "string" ? e.data : "");
      if (ev) for (const h of this.handlers) h(ev);
    });
    socket.addEventListener("close", () => {
      // Ignoruj zamknięcie starego socketu, jeśli w międzyczasie powstał nowy
      // (np. po udanym reconnect) — to nie jest realny drop.
      if (this.socket !== socket) return;
      this.handleDrop();
    });
  }

  private startKeepAlive(): void {
    this.stopKeepAlive();
    this.keepAliveTimer = setInterval(() => {
      void this.pingOnce();
    }, KEEP_ALIVE_INTERVAL_MS);
  }

  private stopKeepAlive(): void {
    if (this.keepAliveTimer !== null) {
      clearInterval(this.keepAliveTimer);
      this.keepAliveTimer = null;
    }
  }

  private async pingOnce(): Promise<void> {
    try {
      const res = await fetch(`${this.base}/api/hello`, {
        signal: AbortSignal.timeout(KEEP_ALIVE_TIMEOUT_MS),
      });
      if (!res.ok) throw new Error("hello nie ok");
    } catch {
      this.handleDrop();
    }
  }

  /** Połączenie padło samo (WebSocket się zamknął albo keep-alive nie dostał odpowiedzi). */
  private handleDrop(): void {
    if (!this.isConnected || this.manuallyDisconnected) return;
    this.isConnected = false;
    this.stopKeepAlive();
    this.socket = null;
    void unpinFromDeviceNetwork();
    for (const h of this.statusHandlers) h(false);
    void this.scheduleReconnect();
  }

  private async scheduleReconnect(): Promise<void> {
    if (this.reconnecting || this.manuallyDisconnected) return;
    this.reconnecting = true;
    try {
      for (const delay of RECONNECT_DELAYS_MS) {
        if (this.manuallyDisconnected) return;
        await new Promise((r) => setTimeout(r, delay));
        if (this.manuallyDisconnected) return;
        try {
          await this.attemptConnect();
          for (const h of this.statusHandlers) h(true);
          return;
        } catch {
          // spróbuj ponownie po kolejnym opóźnieniu
        }
      }
      // Wyczerpano próby — zostajemy rozłączeni, użytkownik może spróbować ręcznie.
    } finally {
      this.reconnecting = false;
    }
  }

  async disconnect(): Promise<void> {
    this.manuallyDisconnected = true;
    this.stopKeepAlive();
    this.socket?.close();
    this.socket = null;
    this.isConnected = false;
    void unpinFromDeviceNetwork();
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

  onStatusChange(handler: (connected: boolean) => void): () => void {
    this.statusHandlers.add(handler);
    return () => this.statusHandlers.delete(handler);
  }

  static isSupported(): boolean {
    return typeof fetch === "function" && typeof WebSocket === "function";
  }
}
