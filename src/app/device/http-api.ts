/**
 * Real implementation of `DeviceApi` over HTTP — used kiedy PWA jest
 * podłączona do urządzenia przez WiFi (telefon w sieci `Flower-XXXX`,
 * urządzenie pod `http://192.168.4.1`). Stosujemy ten sam interface
 * co `MockDeviceApi`, więc komponenty UI nie wiedzą o różnicy.
 *
 * Firmware używa zagnieżdżonego JSON-a (sekcje `reading`, `display`,
 * `typography`, `developer`) — tu mamy adapter, który mapuje to na
 * płaski `DeviceSettings`. Dzięki temu komponenty mają czysty model
 * bez wiedzy o wewnętrznym schemacie urządzenia.
 */

import { DEVICE_AP_BASE_URL } from "../../shared/config";
import {
  DEFAULT_SETTINGS,
  type Book,
  type DeviceApi,
  type DeviceSettings,
  type Language,
  type PauseBehaviour,
  type ReaderHand,
  type ReaderMode,
  type Theme,
} from "./api";

const LANG_INDEX: Language[] = ["pl", "en", "de", "es", "fr", "it"];

interface FirmwareSettings {
  reading?: {
    wpm?: number;
    readerMode?: ReaderMode;
    pauseMode?: "sentence_end" | "instant";
    pacing?: { longWordMs?: number; complexWordMs?: number; punctuationMs?: number };
  };
  display?: {
    brightnessIndex?: number;
    darkMode?: boolean;
    nightMode?: boolean;
    handedness?: ReaderHand;
    readingBattery?: boolean;
    readingChapter?: boolean;
    readingProgress?: boolean;
    language?: number;
  };
  developer?: { devMode?: boolean };
}

function fromFirmware(fw: FirmwareSettings): DeviceSettings {
  const d = fw.display ?? {};
  const r = fw.reading ?? {};
  const p = r.pacing ?? {};
  const theme: Theme = d.nightMode ? "night" : d.darkMode ? "dark" : "light";
  const pause: PauseBehaviour = r.pauseMode === "instant" ? "auto" : "tap";
  const lang: Language = LANG_INDEX[d.language ?? 0] ?? "pl";
  // brightnessIndex w firmware to 0..N gdzie N to kMaxBrightness — skalujemy
  // przybliżenie do 0..100 dla UI.
  const brightness =
    typeof d.brightnessIndex === "number"
      ? Math.min(100, Math.round((d.brightnessIndex / 4) * 100))
      : DEFAULT_SETTINGS.brightness;
  return {
    ...DEFAULT_SETTINGS,
    theme,
    brightness,
    language: lang,
    readerHand: d.handedness ?? DEFAULT_SETTINGS.readerHand,
    readerMode: r.readerMode ?? DEFAULT_SETTINGS.readerMode,
    pauseBehaviour: pause,
    baseWpm: r.wpm ?? DEFAULT_SETTINGS.baseWpm,
    longWordDelayMs: p.longWordMs ?? DEFAULT_SETTINGS.longWordDelayMs,
    complexWordDelayMs: p.complexWordMs ?? DEFAULT_SETTINGS.complexWordDelayMs,
    punctuationDelayMs: p.punctuationMs ?? DEFAULT_SETTINGS.punctuationDelayMs,
    showBatteryWhileReading: d.readingBattery ?? DEFAULT_SETTINGS.showBatteryWhileReading,
    showChapterWhileReading: d.readingChapter ?? DEFAULT_SETTINGS.showChapterWhileReading,
    showPercentWhileReading: d.readingProgress ?? DEFAULT_SETTINGS.showPercentWhileReading,
    devMode: fw.developer?.devMode ?? false,
  };
}

function toFirmware(p: Partial<DeviceSettings>): Record<string, unknown> {
  // applySettingsJson w firmware czyta po nazwie klucza (nie po sekcji),
  // więc możemy spłaszczyć payload.
  const out: Record<string, unknown> = {};
  if (p.theme != null) {
    out.darkMode = p.theme === "dark" || p.theme === "night";
    out.nightMode = p.theme === "night";
  }
  if (p.brightness != null) {
    // PWA daje 0..100, firmware oczekuje brightnessIndex 0..4 (kMaxBrightness).
    out.brightnessIndex = Math.max(0, Math.min(4, Math.round((p.brightness / 100) * 4)));
  }
  if (p.language != null) {
    const idx = LANG_INDEX.indexOf(p.language);
    if (idx >= 0) out.language = idx;
  }
  if (p.readerHand != null) out.handedness = p.readerHand;
  if (p.readerMode != null) out.readerMode = p.readerMode;
  if (p.pauseBehaviour != null) {
    out.pauseMode = p.pauseBehaviour === "auto" ? "instant" : "sentence_end";
  }
  if (p.baseWpm != null) out.wpm = p.baseWpm;
  if (p.longWordDelayMs != null) out.longWordMs = p.longWordDelayMs;
  if (p.complexWordDelayMs != null) out.complexWordMs = p.complexWordDelayMs;
  if (p.punctuationDelayMs != null) out.punctuationMs = p.punctuationDelayMs;
  if (p.showBatteryWhileReading != null) out.readingBattery = p.showBatteryWhileReading;
  if (p.showChapterWhileReading != null) out.readingChapter = p.showChapterWhileReading;
  if (p.showPercentWhileReading != null) out.readingProgress = p.showPercentWhileReading;
  if (p.devMode != null) out.devMode = p.devMode;
  return out;
}

export class HttpDeviceApi implements DeviceApi {
  constructor(private baseUrl: string = DEVICE_AP_BASE_URL) {}

  private url(path: string): string {
    return this.baseUrl.replace(/\/+$/, "") + path;
  }

  private async json<T>(res: Response): Promise<T> {
    if (!res.ok) {
      const text = await res.text().catch(() => "");
      throw new Error(`Urządzenie odrzuciło żądanie (HTTP ${res.status}). ${text}`);
    }
    return (await res.json()) as T;
  }

  async listBooks(): Promise<Book[]> {
    const data = await this.json<{ books: Book[] }>(await fetch(this.url("/api/books")));
    return data.books;
  }

  async uploadBook(file: Blob, name: string): Promise<void> {
    const fd = new FormData();
    fd.append("file", file, name);
    const res = await fetch(this.url("/api/books"), { method: "POST", body: fd });
    if (!res.ok) {
      const text = await res.text().catch(() => "");
      throw new Error(`Upload nie powiódł się (${res.status}). ${text}`);
    }
  }

  async deleteBook(name: string): Promise<void> {
    const res = await fetch(this.url(`/api/books/${encodeURIComponent(name)}`), {
      method: "DELETE",
    });
    if (!res.ok && res.status !== 404) {
      throw new Error(`Nie udało się usunąć (${res.status}).`);
    }
  }

  async getSettings(): Promise<DeviceSettings> {
    const fw = await this.json<FirmwareSettings>(await fetch(this.url("/api/settings")));
    return fromFirmware(fw);
  }

  async putSettings(patch: Partial<DeviceSettings>): Promise<DeviceSettings> {
    const res = await fetch(this.url("/api/settings"), {
      method: "PUT",
      headers: { "content-type": "application/json" },
      body: JSON.stringify(toFirmware(patch)),
    });
    const fw = await this.json<FirmwareSettings>(res);
    return fromFirmware(fw);
  }

  /**
   * Multipart upload do `/api/ota`. Używamy XMLHttpRequest zamiast fetch,
   * bo fetch nie wystawia natywnego progress callbacka dla uploadu.
   * Urządzenie po sukcesie odpowiada `{"ok":true,"reboot":true}` i robi
   * `ESP.restart()` — następne komendy do `192.168.4.1` będą padać aż
   * wstanie z powrotem (~5–10 s) i klient ponownie podłączy się do AP.
   */
  async installOta(blob: Blob, onProgress?: (loaded: number, total: number) => void): Promise<void> {
    const fd = new FormData();
    fd.append("firmware", blob, "flower-firmware.bin");
    await new Promise<void>((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.open("POST", this.url("/api/ota"));
      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable && onProgress) onProgress(e.loaded, e.total);
      };
      xhr.onload = () => {
        if (xhr.status >= 200 && xhr.status < 300) {
          resolve();
        } else {
          reject(new Error(`Urządzenie odrzuciło OTA (${xhr.status}): ${xhr.responseText}`));
        }
      };
      xhr.onerror = () => reject(new Error("Połączenie z urządzeniem zerwane przed końcem OTA."));
      xhr.send(fd);
    });
  }
}

/** Lekki "ping" — sprawdza czy urządzenie jest pod baseUrl. */
export async function pingDevice(baseUrl: string = DEVICE_AP_BASE_URL): Promise<boolean> {
  try {
    const res = await fetch(`${baseUrl.replace(/\/+$/, "")}/api/hello`, {
      signal: AbortSignal.timeout(3000),
    });
    return res.ok;
  } catch {
    return false;
  }
}
