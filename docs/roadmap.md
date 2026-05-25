# Roadmap

Plan dalszej pracy. Lista, nie kontrakt — układ ma sens dopiero po
przemyśleniu z klientem hardware'u.

## Faza 0 — Podkładka (✅ done)

- [x] Wipe starego repo, orphan branch.
- [x] Scaffold Vite + TS + Lit + vite-plugin-pwa.
- [x] Strona flashera z esp-web-tools, manifest `public/firmware/manifest.json`.
- [x] Stub PWA klienta.
- [x] CI: build + auto-deploy na GitHub Pages.
- [x] Dokumentacja architektury, flow QR.

## Faza 1 — Rebrand + nowy transport (✅ done w tej rundzie)

- [x] Skopiowano firmware z `ionutdecebal/rsvpnano` do `firmware/`.
- [x] Wymiana abstrakcji urządzenia: `DeviceLink` + 3 implementacje:
      `WifiLink` (główny, HTTP+WS), `BluetoothLink` (bonus Androida),
      `SerialLink` (tryb advanced/diagnostyczny).
- [x] Rebrand: motyw **Flower** — jasnoniebieskie niebo, kwiatkowe akcenty,
      książkowa serif typografia, kwiatkowe logo.
- [x] 6 ekranów aplikacji (na razie szkielety): Start, Książki, Konwerter,
      Pluginy, Aktualizacje, Więcej.
- [x] Wybór połączenia jako wizard (WiFi / Bluetooth / USB advanced).
- [x] Sklep pluginów: `public/plugins/index.json` z 3 wpisami
      (Klepsydra gotowa, Dyktafon i Muzyka planowane).

## Faza 2 — Logika ekranów aplikacji (TO JEST NASTĘPNIE)

### 2.1 Konwerter formatów (w przeglądarce)

Wsparcie dla każdego źródła, wyjście zawsze `.rsvp`:

- [ ] `.txt` / `.md` / `.html` — trywialne, parser stringów.
- [ ] `.epub` — biblioteka `epubjs` lub własny parser z `jszip`.
- [ ] `.pdf` — `pdf.js`, wyciągamy text layer.
- [ ] `.mobi` / `.azw` — `kindleunpack-js` lub konwersja przez wewnętrzny EPUB.
- [ ] Drag-and-drop + file picker (mobilne też!).
- [ ] Edytor metadanych (tytuł, autor) przed konwersją.
- [ ] Lokalna pamięć podręczna skonwertowanych książek (IndexedDB).

### 2.2 Biblioteka

- [ ] Pobieranie listy książek z urządzenia (`GET /api/library`).
- [ ] Upload książki (`POST /api/library` multipart).
- [ ] Usuwanie, zmiana kolejności, kategoryzacja.

### 2.3 Pluginy

- [ ] Pobieranie listy z `public/plugins/index.json`.
- [ ] Pobieranie paczki `.zip` przez `fetch`.
- [ ] Wysyłka na urządzenie (`POST /api/plugins/install`).
- [ ] Lista zainstalowanych pluginów na urządzeniu + usuwanie.

### 2.4 Aktualizacje firmware

- [ ] Sprawdzanie GitHub Releases (`OTA_RELEASES_API` w configu).
- [ ] Porównanie z aktualną wersją na urządzeniu.
- [ ] Pobranie `.bin` i wysyłka na urządzenie (`POST /api/ota`).
- [ ] Pasek postępu, retry on fail.

## Faza 3 — Firmware: dodanie API WiFi/BT

Decyzje do podjęcia z Karolem:

- [ ] Rebrand stringów RSVP Nano → Flower w `firmware/src/`.
- [ ] WiFi mode dla aplikacji: AP-only, czy konfigurowalny (AP + STA)?
- [ ] HTTP API: jakie endpointy (`/api/hello`, `/api/library`, `/api/cmd`,
      `/api/ota`, `/api/plugins/*`).
- [ ] WebSocket dla eventów real-time.
- [ ] BLE GATT: definicja serwisu (UUID już w configu), charakterystyki
      CMD/EVENT.
- [ ] Plugin runtime: jak ładujemy paczki ZIP, gdzie trzymamy na SD,
      jak firmware je czyta / włącza w menu.
- [ ] OTA: integracja z `firmware/src/update/OtaUpdater.cpp` —
      retarget endpointa z rsvpnano na nasz GitHub Releases.

## Faza 4 — Onboarding / dystrybucja

- [ ] Pierwszorazowy wizard po pobraniu PWA (3 ekrany: cześć / połącz /
      gotowe).
- [ ] Detekcja iOS — dodatkowa podpowiedź "Dodaj do ekranu głównego"
      z ikonografią Share Sheet.
- [ ] Generator etykiet QR na pudełka (skrypt w `firmware/tools/`?).
- [ ] Domena własna (np. `flower.app` / `app.flower.pl`).
- [ ] Sygnowanie firmware (opcjonalnie, dla anti-tampering).

## Otwarte pytania

- Czy konwerter ma robić batch (wiele plików naraz)?
- Czy biblioteka na urządzeniu ma kategorie / tagi, czy płaska lista?
- Plugin "Klepsydra" już jest jako `firmware/src/timer/FocusTimer.cpp` —
  decyzja: zostaje wbudowany i plugin tylko go włącza, czy wyrzucamy
  z firmware i dystrybuujemy wyłącznie jako paczkę plugin?
- Jak nazywamy paczki pluginów w UI — "Pluginy", "Rozszerzenia",
  "Dodatki", coś jeszcze?
