# Flower / Czytnik01

Web flasher + PWA dla urządzenia **Flower** (czytnik RSVP) opartego o **ESP32-S3**.

Repo zawiera trzy spójne kawałki:

| Ścieżka      | Co to                            | Dla kogo                       |
| ------------ | -------------------------------- | ------------------------------ |
| `/`          | Web flasher (USB)                | Serwis / Karol                 |
| `/app/`      | PWA klienta (WiFi + BLE)         | Codzienne użycie klienta       |
| `firmware/`  | Kod ESP32-S3 (vendor: rsvpnano)  | Build osobno PlatformIO        |

Frontend (flasher + PWA) hostowany statycznie na GitHub Pages — nic nie
wymaga backendu. Konwersja formatów książek dzieje się w przeglądarce
(offline). Komunikacja z urządzeniem przez **WiFi (HTTP + WebSocket)**
jako główny tor, **Bluetooth (Web BT)** jako bonus dla Androida,
**USB (Web Serial)** w trybie advanced/diagnostyki.

## Stack

- **Vite** + **TypeScript**
- **Lit** (Web Components) — UI flashera i aplikacji
- **vite-plugin-pwa** (Workbox) — instalowalność + offline dla `/app/`
- **esp-web-tools** — flashowanie ESP32-S3 z poziomu Chrome/Edge przez Web Serial
- Komunikacja z urządzeniem po flashowaniu: trzy implementacje wspólnego
  interfejsu `DeviceLink` (`src/app/device/`):
  - `WifiLink` — HTTP + WebSocket do AP urządzenia (główny tor, iOS+Android)
  - `BluetoothLink` — Web Bluetooth (bonus, tylko Android Chrome — iOS nie wspiera)
  - `SerialLink` — Web Serial / USB (tryb advanced, diagnostyka)

## Struktura

```
.
├── index.html                  # landing + flasher
├── app/
│   └── index.html              # PWA klienta (instalowalna z QR)
├── src/
│   ├── flasher/                # kod strony flashera
│   ├── app/
│   │   ├── components/         # współdzielone komponenty UI (install-prompt, flower-decor)
│   │   └── device/             # 3 transporty: wifi, bluetooth, serial
│   └── shared/                 # protokół, config, typy
├── public/
│   ├── firmware/               # manifest.json + .bin (nie commitowane)
│   ├── plugins/                # statyczny sklep pluginów (index.json + paczki)
│   └── icons/                  # ikony PWA / favicon
├── firmware/                   # kod C++ na ESP32-S3 (vendored: rsvpnano)
├── docs/                       # architektura, plan, flow QR
├── .github/workflows/          # CI + auto-deploy na GitHub Pages
├── vite.config.ts              # multi-page input + plugin PWA
└── package.json
```

## Praca lokalna

Wymagania: Node.js 20+.

```bash
npm install
npm run dev          # http://localhost:5173  i  http://localhost:5173/app/
npm run build        # buduje do dist/
npm run preview      # podgląd buildu z lokalnego serwera
npm run typecheck    # TS bez emitowania
```

Web Serial wymaga **HTTPS lub localhost** — dev server na `localhost:5173` działa.
Z innego urządzenia w sieci (np. telefon) musisz wystawić tunel HTTPS, np.
`npx http-server dist -S` z certyfikatem, albo użyć `cloudflared tunnel`.

## Firmware

Plik binarny firmware (`czytnik01.bin`) wrzuć do `public/firmware/`. Szczegóły:
[`public/firmware/README.md`](public/firmware/README.md).

Firmware nie jest częścią tego repo — to repo to **tylko warstwa webowa**.
Repo firmware (PlatformIO/ESP-IDF) zostanie założone osobno.

## Deploy

Push na branch `main` → GitHub Actions buduje i deployuje na GitHub Pages.
Workflow: [`.github/workflows/deploy.yml`](.github/workflows/deploy.yml).

Aby Pages serwowało pod ścieżką `/czytnik01/`, build w CI ustawia
`VITE_BASE=/czytnik01/`. Lokalnie domyślnie używa `/`.

## Plan dalszej pracy

Patrz [`docs/architecture.md`](docs/architecture.md) i
[`docs/roadmap.md`](docs/roadmap.md).

## Licencja

TBD.
