# Opis dwóch stron webowych projektu "Flower" (czytnik01)

## Kontekst projektu

Projekt dotyczy fizycznego urządzenia — czytnika e-booków o nazwie **Flower** (codename: `czytnik01`). Urządzenie oparte jest na ESP32-S3 (Waveshare). Cały frontend to dwie osobne strony webowe (SPA) zbudowane w **TypeScript + Lit (Web Components) + Vite**. Obie strony są hostowane statycznie (np. na Netlify/GitHub Pages).

---

## STRONA 1: Flasher (instalator firmware)

### Cel

Jednorazowa strona desktopowa, która pozwala użytkownikowi wgrać firmware na urządzenie przez kabel USB — bez instalowania żadnych narzędzi (VS Code, PlatformIO, terminal).

### Jak działa

- Używa biblioteki **ESP Web Tools** (`esp-web-install-button`) — gotowy Web Component, który obsługuje cały proces flashowania ESP32 przez **Web Serial API**.
- Strona wyświetla przycisk "Zainstaluj firmware". Po kliknięciu przeglądarka (Chrome/Edge) otwiera okno wyboru portu USB, a ESP Web Tools pobiera manifest firmware i wgrywa go na urządzenie.
- Manifest firmware (`/firmware/manifest.json`) wskazuje na pliki .bin do pobrania.

### Wymagania techniczne

- Działa TYLKO w Chrome/Edge na desktopie (Web Serial wymaga HTTPS lub localhost).
- Użytkownik musi podłączyć urządzenie kablem USB z transmisją danych.
- Jeśli nie łączy — przytrzymać BOOT, nacisnąć reset, spróbować ponownie.

### Struktura kodu

- `src/flasher/main.ts` — importuje ESP Web Tools i rejestruje element.
- `src/flasher/flasher.element.ts` — jeden Web Component `<czytnik-flasher>` z dwoma krokami:
  1. Krok 1: Zainstaluj firmware (przycisk ESP Web Tools).
  2. Krok 2: Link do aplikacji mobilnej (PWA).
- Sprawdza `navigator.serial` i `window.isSecureContext` — wyświetla ostrzeżenia jeśli brakuje wsparcia.

### Technologie

- Lit (Web Components)
- ESP Web Tools (esp-web-tools)
- Web Serial API
- Vite (bundler)

---

## STRONA 2: Aplikacja mobilna (PWA)

### Cel

Główna aplikacja użytkownika — PWA (Progressive Web App) instalowana na telefonie. Służy do:

- Łączenia się z urządzeniem bezprzewodowo
- Wysyłania książek na urządzenie
- Konwertowania plików (EPUB, PDF, TXT, MD, HTML) na format `.rsvp`
- Zarządzania biblioteką na urządzeniu
- Instalowania pluginów
- Aktualizowania firmware OTA (Over The Air)
- Konfigurowania ustawień czytnika

### Jak działa połączenie z urządzeniem

Aplikacja oferuje 3 sposoby połączenia:

1. **WiFi (główny, polecany)** — urządzenie tworzy własną sieć WiFi (Access Point) o nazwie `Flower-XXXX`. Użytkownik przełącza telefon na tę sieć, a aplikacja komunikuje się z urządzeniem pod `http://192.168.4.1` przez:
   - HTTP REST API (komendy, upload plików, ustawienia)
   - WebSocket (eventy w czasie rzeczywistym)
2. **Bluetooth (bonus dla Androida)** — Web Bluetooth API. Komunikacja przez GATT characteristics (CMD do wysyłania komend, EVT do odbierania eventów). NIE działa na iOS (Apple nie wspiera Web Bluetooth).

3. **USB / Web Serial (tryb zaawansowany)** — dla developera/serwisu. Komunikacja JSON Lines @ 115200 baud. Tylko Chrome/Edge na desktopie.

### Protokół komunikacji

- Format: **JSON Lines** (jedna linia JSON = jedna wiadomość, zakończona `\n`)
- Host → urządzenie: `{"cmd": "ping"}\n`
- Urządzenie → host: `{"ev": "pong", "ts": 12345}\n`
- Przez WiFi komendy idą jako POST na `/api/cmd`, eventy przez WebSocket na `/api/events`.

### API urządzenia (endpointy HTTP na ESP32)

- `GET /api/hello` — ping, sprawdzenie czy urządzenie odpowiada
- `GET /api/books` — lista książek na urządzeniu
- `POST /api/books` — upload pliku (multipart/form-data)
- `DELETE /api/books/:name` — usunięcie książki
- `GET /api/settings` — pobranie ustawień (format firmware: zagnieżdżony JSON z sekcjami reading/display/developer)
- `PUT /api/settings` — zmiana ustawień (adapter w kodzie mapuje płaski model UI na format firmware)
- `POST /api/ota` — upload firmware .bin do aktualizacji OTA (urządzenie restartuje się po sukcesie)
- WebSocket `/api/events` — strumień eventów z urządzenia

### Konwerter plików (działa w przeglądarce, offline)

Konwertuje pliki na autorski format `.rsvp` (Rapid Serial Visual Presentation — wyświetlanie słowo po słowie):

- **EPUB** → rozpakowuje ZIP, parsuje OPF (metadata), iteruje po spine (XHTML), wyciąga rozdziały i paragrafy
- **PDF** → używa `pdfjs-dist`, wyciąga tekst strona po stronie, dzieli na akapity
- **TXT** → dzieli po pustych liniach na paragrafy
- **Markdown** → rozpoznaje nagłówki (#, setext), stripuje inline formatting
- **HTML** → parsuje DOM, rozpoznaje `<h1>`-`<h6>` jako rozdziały, blokowe elementy jako paragrafy

Format wyjściowy `.rsvp`:

```
@rsvp 1
@title Tytuł książki
@author Autor
@source plik.epub

@chapter Rozdział 1
Tekst paragrafu zawinięty do 96 znaków na linię...
Kolejna linia tego samego paragrafu.

@chapter Rozdział 2
...
```

### Biblioteka (zarządzanie książkami)

- Lista książek z urządzenia (tytuł, autor, rozmiar, % przeczytania, kategoria: book/article)
- Upload plików .rsvp/.txt/.epub na urządzenie
- Usuwanie książek
- Filtrowanie: wszystko / książki / artykuły

### Ustawienia urządzenia

Pełna konfiguracja czytnika z poziomu telefonu:

- **Wyświetlanie**: motyw (jasny/ciemny/nocny), jasność (0-100%), ręka (prawa/lewa)
- **Czytanie**: tryb (RSVP/przewijanie), pauza (tap/przytrzymanie/auto), tempo (WPM 50-1000), opóźnienia dla długich/złożonych słów i interpunkcji
- **HUD**: bateria, rozdział, procent — widoczne podczas czytania
- **Język**: pl/en/de/es/fr/it
- **Tryb developera**: ukryty, odblokowanie przez 10-krotne tapnięcie w logo (jak w Androidzie)

Adapter w kodzie tłumaczy płaski model UI (`DeviceSettings`) na zagnieżdżony format firmware (`FirmwareSettings` z sekcjami `reading`, `display`, `developer`).

### Aktualizacje OTA

- Pobiera ostatni release z GitHub API (`/repos/GRKarol/czytnik01/releases/latest`)
- Wyświetla changelog, porównuje wersje (semver)
- Pobiera .bin na telefon (z progress barem)
- Wysyła .bin na urządzenie przez `POST /api/ota` (XMLHttpRequest dla progress uploadu)
- Po sukcesie urządzenie się restartuje

### Pluginy (planowane)

- Klepsydra (timer czytania 25/5)
- Dyktafon (notatki głosowe)
- Odtwarzacz muzyki (muzyka tła z SD)
- Indeks pluginów ładowany z `/plugins/index.json`

### Onboarding (pierwszy raz)

3-krokowy wizard:

1. Powitanie — co to za aplikacja
2. Instalacja PWA — na iOS instrukcja Share Sheet, na Androidzie `beforeinstallprompt`
3. Instrukcja połączenia WiFi

### Tryb offline / Mock

Aplikacja działa BEZ podłączonego urządzenia — używa `MockDeviceApi` (dane w localStorage). Kiedy użytkownik połączy się przez WiFi, API przełącza się na `HttpDeviceApi`. Komponenty UI nie wiedzą o różnicy (ten sam interface `DeviceApi`).

### Struktura kodu

```
src/app/
├── main.ts                    — entry point, rejestracja Service Worker (PWA)
├── app.element.ts             — główny komponent <czytnik-app>, routing, logika połączenia
├── components/
│   ├── converter-panel.element.ts   — UI konwertera
│   ├── library-panel.element.ts     — zarządzanie biblioteką
│   ├── settings-panel.element.ts    — panel ustawień
│   ├── updates-panel.element.ts     — aktualizacje OTA
│   ├── onboarding.element.ts        — wizard pierwszego uruchomienia
│   ├── pwa-install-dialog.element.ts — dialog instalacji PWA
│   └── flower-decor.element.ts      — dekoracyjne kwiatki w tle
├── converter/
│   ├── index.ts        — detekcja formatu, dispatcher
│   ├── epub.ts         — parser EPUB (JSZip + DOMParser)
│   ├── pdf.ts          — parser PDF (pdfjs-dist)
│   ├── text-formats.ts — parsery TXT, Markdown, HTML
│   └── rsvp.ts         — model danych + serializer do .rsvp
├── device/
│   ├── device-link.ts     — interface DeviceLink (connect/disconnect/send/onEvent)
│   ├── wifi-link.ts       — implementacja WiFi (fetch + WebSocket)
│   ├── bluetooth-link.ts  — implementacja BLE (Web Bluetooth)
│   ├── serial-link.ts     — implementacja USB (Web Serial)
│   ├── api.ts             — wyższy poziom API (listBooks, uploadBook, settings, OTA) + MockDeviceApi
│   └── http-api.ts        — HttpDeviceApi (real HTTP do urządzenia) + adapter firmware↔UI
└── updates/
    └── releases.ts        — GitHub Releases API, porównanie wersji, download assetów

src/shared/
├── config.ts           — URL-e, branding, UUID BLE, wersja
├── device-protocol.ts  — format ramek JSON Lines, encode/parse
└── env.d.ts            — typy Vite/PWA
```

### Technologie

- **Lit** (Web Components, reactive properties, scoped CSS)
- **Vite** (bundler, PWA plugin, HMR)
- **TypeScript** (strict)
- **Service Worker** (vite-plugin-pwa) — pełna praca offline
- **JSZip** — rozpakowywanie EPUB
- **pdfjs-dist** — ekstrakcja tekstu z PDF
- **Web APIs**: Web Bluetooth, Web Serial, WebSocket, Fetch, File API, Drag & Drop

---

## Wspólny kod (shared)

- `config.ts` — stałe: nazwa marki ("Flower"), URL-e (manifest firmware, app, flasher, pluginy, GitHub releases), prefix SSID AP (`Flower-`), adres IP urządzenia (`192.168.4.1`), UUID serwisu BLE
- `device-protocol.ts` — definicja formatu komunikacji (JSON Lines), funkcje `encodeCommand()` i `parseEvent()`
- `env.d.ts` — deklaracje typów dla Vite

---

## Podsumowanie architektury

```
┌─────────────────────────────────────────────────────────┐
│                    UŻYTKOWNIK                            │
├─────────────────────┬───────────────────────────────────┤
│  Flasher (desktop)  │  Aplikacja PWA (telefon)          │
│  Chrome/Edge        │  iOS Safari / Android Chrome      │
│  USB + Web Serial   │  WiFi / BLE / USB                 │
│  ESP Web Tools      │  Lit + Vite + Service Worker      │
├─────────────────────┴───────────────────────────────────┤
│              ESP32-S3 (Flower / czytnik01)               │
│  - AP WiFi "Flower-XXXX" @ 192.168.4.1                  │
│  - HTTP REST API + WebSocket                            │
│  - BLE GATT Service                                     │
│  - Serial 115200 baud (JSON Lines)                      │
│  - OTA update endpoint                                  │
│  - Czytnik e-booków (format .rsvp, RSVP word-by-word)   │
└─────────────────────────────────────────────────────────┘
```
