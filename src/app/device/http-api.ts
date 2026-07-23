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
  type Typeface,
  type FooterMetric,
  type BatteryLabel,
  type NavMode,
  type ScreensaverMode,
  cacheBooks,
  cacheSettings,
} from "./api";
import { UI_LANG_INDEX_MAP } from "../i18n/lang-map";

// Jedno źródło prawdy dla mapowania indeksu języka firmware (0-5) —
// UI_LANG_INDEX_MAP w src/app/i18n/lang-map.ts, zgodne z
// firmware/src/app/Localization.h (enum UiLanguage). Wcześniej był tu
// osobny, BŁĘDNY hardkodowany index (["pl","en","de","es","fr","it"]),
// który nie zgadzał się z prawdziwą kolejnością firmware — stąd appka
// wysyłała/odczytywała zły język (zgłoszone jako "języki są źle zrobione").
function langIdToCode(id: number | undefined): Language {
  return UI_LANG_INDEX_MAP[id ?? 0] ?? "pl";
}

function langCodeToId(code: Language): number | undefined {
  const entry = Object.entries(UI_LANG_INDEX_MAP).find(([, v]) => v === code);
  return entry ? Number(entry[0]) : undefined;
}

interface FirmwareSettings {
  reading?: {
    wpm?: number;
    readerMode?: ReaderMode;
    pauseMode?: "sentence_end" | "instant";
    accurateTimeEstimate?: boolean;
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
    savePointButton?: boolean;
    showHelpHints?: boolean;
    language?: number;
    phantomWords?: boolean;
    fontSizeIndex?: number;
    footerMetric?: FooterMetric;
    batteryLabel?: BatteryLabel;
  };
  typography?: {
    typeface?: Typeface;
    focusHighlight?: boolean;
    tracking?: number;
    anchorPercent?: number;
    guideWidth?: number;
    guideGap?: number;
    focusColorIndex?: number;
  };
  scroll?: {
    scrollFontSize?: number;
    scrollLineSpacing?: number;
    scrollMargin?: number;
  };
  input?: {
    navMode?: NavMode;
  };
  screensaver?: {
    mode?: ScreensaverMode;
    timeoutIndex?: number;
    autoOffIndex?: number;
    sleepGuardIndex?: number;
  };
  connectivity?: {
    bleEnabled?: boolean;
    otaAutoCheck?: boolean;
  };
  developer?: { devMode?: boolean };
}

function fromFirmware(fw: FirmwareSettings): DeviceSettings {
  const d = fw.display ?? {};
  const r = fw.reading ?? {};
  const p = r.pacing ?? {};
  const t = fw.typography ?? {};
  const sc = fw.scroll ?? {};
  const theme: Theme = d.nightMode ? "night" : d.darkMode ? "dark" : "light";
  const pause: PauseBehaviour = r.pauseMode === "instant" ? "auto" : "tap";
  const lang: Language = langIdToCode(d.language);
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
    // Scroll settings
    scrollFontSize: sc.scrollFontSize ?? DEFAULT_SETTINGS.scrollFontSize,
    scrollLineSpacing: sc.scrollLineSpacing ?? DEFAULT_SETTINGS.scrollLineSpacing,
    scrollMargin: sc.scrollMargin ?? DEFAULT_SETTINGS.scrollMargin,
    // Typography (RSVP)
    fontSizeIndex: d.fontSizeIndex ?? DEFAULT_SETTINGS.fontSizeIndex,
    typeface: t.typeface ?? DEFAULT_SETTINGS.typeface,
    phantomWords: d.phantomWords ?? DEFAULT_SETTINGS.phantomWords,
    focusHighlight: t.focusHighlight ?? DEFAULT_SETTINGS.focusHighlight,
    tracking: t.tracking ?? DEFAULT_SETTINGS.tracking,
    anchorPercent: t.anchorPercent ?? DEFAULT_SETTINGS.anchorPercent,
    guideWidth: t.guideWidth ?? DEFAULT_SETTINGS.guideWidth,
    guideGap: t.guideGap ?? DEFAULT_SETTINGS.guideGap,
    focusColorIndex: t.focusColorIndex ?? DEFAULT_SETTINGS.focusColorIndex,
    // HUD metrics
    footerMetric: d.footerMetric ?? DEFAULT_SETTINGS.footerMetric,
    batteryLabel: d.batteryLabel ?? DEFAULT_SETTINGS.batteryLabel,
    // Dopisane po audycie parytetu firmware (2026-07-21)
    accurateTimeEstimate: r.accurateTimeEstimate ?? DEFAULT_SETTINGS.accurateTimeEstimate,
    savePointButtonVisible: d.savePointButton ?? DEFAULT_SETTINGS.savePointButtonVisible,
    showHelpHints: d.showHelpHints ?? DEFAULT_SETTINGS.showHelpHints,
    navMode: fw.input?.navMode ?? DEFAULT_SETTINGS.navMode,
    screensaverMode: fw.screensaver?.mode ?? DEFAULT_SETTINGS.screensaverMode,
    screensaverTimeoutIndex:
      fw.screensaver?.timeoutIndex ?? DEFAULT_SETTINGS.screensaverTimeoutIndex,
    screensaverAutoOffIndex:
      fw.screensaver?.autoOffIndex ?? DEFAULT_SETTINGS.screensaverAutoOffIndex,
    screensaverSleepGuardIndex:
      fw.screensaver?.sleepGuardIndex ?? DEFAULT_SETTINGS.screensaverSleepGuardIndex,
    bleEnabled: fw.connectivity?.bleEnabled ?? DEFAULT_SETTINGS.bleEnabled,
    otaAutoCheck: fw.connectivity?.otaAutoCheck ?? DEFAULT_SETTINGS.otaAutoCheck,
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
    const idx = langCodeToId(p.language);
    if (idx != null) out.language = idx;
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
  // Scroll settings
  if (p.scrollFontSize != null) out.scrollFontSize = p.scrollFontSize;
  if (p.scrollLineSpacing != null) out.scrollLineSpacing = p.scrollLineSpacing;
  if (p.scrollMargin != null) out.scrollMargin = p.scrollMargin;
  // Typography (RSVP)
  if (p.fontSizeIndex != null) out.fontSizeIndex = p.fontSizeIndex;
  if (p.typeface != null) out.typeface = p.typeface;
  if (p.phantomWords != null) out.phantomWords = p.phantomWords;
  if (p.focusHighlight != null) out.focusHighlight = p.focusHighlight;
  if (p.tracking != null) out.tracking = p.tracking;
  if (p.anchorPercent != null) out.anchorPercent = p.anchorPercent;
  if (p.guideWidth != null) out.guideWidth = p.guideWidth;
  if (p.guideGap != null) out.guideGap = p.guideGap;
  // HUD metrics
  if (p.footerMetric != null) out.footerMetric = p.footerMetric;
  if (p.batteryLabel != null) out.batteryLabel = p.batteryLabel;
  // Dopisane po audycie parytetu firmware (2026-07-21)
  if (p.accurateTimeEstimate != null) out.accurateTimeEstimate = p.accurateTimeEstimate;
  if (p.savePointButtonVisible != null) out.savePointButton = p.savePointButtonVisible;
  if (p.showHelpHints != null) out.showHelpHints = p.showHelpHints;
  if (p.focusColorIndex != null) out.focusColorIndex = p.focusColorIndex;
  if (p.navMode != null) out.navMode = p.navMode;
  if (p.screensaverMode != null) out.screensaverMode = p.screensaverMode;
  if (p.screensaverTimeoutIndex != null) out.screensaverTimeoutIndex = p.screensaverTimeoutIndex;
  if (p.screensaverAutoOffIndex != null) out.screensaverAutoOffIndex = p.screensaverAutoOffIndex;
  if (p.screensaverSleepGuardIndex != null)
    out.screensaverSleepGuardIndex = p.screensaverSleepGuardIndex;
  if (p.bleEnabled != null) out.bleEnabled = p.bleEnabled;
  if (p.otaAutoCheck != null) out.otaAutoCheck = p.otaAutoCheck;
  return out;
}

export class HttpDeviceApi implements DeviceApi {
  // ESP32 jest single-threaded — kolejkujemy PUT/PATCH ustawień, żeby nie
  // wysłać dwóch naraz (zasada niezawodności z docs/flower-companion-api.md).
  private settingsQueue: Promise<void> = Promise.resolve();

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
    cacheBooks(data.books);
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
    // Firmware rejestruje tylko dokładną ścieżkę "/api/books" (bez segmentu
    // ścieżki) i czyta nazwę z parametru zapytania — zob.
    // CompanionSyncManager::handleBookDelete() (server_.arg("name")).
    // Wcześniej ta funkcja wysyłała nazwę jako segment ścieżki
    // (/api/books/<nazwa>), co nigdy nie trafiało w zarejestrowaną trasę —
    // każde usunięcie książki z poziomu appki kończyło się 404.
    const res = await fetch(this.url(`/api/books?name=${encodeURIComponent(name)}`), {
      method: "DELETE",
    });
    if (!res.ok && res.status !== 404) {
      throw new Error(`Nie udało się usunąć (${res.status}).`);
    }
  }

  async getSettings(): Promise<DeviceSettings> {
    const fw = await this.json<FirmwareSettings>(await fetch(this.url("/api/settings")));
    const settings = fromFirmware(fw);
    cacheSettings(settings);
    return settings;
  }

  async putSettings(patch: Partial<DeviceSettings>): Promise<DeviceSettings> {
    // Czeka aż poprzednie zapytanie (sukces albo błąd) się skończy, zanim
    // wyśle kolejne — bez tego dwa szybkie po sobie PATCH-e (np. z suwaka)
    // mogłyby dojść do ESP32 w złej kolejności albo się pogubić.
    const task = this.settingsQueue.then(() => this.putSettingsNow(patch));
    this.settingsQueue = task.then(
      () => undefined,
      () => undefined,
    );
    return task;
  }

  private async putSettingsNow(patch: Partial<DeviceSettings>): Promise<DeviceSettings> {
    const res = await fetch(this.url("/api/settings"), {
      method: "PUT",
      headers: { "content-type": "application/json" },
      body: JSON.stringify(toFirmware(patch)),
    });
    const fw = await this.json<FirmwareSettings>(res);
    const settings = fromFirmware(fw);
    cacheSettings(settings);
    return settings;
  }

  /**
   * Multipart upload do `/api/ota`. Używamy XMLHttpRequest zamiast fetch,
   * bo fetch nie wystawia natywnego progress callbacka dla uploadu.
   * Urządzenie po sukcesie odpowiada `{"ok":true,"reboot":true}` i robi
   * `ESP.restart()` — następne komendy do `192.168.4.1` będą padać aż
   * wstanie z powrotem (~5–10 s) i klient ponownie podłączy się do AP.
   */
  async installOta(
    blob: Blob,
    onProgress?: (loaded: number, total: number) => void,
  ): Promise<void> {
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

/**
 * To samo co `pingDevice`, ale próbuje kilka razy zanim odda `false`.
 *
 * Zaraz po dołączeniu telefonu do AP czytnika pojedynczy `/api/hello`
 * potrafi zawieść (urządzenie jeszcze się nie ustatkowało w sieci) — bez
 * retry appka po cichu zostawała na `MockDeviceApi` mimo że pokazywała
 * "połączono" (patrz docs/roadmap.md, Faza 6).
 */
export async function pingDeviceWithRetry(
  baseUrl: string = DEVICE_AP_BASE_URL,
  attempts = 3,
  delayMs = 800,
): Promise<boolean> {
  for (let i = 0; i < attempts; i++) {
    if (await pingDevice(baseUrl)) return true;
    if (i < attempts - 1) await new Promise((r) => setTimeout(r, delayMs));
  }
  return false;
}
