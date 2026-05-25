/**
 * Wyższego rzędu API urządzenia: biblioteka, ustawienia, plugins, dev mode.
 *
 * Pod spodem (faza 3) opakuje to WifiLink / BluetoothLink. Na razie
 * — żeby PWA się rozwijała równolegle z firmware — `DeviceApi.mock`
 * zwraca dane testowe i pamięta zmiany w `localStorage`, więc Karol
 * widzi pełen flow UI od razu.
 */

export type Theme = "light" | "dark" | "night";
export type Language = "pl" | "en" | "de" | "es" | "fr" | "it";
export type ReaderHand = "right" | "left";
export type ReaderMode = "rsvp" | "scroll";
export type PauseBehaviour = "tap" | "long-press" | "auto";

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
  showBatteryWhileReading: boolean;
  showChapterWhileReading: boolean;
  showPercentWhileReading: boolean;
  devMode: boolean;
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
  showBatteryWhileReading: true,
  showChapterWhileReading: true,
  showPercentWhileReading: true,
  devMode: false,
};

export interface DeviceApi {
  listBooks(): Promise<Book[]>;
  uploadBook(file: Blob, name: string): Promise<void>;
  deleteBook(name: string): Promise<void>;
  getSettings(): Promise<DeviceSettings>;
  putSettings(patch: Partial<DeviceSettings>): Promise<DeviceSettings>;
}

// ─── Mock implementation ────────────────────────────────────────────────────

const STORE_BOOKS = "flower.mock.books";
const STORE_SETTINGS = "flower.mock.settings";

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

const MOCK_BOOKS_SEED: Book[] = [
  {
    name: "books/sample-rozdzialy.rsvp",
    title: "Mały próbnik",
    author: "Anonim",
    bytes: 12480,
    progressPercent: 42,
    category: "book",
    addedAt: "2026-05-22T10:14:00Z",
  },
  {
    name: "books/krotki-tekst.rsvp",
    title: "Krótki tekst",
    author: "",
    bytes: 3120,
    progressPercent: 100,
    category: "book",
    addedAt: "2026-05-18T19:02:00Z",
  },
  {
    name: "articles/poniedzialkowy-newsletter.rsvp",
    title: "Poniedziałkowy newsletter",
    author: "Redakcja",
    bytes: 18420,
    progressPercent: 0,
    category: "article",
    addedAt: "2026-05-24T07:30:00Z",
  },
];

export class MockDeviceApi implements DeviceApi {
  private async delay<T>(value: T, ms = 200): Promise<T> {
    await new Promise((r) => setTimeout(r, ms));
    return value;
  }

  async listBooks(): Promise<Book[]> {
    return this.delay(read<Book[]>(STORE_BOOKS, MOCK_BOOKS_SEED));
  }

  async uploadBook(file: Blob, name: string): Promise<void> {
    const list = read<Book[]>(STORE_BOOKS, MOCK_BOOKS_SEED);
    list.unshift({
      name: name.startsWith("books/") ? name : `books/${name}`,
      title: stripExt(name.replace(/^books\//, "")),
      author: "",
      bytes: file.size,
      progressPercent: 0,
      category: "book",
      addedAt: new Date().toISOString(),
    });
    write(STORE_BOOKS, list);
    await this.delay(undefined, 400);
  }

  async deleteBook(name: string): Promise<void> {
    const list = read<Book[]>(STORE_BOOKS, MOCK_BOOKS_SEED);
    write(STORE_BOOKS, list.filter((b) => b.name !== name));
    await this.delay(undefined, 200);
  }

  async getSettings(): Promise<DeviceSettings> {
    return this.delay(read<DeviceSettings>(STORE_SETTINGS, DEFAULT_SETTINGS));
  }

  async putSettings(patch: Partial<DeviceSettings>): Promise<DeviceSettings> {
    const current = read<DeviceSettings>(STORE_SETTINGS, DEFAULT_SETTINGS);
    const next = { ...current, ...patch };
    write(STORE_SETTINGS, next);
    return this.delay(next, 150);
  }
}

function stripExt(name: string): string {
  return name.replace(/\.[^.]+$/, "");
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
};

export function setDeviceApi(api: DeviceApi): void {
  _api = api;
  for (const l of _apiListeners) l(api);
}

export function onDeviceApiChange(handler: (api: DeviceApi) => void): () => void {
  _apiListeners.add(handler);
  return () => _apiListeners.delete(handler);
}
