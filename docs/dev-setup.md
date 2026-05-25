# Dev setup

Co potrzeba do pracy lokalnej.

## Wymagania

- **Node.js 20+** (LTS): https://nodejs.org/
- Przeglądarka z Web Serial: Chrome lub Edge (desktop). Firefox/Safari nie wspierają.
- Do flashowania: ESP32-S3 podłączony przez USB-C, kabel z transmisją danych.

## Pierwsze uruchomienie

```bash
npm install
npm run dev
```

Otwiera się `http://localhost:5173/`. Flasher pod `/`, PWA pod `/app/`.

## Sprawdzenie typów

```bash
npm run typecheck
```

CI puszcza to przy każdym pushu na nie-main i przy PR-ach na main.

## Build produkcyjny

```bash
npm run build
npm run preview
```

`dist/` to artefakt deployowalny. `npm run preview` uruchamia statyczny
serwer pod `http://localhost:4173/` żeby zobaczyć build "jak prod".

## Testowanie PWA / install promptu

PWA install prompt **wymaga HTTPS** — `npm run dev` używa HTTP, więc
service worker będzie wyłączony (`vite-plugin-pwa.devOptions.enabled = false`).

Żeby przetestować PWA:

```bash
npm run build
npm run preview     # http://localhost:4173/app/
```

Chrome DevTools → Application → Service Workers → sprawdź, czy SW jest
zarejestrowany. Manifest sprawdź w Application → Manifest.

## Testowanie połączenia z urządzeniem bez prawdziwego ESP

Można podpiąć dowolny USB-Serial (np. CP2102, CH340) i wysłać do niego
JSON Linesy z poziomu terminala (`screen`, `minicom`, `pio device monitor`)
żeby zasymulować urządzenie. Aplikacja PWA połączy się tak samo —
Web Serial nie rozróżnia "prawdziwego" ESP32 od konwertera.

## Dodanie nowego firmware'u do testów

1. Skopiuj scaloną binarkę do `public/firmware/czytnik01.bin`.
2. Sprawdź, że `public/firmware/manifest.json` ma `chipFamily` zgodny
   z hardware (`ESP32-S3` domyślnie).
3. `npm run dev` → otwórz `/` → kliknij "Zainstaluj firmware".

> Binarka nie jest commitowana — `public/firmware/*.bin` w `.gitignore`.
