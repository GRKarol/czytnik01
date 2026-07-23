/**
 * Wyższego rzędu API urządzenia: biblioteka, ustawienia, plugins, dev mode.
 *
 * Pod spodem (faza 3) opakuje to WifiLink / BluetoothLink. Na razie
 * — żeby PWA się rozwijała równolegle z firmware — `DeviceApi.mock`
 * zwraca dane testowe i pamięta zmiany w `localStorage`, więc Karol
 * widzi pełen flow UI od razu.
 */

export type Theme = "light" | "dark" | "night";
export type Language = "pl" | "en" | "de" | "es" | "fr" | "ro";
export type ReaderHand = "right" | "left";
export type ReaderMode = "rsvp" | "scroll";
export type PauseBehaviour = "tap" | "long-press" | "auto";
export type Typeface = "standard" | "open_dyslexic" | "atkinson";
export type FooterMetric = "percentage" | "chapter_time" | "book_time";
export type BatteryLabel = "percent" | "time_remaining" | "voltage";
export type NavMode = "swipe" | "dpad";
export type ScreensaverMode = "life" | "maze" | "voronoi" | "stars" | "matrix" | "screen_off";

export interface DeviceSettings {
  theme: Theme;
  brightness: number; // 0-100
  language: Language;
  readerHand: ReaderHand;
  readerMode: ReaderMode;
  pauseBehaviour: PauseBehaviour;
  baseWpm: number; // 10-1000
  longWordDelayMs: number;
  complexWordDelayMs: number;
  punctuationDelayMs: number;
  accurateTimeEstimate: boolean;
  showBatteryWhileReading: boolean;
  showChapterWhileReading: boolean;
  showPercentWhileReading: boolean;
  savePointButtonVisible: boolean;
  showHelpHints: boolean;
  devMode: boolean;
  scrollFontSize: number; // 0–8 (numeric scale, 0=tiny, 8=maximum)
  scrollLineSpacing: number; // 0–2 (Compact → Relaxed)
  scrollMargin: number; // 0–2 (Narrow → Wide)
  // Typography (RSVP)
  fontSizeIndex: number; // 0–2 (small/medium/large)
  typeface: Typeface;
  phantomWords: boolean;
  focusHighlight: boolean;
  focusColorIndex: number; // 0-5 (red/blue/green/yellow/orange/purple)
  tracking: number; // -2 to +3
  anchorPercent: number; // 30–40
  guideWidth: number; // 12–30
  guideGap: number; // 2–8
  // HUD metrics
  footerMetric: FooterMetric;
  batteryLabel: BatteryLabel;
  // Sterowanie (dopisane po audycie parytetu firmware, patrz docs/roadmap.md)
  navMode: NavMode;
  // Wygaszacz ekranu
  screensaverMode: ScreensaverMode;
  screensaverTimeoutIndex: number; // 0-7, minuty: 1,2,3,5,10,15,20,30
  screensaverAutoOffIndex: number; // 0-7, minuty: 0(nigdy),5,10,15,20,30,45,60
  screensaverSleepGuardIndex: number; // 0-7, minuty: 0(wyłączone),5,10,15,20,30,45,60
  // Łączność
  bleEnabled: boolean;
  otaAutoCheck: boolean;
}

export interface Book {
  name: string;
  title?: string;
  author?: string;
  bytes: number;
  progressPercent?: number;
  category?: "book" | "article";
  addedAt?: string;
}

export const DEFAULT_SETTINGS: DeviceSettings = {
  theme: "dark",
  brightness: 70,
  language: "pl",
  readerHand: "right",
  readerMode: "rsvp",
  pauseBehaviour: "tap",
  baseWpm: 300,
  longWordDelayMs: 150,
  complexWordDelayMs: 100,
  punctuationDelayMs: 200,
  accurateTimeEstimate: true,
  showBatteryWhileReading: true,
  showChapterWhileReading: true,
  showPercentWhileReading: true,
  savePointButtonVisible: true,
  showHelpHints: true,
  devMode: false,
  scrollFontSize: 4, // Medium (level 4 of 0-8)
  scrollLineSpacing: 1, // Normal
  scrollMargin: 1, // Normal
  // Typography (RSVP)
  fontSizeIndex: 0,
  typeface: "standard",
  phantomWords: true,
  focusHighlight: true,
  focusColorIndex: 0, // red
  tracking: 0,
  anchorPercent: 30,
  guideWidth: 30,
  guideGap: 5,
  // HUD metrics
  footerMetric: "percentage",
  batteryLabel: "percent",
  // Sterowanie
  navMode: "swipe",
  // Wygaszacz ekranu
  screensaverMode: "life",
  screensaverTimeoutIndex: 2, // 3 min
  screensaverAutoOffIndex: 0, // nigdy
  screensaverSleepGuardIndex: 0, // wyłączone
  // Łączność
  bleEnabled: false,
  otaAutoCheck: true,
};

export interface DeviceApi {
  listBooks(): Promise<Book[]>;
  uploadBook(file: Blob, name: string): Promise<void>;
  deleteBook(name: string): Promise<void>;
  getSettings(): Promise<DeviceSettings>;
  putSettings(patch: Partial<DeviceSettings>): Promise<DeviceSettings>;
  /**
   * Wysyła firmware (.bin) na urządzenie i instaluje przez OTA. Po sukcesie
   * urządzenie się restartuje, więc Promise resolve'uje TUŻ przed restartem
   * — connection zaraz potem padnie. Mock no-op (rzuca błąd).
   */
  installOta(blob: Blob, onProgress?: (loaded: number, total: number) => void): Promise<void>;
}

// ─── Offline/cache implementation ───────────────────────────────────────────
//
// To NIE jest generator danych demo — to jest "ostatni znany, prawdziwy stan"
// czytnika, zapamiętany z poprzedniej udanej synchronizacji (HttpDeviceApi
// dopisuje tu przy każdym udanym fetchu, patrz http-api.ts). Zanim appka
// choć raz się zsynchronizuje, biblioteka jest pusta — żadnych fałszywych
// "przykładowych książek". Po synchronizacji, przy rozłączeniu, appka nadal
// pokazuje tę samą listę (na szaro, w UI), ale każda próba zmiany czegokolwiek
// (upload, usunięcie, zmiana ustawienia, OTA) rzuca błąd — to wymaga realnego
// połączenia z czytnikiem.

const STORE_BOOKS = "flower.cache.books";
const STORE_SETTINGS = "flower.cache.settings";

function read<T>(key: string, fallback: T): T {
  try {
    const raw = localStorage.getItem(key);
    return raw ? (JSON.parse(raw) as T) : fallback;
  } catch {
    return fallback;
  }
}

function write<T>(key: string, value: T): void {
  try {
    localStorage.setItem(key, JSON.stringify(value));
  } catch {
    /* ignored */
  }
}

/** Wywoływane przez HttpDeviceApi po każdym udanym fetchu — zapamiętuje "ostatni znany" stan. */
export function cacheBooks(books: Book[]): void {
  write(STORE_BOOKS, books);
}

export function cacheSettings(settings: DeviceSettings): void {
  write(STORE_SETTINGS, settings);
}

const NEEDS_CONNECTION_MESSAGE =
  "Wymaga połączenia z czytnikiem. Najpierw połącz się przez WiFi.";

export class MockDeviceApi implements DeviceApi {
  private async delay<T>(value: T, ms = 150): Promise<T> {
    await new Promise((r) => setTimeout(r, ms));
    return value;
  }

  async listBooks(): Promise<Book[]> {
    return this.delay(read<Book[]>(STORE_BOOKS, []));
  }

  async uploadBook(): Promise<void> {
    throw new Error(NEEDS_CONNECTION_MESSAGE);
  }

  async deleteBook(): Promise<void> {
    throw new Error(NEEDS_CONNECTION_MESSAGE);
  }

  async getSettings(): Promise<DeviceSettings> {
    return this.delay(read<DeviceSettings>(STORE_SETTINGS, DEFAULT_SETTINGS));
  }

  async putSettings(): Promise<DeviceSettings> {
    throw new Error(NEEDS_CONNECTION_MESSAGE);
  }

  async installOta(): Promise<void> {
    throw new Error(NEEDS_CONNECTION_MESSAGE);
  }
}

// ─── Reactive API selection ─────────────────────────────────────────────────
//
// `deviceApi` to ruchomy wskaźnik. Domyślnie wskazuje na mocka (PWA
// działa nawet bez urządzenia). Kiedy klient połączy się przez WiFi,
// app.element.ts robi `setDeviceApi(new HttpDeviceApi(...))`.
//
// Komponenty subskrybują zmianę przez `onDeviceApiChange()` — kiedy
// API się przełączy, są informowane żeby odświeżyć dane.

let _api: DeviceApi = new MockDeviceApi();
const _apiListeners = new Set<(api: DeviceApi) => void>();

export const deviceApi = {
  get current(): DeviceApi {
    return _api;
  },
  listBooks: () => _api.listBooks(),
  uploadBook: (f: Blob, n: string) => _api.uploadBook(f, n),
  deleteBook: (n: string) => _api.deleteBook(n),
  getSettings: () => _api.getSettings(),
  putSettings: (p: Partial<DeviceSettings>) => _api.putSettings(p),
  installOta: (b: Blob, onProgress?: (loaded: number, total: number) => void) =>
    _api.installOta(b, onProgress),
};

export function setDeviceApi(api: DeviceApi): void {
  _api = api;
  for (const l of _apiListeners) l(api);
}

/** Czy appka gada właśnie z prawdziwym czytnikiem (nie tylko z cache'em). */
export function isDeviceConnected(): boolean {
  return !(_api instanceof MockDeviceApi);
}

export function onDeviceApiChange(handler: (api: DeviceApi) => void): () => void {
  _apiListeners.add(handler);
  return () => _apiListeners.delete(handler);
}
