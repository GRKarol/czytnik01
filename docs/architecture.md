# Architektura

## Cel

Klient kupuje urządzenie **Czytnik01** (ESP32-S3) z już zainstalowanym firmware.
Z tym urządzeniem klient pracuje przez aplikację webową (PWA), którą instaluje
sobie na telefonie/komputerze skanując kod QR.

Dwa publiczne "punkty wejścia":

1. **Web flasher** (`/`) — używamy my (serwis / przygotowanie sprzętu) albo
   advanced user. Pozwala wgrać/zaktualizować firmware z poziomu Chrome/Edge.
2. **PWA klienta** (`/app/`) — to dostaje klient. Skanuje QR ⇒ przeglądarka
   otwiera stronę ⇒ klient klika "Dodaj do ekranu głównego" ⇒ aplikacja staje
   się ikoną. Wewnątrz aplikacji klient łączy się z urządzeniem (Web Serial
   przez USB-C, w przyszłości być może Web Bluetooth).

Oba interfejsy są statycznymi plikami serwowanymi z GitHub Pages — żadnego
backendu nie potrzebujemy.

## Warstwy

```
┌─────────────────────────────────────────────────────────────┐
│  PWA klienta (/app/)         │   Web flasher (/)            │
│  - Lit components            │   - Lit components           │
│  - Web Serial / Web BT       │   - esp-web-install-button   │
│  - Workbox (offline cache)   │                              │
├─────────────────────────────────────────────────────────────┤
│  src/shared/   protokół, config, wersja                     │
├─────────────────────────────────────────────────────────────┤
│  Urządzenie Czytnik01 (ESP32-S3)                            │
│  - firmware komunikuje się przez Serial @ 115200 (JSON Lines)│
└─────────────────────────────────────────────────────────────┘
```

## Dlaczego ten stack

- **Vite + TS** — szybki dev loop, dobre typowanie.
- **Lit (Web Components)** — małe runtime (~5 kB), natywne web components,
  działa bez Reacta/wirtualnego DOM. `esp-web-install-button` jest też web
  componentem, więc nic nie trzeba opakowywać.
- **vite-plugin-pwa** — generuje service worker (Workbox), manifest PWA,
  i daje `registerSW()` z auto-update.
- **esp-web-tools** — sprawdzony, używany przez Home Assistant / ESPHome.
  Pod spodem `esptool-js`.

## Komunikacja host ↔ urządzenie

Domyślnie **JSON Lines po Web Serial** @ 115200 baud:

- host → device: `{"cmd": "<nazwa>", ...}\n`
- device → host: `{"ev": "<nazwa>", ...}\n`

Plus reszta linii to nieaktywny szum (np. logi printf z firmware), które
parser ignoruje.

Zmiana transportu (USB CDC → BLE) wymaga tylko podmiany klasy
`SerialLink` na inną implementację `DeviceLink`. Reszta aplikacji jest
agnostyczna.

## Flow użytkownika

1. **Produkcja / serwis:**
   - Otwieramy https://grkarol.github.io/czytnik01/ na laptopie.
   - "Zainstaluj firmware" → ESP Web Tools wgrywa `czytnik01.bin`.
2. **Wysyłka:**
   - Klient dostaje urządzenie + kartę z kodem QR prowadzącym do
     `https://grkarol.github.io/czytnik01/app/`.
3. **Pierwsze uruchomienie u klienta:**
   - Skanuje QR → otwiera się PWA w domyślnej przeglądarce.
   - Banner "Dodaj do ekranu głównego" (lub native install prompt na
     Androidzie/Edge) — `czytnik-install-prompt` w aplikacji wywołuje
     `beforeinstallprompt`.
   - Klient klika "Zainstaluj" → ikona aplikacji na home screen.
   - W aplikacji: "Połącz urządzenie" → wybór portu USB → gotowe.

## Granice tego repo

Co jest tu:

- Web flasher
- PWA klienta
- Wspólna konfiguracja, protokół komunikacji
- CI / deploy

Czego tu nie ma (i być nie powinno):

- **Firmware** (osobne repo — PlatformIO / ESP-IDF). To repo tylko konsumuje
  scaloną binarkę przez `public/firmware/czytnik01.bin`.
- Backend / chmura — projekt jest pure-frontend / pure-device. Jeśli pojawi
  się potrzeba synca między urządzeniami → osobna usługa, osobne repo.
