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

## Podgląd ekranów bez czytnika (symulator nanosim)

Ekrany trybu nawigacji *Nowoczesny* (skórka Nano) da się obejrzeć na
komputerze, bez wgrywania firmware. To nie jest emulator ESP32. Czytnik
rysuje każdą klatkę do bufora 640×172 w RAM i dopiero potem wysyła ją na
panel. Symulator kompiluje te same pliki rysujące
(`firmware/src/display/DisplayManager.cpp`, `firmware/src/ui/NanoScreens.cpp`)
pod zwykły komputer i zapisuje ten bufor do pliku obrazka.

Kod w `firmware/tools/nanosim/`:

| Plik               | Co robi                                                              |
| ------------------ | -------------------------------------------------------------------- |
| `sim_screens.cpp`  | ustawia każdy ekran (zakładka, przykładowe książki, bateria 76%) i robi zrzut |
| `sim_plugins.cpp`  | to samo dla pluginów (Dyktafon, Klepsydra)                           |
| `sim_main.cpp`     | zapis bufora do `.ppm`                                               |
| `stubs/`           | atrapy Arduino, SD_MMC i logów ESP, żeby kod skompilował się na PC   |
| `build.sh`         | kompiluje i odpala symulator                                         |
| `sheet.py`         | skleja wybrane zrzuty w jeden PNG                                    |

Wymagania: g++ (na Windowsie przez WSL z Ubuntu) i Python z Pillow
(`pip install pillow`).

```powershell
cd firmware
wsl -d Ubuntu -- bash tools/nanosim/build.sh
cd tools\nanosim
python sheet.py podglad read settings library device
```

Pierwsza komenda zapisuje wszystkie ekrany do `tools/nanosim/out/*.ppm`
(otwiera je np. GIMP albo IrfanView). Każdy ekran ma też wersję
`*_targets.ppm` z fioletowymi ramkami w miejscach, które reagują na dotyk.
`sheet.py <nazwa_wyniku> <ekran> <ekran> ...` skleja podane ekrany w
`out/<nazwa_wyniku>.png`. Ekran podajesz nazwą pliku bez `.ppm`, np. `read`,
`settings`, `themes0`, `library`, `chapters`, `device`, `reader_panel`,
`plugin_dict_home`. Wersje z czcionką Literata mają końcówkę `_literata`.

Nowy ekran trzeba dopisać do `sim_screens.cpp` (albo `sim_plugins.cpp`)
wywołaniem `frame("nazwa", zakładka, [&] { ... })`, wzorując się na
istniejących. Symulator robi statyczne zrzuty, klikać w nim się nie da.
Kolory na monitorze wyglądają trochę inaczej niż na panelu czytnika.

## Deploy

Push na branch `main` → GitHub Actions buduje i deployuje na GitHub Pages.
Workflow: [`.github/workflows/deploy.yml`](.github/workflows/deploy.yml).

Aby Pages serwowało pod ścieżką `/czytnik01/`, build w CI ustawia
`VITE_BASE=/czytnik01/`. Lokalnie domyślnie używa `/`.

## Workflow: staging vs release

Dwa zdalne repo, dwie różne role:

| Remote    | Adres                              | Widoczność | Rola                                    |
| --------- | ----------------------------------- | ---------- | ---------------------------------------- |
| `staging` | GRKarol/czytnik01-staging            | prywatne   | codzienna praca, testy, buildy .apk debug |
| `origin`  | GRKarol/czytnik01                    | publiczne  | jedyne źródło GitHub Pages i release'ów   |

Cała bieżąca praca (redesign, nowe funkcje, poprawki) idzie na `staging`.
Push na `origin/main` uruchamia realny deploy na GitHub Pages — dlatego
tam trafia wyłącznie to, co Karol już przetestował i wyraźnie potwierdził.

Zasada dla Claude: nigdy nie pushować na `origin` samodzielnie. Domyślny cel
push to `staging`. Push na `origin` (main albo release/*) wymaga jawnego
"tak, wypchnij na główne repo" od Karola — nie wystarczy samo "wygląda dobrze"
czy zaakceptowanie buildu .apk, bo to tylko test na staging.

## Plan dalszej pracy

Patrz [`docs/architecture.md`](docs/architecture.md) i
[`docs/roadmap.md`](docs/roadmap.md).

## Licencja

TBD.
