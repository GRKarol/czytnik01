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

## Faza 5 — Natywna appka Android (Capacitor) ✅ połączenie zweryfikowane na sprzęcie

> **Status 2026-07-23:** appka zbudowana, zainstalowana na fizycznym
> telefonie (Samsung SM-G986B) i **realnie połączona z czytnikiem po WiFi**
> — widać `Połączono · WiFi` w headerze i prawdziwe ustawienia z urządzenia
> (badge DEV pobrany z firmware). Branch `plan/native-app-v2` wypchnięty na
> GitHub, firmware wydane jako v0.3.7.

- [x] Dodać Capacitor do projektu (`@capacitor/core`, `@capacitor/android`,
      `@capacitor/cli`). Osobny build tylko appki klienta:
      `vite.capacitor.config.ts` → `dist-capacitor/` (bez flashera, bez
      vite-plugin-pwa — patrz `src/app/pwa-register-noop.ts`).
- [x] Konfiguracja Androida: `network_security_config.xml` scoped do
      `192.168.4.1` (bezpieczniejsze niż globalne `usesCleartextTraffic`).
- [x] **`capacitor.config.ts` → `server.androidScheme: "http"`** — znaleziony
      i naprawiony w tej sesji na żywym telefonie. Capacitor domyślnie
      serwuje appkę pod wirtualnym `https://localhost`, przez co WebView
      blokuje fetch do `http://192.168.4.1` jako "Mixed Content" —
      **dokładnie ten sam problem co w przeglądarce**, tylko wewnątrz
      natywnej appki. `network_security_config.xml` (cleartext dla
      Androida) i "Mixed Content" (polityka silnika Chromium) to dwa
      różne mechanizmy — trzeba naprawić oba. Zmiana schematu na `http`
      naprawia to w 100%, zweryfikowane logiem z urządzenia.
- [x] `@capacitor/share` + `@capacitor/filesystem` — przycisk "Pobierz
      .rsvp" w konwerterze na appce natywnej otwiera natywny arkusz
      Udostępnij zamiast linku do pobrania (funkcja #8 z wymagań).
- [x] Build debug APK (`gradlew assembleDebug`) — **sukces**,
      `android/app/build/outputs/apk/debug/app-debug.apk` (~5,5 MB).
- [x] **Test na realnym telefonie — sukces.** Zainstalowane przez
      `adb install` (USB), appka wykryła i połączyła się z czytnikiem w
      trybie Sync po WiFi. Pierwsze realne, potwierdzone działające
      połączenie od początku projektu.
- [x] `ConnectivityManager.bindProcessToNetwork` — natywny plugin Capacitora
      (`NetworkPinPlugin.java`) pina appkę na sztywno do aktywnej sieci
      WiFi przy udanym połączeniu i odpina przy każdym rozłączeniu/drop —
      zapobiega cichemu przełączeniu ruchu appki z powrotem na sieć z
      internetem, mimo że telefon nadal jest fizycznie w AP czytnika.
      Zweryfikowane: build (tsc/vite/gradle) + instalacja + rejestracja
      pluginu bez błędów na fizycznym telefonie (SM-G986B).
- [x] Natywny share target (odbieranie tekstu z innych aplikacji, jak w
      `rsvpnano`) — `ShareTargetPlugin.java` + intent-filter ACTION_SEND
      dla `text/plain`. Cold start (appka jeszcze nie działała) trafia do
      `getPending()`, warm start leci od razu jako event. Appka przełącza
      się na Konwerter i podaje tekst przez ten sam kod co ręczne "wklej
      tekst". Zweryfikowane end-to-end na fizycznym telefonie przez
      symulowany intent ACTION_SEND.
- [x] **Ręczna edycja rozdziałów w konwerterze** (`src/app/converter/chapters.ts`
      + `converter-panel.element.ts`) — czytnik pokazuje cokolwiek wylądowało
      w pliku `.rsvp`, a auto-wykrywanie bywa błędne (zwykły `.txt` — w tym
      wklejony/udostępniony tekst — w ogóle nie wykrywa rozdziałów). Nowy
      krok "Edytuj rozdziały" przed pobraniem/wysłaniem: zmiana tytułu,
      scalenie zbędnego podziału, podział w wybranym miejscu treści.
      Zweryfikowane end-to-end w przeglądarce (split/rename/merge
      poprawnie mutują `book.events` i widać to w statystykach/podglądzie).
- [x] Background task (WorkManager) do okresowego sprawdzania GitHub
      Releases co 12h, gdy appka nie jest otwarta i telefon ma internet
      (`UpdateCheckWorker.java`) — powiadomienie lokalne przy nowym tagu.
      Drugorzędny mechanizm: czytnik i tak sam się aktualizuje gdy ma
      zapisane WiFi domowe (`ota_auto` w App.cpp) — to tylko dla
      przypadku gdy ktoś tego nie skonfigurował.
- [x] **Podpisany (release, nie debug) APK — gotowe.** Keystore
      wygenerowany (`keytool`, 2048-bit RSA, ważność 10000 dni),
      `android/keystore.properties` (gitignored) wskazuje na plik +
      hasła, `build.gradle` odczytuje go i podpisuje `assembleRelease`.
      Zweryfikowane: `apksigner verify` potwierdza podpis, APK
      instaluje się i działa na fizycznym telefonie (nadpisał debug
      build — inny klucz, wymagało odinstalowania starej wersji).
      **Sam plik keystore leży poza repo (scratchpad sesji) — Karol
      przenosi go na stałe miejsce (backup!), inaczej zniknie i żadna
      przyszła aktualizacja nie nadpisze tej instalacji.**
- [x] **Publikacja gotowa.** `flower-android-v1.0.apk` wrzucony jako
      dodatkowy asset do release'u v0.3.7 na GitHubie, przycisk
      "Download for Android" na `flower.theworkpc.com/appdownload`
      podpięty pod prawdziwy link (zweryfikowane: 302 → poprawny plik
      APK, `content-type: application/vnd.android.package-archive`).
      Kod QR na tej stronie zostaje na razie jako "coming soon" —
      generowanie realnego QR bez zewnętrznej usługi/biblioteki to
      osobna, mniejsza rzecz do zrobienia kiedy indziej.
- [ ] Dłuższy test odporności: świadomie zerwać WiFi w trakcie sesji i
      sprawdzić czy auto-reconnect z Fazy 6 faktycznie łapie połączenie z
      powrotem (na razie zweryfikowano tylko sam pierwszy connect, nie
      zerwanie w trakcie używania).

## Faza 6 — Niezawodność połączenia (P0 — realny, potwierdzony bug) ✅ zweryfikowane na sprzęcie

> **Status 2026-07-21:** zaimplementowane, przechodzi `typecheck`/`build`,
> i **potwierdzone działające na prawdziwym czytniku** przez appkę Android.

- [x] **BUG naprawiony:** w `src/app/app.element.ts`, metoda `connect()` —
      wcześniej jeśli `pingDevice()` zawiódł choćby raz tuż po połączeniu
      WiFi, appka po cichu zostawała na `MockDeviceApi` zamiast pokazać
      błąd. To była zdiagnozowana przyczyna "starych książek, nic się nie
      zgrywa" z testów. Naprawa: `pingDeviceWithRetry()` (3 próby, 800ms
      odstępu) zanim appka odda błąd; jeśli finalnie się nie uda, appka się
      rozłącza i pokazuje jasny komunikat zamiast ciągnąć dalej na mocku.
- [x] **Drugi realny bug, znaleziony i naprawiony na sprzęcie:**
      `wifi-link.ts` wymagał otwartego WebSocketu (`/api/events`) jako
      warunku udanego połączenia — ale obecny firmware
      (`CompanionSyncManager.cpp`) **w ogóle nie implementuje WebSocketu**
      (zweryfikowane grepem, zero wystąpień). To znaczy, że connect()
      zawodził zawsze, na każdej platformie, niezależnie od Capacitora czy
      mixed content. Naprawa: WebSocket jest teraz opcjonalnym bonusem
      (próba + `console.warn` przy porażce), nie wymogiem — kierunek
      telefon→czytnik (zmiana ustawień) i tak działa zwykłym HTTP PATCH.
- [x] Keep-alive: `GET /api/hello` co 8s w `wifi-link.ts`, wykrycie
      rozłączenia przez timeout 3s na każdym zapytaniu.
- [x] Auto-reconnect po zerwaniu WebSocketu/keep-alive — backoff (1s, 2s,
      4s, 8s, 8s), potem appka zostaje rozłączona i czeka na ręczną próbę.
- [x] `DeviceLink.onStatusChange()` — appka pokazuje żółty banner
      "Połączenie przerwane — próbuję połączyć ponownie…" i czerwoną
      "pill" w headerze zamiast cichego zawieszenia w stanie "połączono".
- [x] Kolejkowanie `PUT /api/settings` w `http-api.ts` — dwa szybkie po
      sobie zapisy ustawień (np. z suwaka) nie wyścigują się już do ESP32.
- [x] **Pasek nawigacji zawsze widoczny.** `:host` miał `min-height: 100vh`,
      co pozwalało całej stronie rosnąć wyżej niż ekran i spychało pasek pod
      dół — trzeba było przewijać żeby go zobaczyć. Zmienione na sztywne
      `height: 100vh` + `overflow: hidden`, `main` przewija się wewnętrznie.
- [x] **Bug z językami — potwierdzony i naprawiony.** Dwie różne mapy tego
      samego indeksu języka firmware istniały równolegle: poprawna w
      `src/app/i18n/lang-map.ts` (`0=en,1=es,2=fr,3=de,4=ro,5=pl`) i
      **błędna** w `src/app/device/http-api.ts`
      (`["pl","en","de","es","fr","it"]` — z włoskim, którego firmware nie
      obsługuje). Appka wysyłała/czytała zupełnie inny język niż myślała.
      Naprawa: `http-api.ts` importuje teraz `UI_LANG_INDEX_MAP` zamiast
      trzymać własną kopię. Tłumaczenia w samym firmware sprawdzone —
      kompletne dla wszystkich 6 języków (123 klucze, zero braków).

## Faza 9 — Pełny parytet ustawień firmware ↔ appka

> **Status 2026-07-23:** zaimplementowane, wydane jako **v0.3.7** i
> **potwierdzone na fizycznym czytniku** — urządzenie samo zaciągnęło
> aktualizację przez auto-OTA po wydaniu release'u, ekran Informacje
> pokazuje `v0.3.7`.

Audyt: porównałem każdy klucz NVS w `firmware/src/app/App.cpp` (49 kluczy)
z tym co faktycznie wystawia `/api/settings` w `CompanionSyncManager.cpp`.
Znalezione i dopisane (firmware + `api.ts` + `http-api.ts` +
`settings-panel.element.ts` — wszystko już zaimplementowane):

- [x] `navMode` (Swipe/D-Pad) — to jest dokładnie "rodzaj sterowania",
      o które prosiłeś. Nowa sekcja `input` w JSON.
- [x] `focusColorIndex` (kolor podświetlenia ORP, 0-5) — dopisany do sekcji
      `typography`.
- [x] `savePointButtonVisible`, `showHelpHints` — dopisane do `display`.
- [x] Cała nowa sekcja `screensaver`: `mode`, `timeoutIndex`,
      `autoOffIndex`, `sleepGuardIndex`.
- [x] Nowa sekcja `connectivity`: `bleEnabled`, `otaAutoCheck` — dodane w
      appce jako pozycje w sekcji Developer (zgodnie z komentarzem, który
      już to zapowiadał w kodzie, ale nigdy nie było zaimplementowane).
- [x] **Naprawiony martwy kod:** `accurateTimeEstimate` było zawsze
      zwracane jako `true` i zapis zawsze wymuszał `true` niezależnie od
      tego co appka wysłała — w `App.cpp` `accurateTimeEstimateEnabled_`
      było zahardkodowane, NVS w ogóle nie było czytane. Naprawione w obu
      miejscach naraz.
- [x] Wgrane na czytnik przez auto-OTA po wydaniu v0.3.7, potwierdzone na
      ekranie Informacje.
- [x] Wbudowana strona web companion (`kWebCompanionHtml` w
      `CompanionSyncManager.cpp`) doprowadzona do parytetu — dodane
      `navMode`, `focusColorIndex`, `savePointButton`, `showHelpHints`,
      `accurateTimeEstimate`, cała sekcja `screensaver` i `connectivity`.
- [x] **Znaleziony i naprawiony realny, żywy bug przy okazji:**
      `applySettingsJson()` miał niewarunkowe
      `preferences_.putBool(kPrefAccurateTime, true)` uruchamiane przy
      **każdym** PATCH `/api/settings` — nie tylko takim, który dotyczył
      tego pola. Appka i web companion wysyłają patch pojedynczego pola
      naraz (jeden suwak/toggle = jeden PATCH), więc każda niezwiązana
      zmiana ustawień (WPM, jasność, dowolny toggle) cicho resetowała
      `accurateTimeEstimate` z powrotem na `true`. Prawidłowy, warunkowy
      zapis już istniał niżej w kodzie — ta linijka była zapomnianą
      pozostałością z czasu przed jego dodaniem i wygrywała bo działała
      pierwsza. Dotyczyło to również appki, nie tylko web companion.

## Faza 7 — Onboarding przez QR (⚠️ plan skorygowany po realnym użyciu)

> **Korekta 2026-07-21, potwierdzone 2026-07-23:** "QR jako główna ścieżka"
> (plan poniżej z Fazy 3) okazał się błędnym założeniem po realnym teście —
> QR "nic nie tłumaczy" i nie jest tym czego Karol chce jako domyślne.
> Zamiast tego: **ekran Sync na czytniku pokazuje domyślnie tekst** (nazwa
> sieci + adres), **QR jest dostępny jako opcja po tapnięciu ekranu**
> (przesunięcie w bok nadal wychodzi z trybu Sync, tak jak wszędzie indziej
> w menu). Zaimplementowane w `App.cpp`/`App.h` (`companionSyncShowQr_` +
> rozróżnienie tap/swipe), wydane w v0.3.7, żyje na urządzeniu.

- [x] Firmware generuje QR (Faza 3).
- [x] **Skorygowane:** domyślny widok ekranu Sync to tekst (SSID + URL),
      tap przełącza na QR, swipe wychodzi — zamiast "QR jako główne".
- [ ] ~~Android: `WifiNetworkSpecifier` jednoklikowe dołączenie~~ — nie
      priorytet, skoro tekst i tak jest teraz domyślny i czytelny.

## Faza 8 — iOS (odroczone)

- [ ] Decyzja: Apple Developer Program (99$/rok), gdy budżet na to pozwoli.
- [ ] Do tego czasu: PWA + jasna instrukcja w appce "jeśli się nie łączy,
      otwórz `http://192.168.4.1` bezpośrednio w Safari".

## Poza głównym planem — do naprawienia niezależnie

- [x] **v0.3.7 wydane** — pierwszy release z branchu `plan/native-app-v2`
      (WiFi reliability, parytet ustawień, poprawki ekranu Sync). Główne
      binarki (`czytnik01.bin`, `flower-firmware.bin`) budują się i
      publikują poprawnie.
- [x] **CI: `UnknownBoard` w joście `build-plugins` naprawione.** Przyczyna:
      `firmware/boards/esp32-s3-r8-opi.json` jest widoczny tylko gdy `pio
      run` odpala się z katalogu `firmware/` (auto-discovery). Każdy plugin
      buduje się jako osobny, izolowany projekt PlatformIO (`cd
      plugins/<nazwa> && pio run`), więc nigdy nie widział tego pliku.
      Naprawa: `boards_dir = ../../firmware/boards` w sekcji `[platformio]`
      obu `plugins/*/platformio.ini`.
- [x] **`Nothing to build` naprawione — build pluginów kompiluje się i
      linkuje po raz pierwszy w całej historii projektu.** Trzy osobne
      przyczyny, każda zasłaniała kolejną:
      1. `build_src_filter = +<src/>` szukał folderu `src/src/` (filtr
         jest już liczony względem `src_dir`, domyślnie `src`) — zmiana
         na `+<*>`.
      2. `lib_extra_dirs` wskazywał wprost na `firmware/src/plugins/sdk`,
         ale ten folder to luźne nagłówki bez `library.json` — PlatformIO
         rozpoznaje jako biblioteki tylko PODFOLDERY wskazanego katalogu,
         więc nigdy nie trafił na ścieżkę include. Zastąpione zwykłą flagą
         `-I` (+ druga flaga `-I..` dla `plugin_new.h`, który leży wprost
         w `plugins/`).
      3. `plugin_runtime.cpp` (współdzielone zaślepki linkera dla buildów
         `-nostdlib`) leży w `plugins/`, poziom wyżej niż `src/` każdego
         pluginu — filtr nie sięga tam wzorcem `../`. Dodany
         `tools/pio_plugin_shared_runtime.py` (pre-script, `env.BuildSources()`)
         żeby dociągnąć ten plik jawnie. Przy okazji: usunięta duplikująca
         się definicja `__dso_handle` (toolchain i tak ją dostarcza) i
         dodane puste `setup()`/`loop()` — framework Arduino zawsze ich
         wymaga przy linkowaniu, mimo że plugin nigdy nie odpala się jako
         normalny szkic.
      Zweryfikowane: **`pio run` dla obu pluginów kończy się `SUCCESS`**
      (wcześniej failowało od 2026-06-10, zawsze — najpierw zasłonięte
      przez `UnknownBoard`, potem przez to).
- [ ] **Nowo znaleziony, głębszy problem — binarka pluginu prawdopodobnie
      nie nadaje się jeszcze do wgrania.** Skrypt patchujący nagłówek
      (`tools/pio_plugin_build.py`) zgłasza `Invalid magic` — `esptool.py`
      zawsze doklej swój własny nagłówek obrazu ESP32 (magic `0xE9`) na
      początku `.bin`, więc własny nagłówek pluginu (`PluginBinaryHeader`,
      magic `"PLUG"`) nie ląduje już na bajcie 0, tam gdzie loader go
      oczekuje. To osobny problem od "build failuje" (surowa binarka vs.
      format obrazu ESP32) — CI będzie już zielone, ale sam plugin może
      nadal nie działać po wgraniu na czytnik. Nie badane głębiej w tej
      sesji, zgodnie z wcześniejszą decyzją że system pluginów to niski
      priorytet.

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
- **Appka i firmware rozwijają się razem.** Każdy nowy release firmware
  (nowe ustawienie, nowa funkcja) powinien pociągać za sobą audyt: czy
  appka to wystawia? Faza 9 (parytet ustawień) to pierwszy taki przegląd —
  kolejne powinny się dziać cyklicznie, nie tylko raz.
