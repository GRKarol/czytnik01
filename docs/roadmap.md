# Roadmap

> **Zaktualizowano 2026-07-21** po przeglądzie całego repo (lokalnego i
> `main` na GitHubie), repo-inspiracji ([`ionutdecebal/rsvpnano`](https://github.com/ionutdecebal/rsvpnano))
> i realnych testów połączenia WiFi. Fazy 0-4 poniżej to zapis historyczny —
> większość była już zrobiona, tylko nikt nie odhaczał checkboxów na bieżąco.
> Aktywny plan zaczyna się od **Fazy 5**.

Plan dalszej pracy. Lista, nie kontrakt.

---

## Fazy 0-4 — zrobione (zapis historyczny)

### Faza 0 — Podkładka (✅ done)

- [x] Wipe starego repo, orphan branch.
- [x] Scaffold Vite + TS + Lit + vite-plugin-pwa.
- [x] Strona flashera z esp-web-tools, manifest `public/firmware/manifest.json`.
- [x] Stub PWA klienta.
- [x] CI: build + auto-deploy na GitHub Pages.

### Faza 1 — Rebrand + nowy transport (✅ done)

- [x] Skopiowano firmware z `ionutdecebal/rsvpnano` do `firmware/`.
- [x] `DeviceLink` + 3 implementacje: `WifiLink`, `BluetoothLink`, `SerialLink`.
- [x] Rebrand: motyw **Flower**.
- [x] 6 ekranów aplikacji, wybór połączenia jako wizard.
- [x] Sklep pluginów: `public/plugins/index.json`.

### Faza 2 — Logika ekranów (✅ done, mimo że stary roadmap tego nie pokazywał)

- [x] Konwerter formatów: `.txt`/`.md`/`.html`/`.epub` (`src/app/converter/`).
- [x] Biblioteka: `GET/POST/DELETE /api/books`, pozycja czytania (`/api/books/position`).
- [x] Pluginy: lista, instalacja, usuwanie (`/api/plugins`), w tym RSS jako plugin.
- [x] OTA: sprawdzanie GitHub Releases, upload `.bin`, pasek postępu.

### Faza 3 — Firmware WiFi/BLE API (✅ done, poszło dalej niż plan)

- [x] Pełne HTTP API opisane w [`docs/flower-companion-api.md`](flower-companion-api.md):
      `/api/hello`, `/api/state`, `/api/settings`, `/api/books`, `/api/wifi`,
      `/api/rss-feeds`, `/api/ota`, `/api/plugins`, `/api/log/*`, `/api/power/wifi-timeout`.
- [x] BLE peripheral (NimBLE) — `firmware/src/ble/BleApi.cpp` (commit `1e958ae`).
- [x] UDP broadcast discovery (port 5555, v0.3.6) — wykrycie czytnika w <50ms.
- [x] Captive portal bypass (204 na wszystko poza `/api/`) — eliminuje "!" na Androidzie.
- [x] **Kod QR na ekranie czytnika** w trybie Sync (commit `cb65c20`) —
      standardowy format `WIFI:T:nopass;S:Flower-XXXX;;`, natywny aparat
      telefonu (Android i iOS) sam rozpoznaje i łączy jednym tapnięciem.

### Faza 4 — Onboarding (częściowo done)

- [x] Tutorial wizard po pierwszym połączeniu (`tutorial-wizard.element.ts`).
- [x] Help panel + tooltips ustawień (`help-panel.element.ts`, `setting-tooltip.element.ts`).
- [ ] Wykorzystanie kodu QR z Fazy 3 jako **głównej** ścieżki w onboardingu appki
      (patrz Faza 7 poniżej — to jeszcze nie jest spięte z UX appki).

---

## Decyzje architektoniczne (2026-07-21)

Poniższe **anuluje i zastępuje** wcześniejsze założenia z tego dokumentu:

1. **Rozdział funkcji WiFi/Bluetooth — anulowany.** Wcześniej rozważane
   "połowa funkcji przez WiFi, połowa przez Bluetooth" jest zbędne. WiFi
   (HTTP + WebSocket do `192.168.4.1`) wystarcza jako jedyny wymagany tor,
   działa identycznie na iOS i Androidzie. BLE zostaje w kodzie jako
   **opcjonalny bonus dla Androida**, nie jako wymagany drugi tor.
2. **Offline-first nie koliduje ze sprawdzaniem aktualizacji w tle.**
   To nie jest jeden tryb appki — to dwa różne momenty w czasie: appka gada
   z czytnikiem TYLKO gdy telefon jest w sieci `Flower-XXXX` (zero internetu
   wtedy), i sprawdza GitHub Releases w tle TYLKO gdy telefon ma normalny
   internet (czyli praktycznie przez resztę dnia). Appka rozpoznaje to po
   tym, w jakiej sieci aktualnie jest — nie trzeba nic dzielić ręcznie.
3. **Android → Capacitor, iOS → zostaje na PWA (decyzja 2026-07-21).**
   Powód: PWA hostowana na HTTPS (GitHub Pages) jest blokowana przez
   przeglądarkę przy próbie połączenia z czytnikiem po zwykłym HTTP
   (mixed content policy — to dotyczy **każdego** hostingu HTTPS, nie da
   się tego naprawić zmianą hostingu). Capacitor owija istniejący kod
   TypeScript/Lit w natywną powłokę na Androida, która tej blokadzie nie
   podlega, bez przepisywania appki. iOS zostaje z obecnym ograniczeniem
   (workaround: ręczne wejście na `http://192.168.4.1`), dopóki nie
   zapadnie decyzja o Apple Developer Program (99$/rok) — bez tego appka na
   iOS nie utrzyma się dłużej niż 7 dni bez ponownego podpisywania z Xcode.
4. **Dystrybucja na Androida: sideload, nie Play Store.** Podpisany APK do
   pobrania bezpośrednio ze strony (GitHub Pages/Releases) — Android
   pozwala instalować appki spoza Play Store, więc nie ma potrzeby recenzji
   Google ani opłat.

Odniesienie: `ionutdecebal/rsvpnano` (inspiracja pierwotna) poszedł inną
drogą — dwie osobne natywne appki (Kotlin Multiplatform + Jetpack Compose /
SwiftUI) z tym samym trickiem "wyłącz blokadę HTTP" (`usesCleartextTraffic`
/ `NSAllowsArbitraryLoads`). Świadomie tego nie kopiujemy — Capacitor daje
ten sam efekt bez utrzymywania dwóch kodowych baz. Ich rozwiązanie ma też
te same braki co nasze (brak auto-reconnect, brak publicznej dystrybucji).

---

## Faza 5 — Natywna appka Android (Capacitor)

- [ ] Dodać Capacitor do projektu (`@capacitor/core`, `@capacitor/android`).
- [ ] Konfiguracja Androida: `usesCleartextTraffic="true"` (albo
      `networkSecurityConfig` scoped do `192.168.4.1`, bezpieczniej).
- [ ] `ConnectivityManager.bindProcessToNetwork` — przypięcie appki na
      sztywno do sieci czytnika, żeby Android nie odcinał jej w tle uznając
      sieć za "bez internetu".
- [ ] `@capacitor/share` — eksport pliku `.rsvp` do innych aplikacji
      (funkcja #8 z wymagań: "udostępnij znajomemu").
- [ ] Natywny share target (odbieranie linków/tekstu z innych aplikacji,
      jak w `rsvpnano`) — opcjonalnie, jeśli czas pozwoli.
- [ ] Background task (WorkManager) do okresowego sprawdzania GitHub
      Releases, gdy appka nie jest otwarta i telefon ma internet.
- [ ] Build podpisanego APK, publikacja jako asset w GitHub Releases obok
      firmware — do pobrania bezpośrednio ze strony.
- [ ] Test na realnym telefonie (nie tylko emulator).

## Faza 6 — Niezawodność połączenia (P0 — realny, potwierdzony bug)

- [ ] **BUG:** w `src/app/app.element.ts`, metoda `connect()` — jeśli
      `pingDevice()` zawiedzie choćby raz tuż po połączeniu WiFi (bardzo
      częste), appka **po cichu zostaje na `MockDeviceApi`** (fikcyjne dane
      demo) zamiast pokazać błąd albo spróbować ponownie. To jest
      zdiagnozowana przyczyna "starych książek, nic się nie zgrywa" z
      testów. Naprawa: retry z backoffem zamiast jednorazowego sprawdzenia,
      i jasny komunikat błędu zamiast cichego fallbacku.
- [ ] Keep-alive: `GET /api/hello` co ~8s, wykrycie rozłączenia w <16s
      (zasada już opisana w `docs/flower-companion-api.md`, nie
      zaimplementowana w `wifi-link.ts`).
- [ ] Auto-reconnect po zerwaniu WebSocketu (obecnie: raz zerwane
      połączenie nigdy się samo nie odnawia).
- [ ] UI: wyraźny stan "rozłączono" widoczny dla użytkownika zamiast
      cichego zawieszenia się w stanie "połączono" z martwym socketem.
- [ ] Kolejkowanie requestów — nie wysyłać dwóch `PATCH /api/settings`
      naraz (ESP32 jest single-threaded, może się pogubić).

## Faza 7 — Onboarding przez QR

- [x] Firmware generuje QR (Faza 3) — tylko trzeba to podciągnąć lokalnie.
- [ ] Onboarding appki: QR jako **główna** prowadzona ścieżka połączenia
      ("otwórz aparat, zeskanuj kod na czytniku"), ręczne WiFi ustawienia
      jako opcja zapasowa, nie domyślna.
- [ ] *(nice-to-have, po Fazie 5)* Android: `WifiNetworkSpecifier` —
      jednoklikowe dołączenie do sieci bez wychodzenia z appki i bez
      aparatu. Niemożliwe do zrobienia na iOS (ograniczenie Apple, nie
      nasze) — QR zostaje jedyną wspólną ścieżką na obie platformy.

## Faza 8 — iOS (odroczone)

- [ ] Decyzja: Apple Developer Program (99$/rok), gdy budżet na to pozwoli.
- [ ] Do tego czasu: PWA + jasna instrukcja w appce "jeśli się nie łączy,
      otwórz `http://192.168.4.1` bezpośrednio w Safari".

## Poza głównym planem — do naprawienia niezależnie

- [ ] **CI: `Release firmware` failuje od v0.3.2.** Przyczyna (zweryfikowana
      w logach, nie zgadywana): `UnknownBoard: Unknown board ID
      'esp32-s3-r8-opi'` w joście `build-plugins` przy buildowaniu
      `focus-timer-plugin` — brakuje definicji custom board dla tego ID w
      PlatformIO. Nie ma to związku z BLE, mimo że tak wcześniej
      sugerowano w innej rozmowie.

## Otwarte pytania (nadal aktualne)

- Czy konwerter ma robić batch (wiele plików naraz)?
- Czy biblioteka na urządzeniu ma kategorie/tagi, czy płaska lista?
- Plugin "Klepsydra" — zostaje wbudowany w firmware, czy przechodzi
  wyłącznie na dystrybucję jako paczka pluginu?
- Nazwa "Pluginy" w UI — zostaje, czy zmieniamy na "Rozszerzenia"/"Dodatki"?

## Zasady pracy nad tym planem

- Praca lokalnie w `C:\Users\karol\czytnik01`, na branchu odgałęzionym od
  `origin/main` (nie bezpośrednio na `main`).
- Zmiany testowane lokalnie i na realnym telefonie przed pushem.
- Push/PR na GitHub tylko po wyraźnym potwierdzeniu — za każdym razem, nie
  jednorazowo.
