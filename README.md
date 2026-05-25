# Czytnik01

Web flasher + PWA dla urządzenia **Czytnik01** opartego o **ESP32-S3**.

Repo zawiera dwie aplikacje webowe budowane wspólnie przez Vite:

| Ścieżka  | Co to              | Dla kogo                       |
| -------- | ------------------ | ------------------------------ |
| `/`      | Web flasher        | Pierwsze uruchomienie / serwis |
| `/app/`  | PWA klienta        | Codzienne użycie               |

Wszystko hostowane statycznie na GitHub Pages — nic nie wymaga backendu.

## Stack

- **Vite** + **TypeScript**
- **Lit** (Web Components) — UI flashera i aplikacji
- **vite-plugin-pwa** (Workbox) — instalowalność + offline dla `/app/`
- **esp-web-tools** — flashowanie ESP32-S3 z poziomu Chrome/Edge przez Web Serial
- Komunikacja z urządzeniem po flashowaniu: **Web Serial** (potencjalnie też Web Bluetooth
  w przyszłości — interfejs `DeviceLink` w `src/app/device/` jest do tego przygotowany)

## Struktura

```
.
├── index.html                  # landing + flasher
├── app/
│   └── index.html              # PWA klienta (instalowalna z QR)
├── src/
│   ├── flasher/                # kod strony flashera
│   ├── app/
│   │   ├── components/         # współdzielone komponenty UI
│   │   ├── device/             # komunikacja z urządzeniem (Web Serial)
│   │   └── pages/              # ekrany aplikacji
│   └── shared/                 # protokół, config, typy
├── public/
│   ├── firmware/               # manifest.json + .bin (nie commitowane)
│   └── icons/                  # ikony PWA (TODO: realne logo)
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
