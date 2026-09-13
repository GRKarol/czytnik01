# Plan refaktoru: Settings + i18n jako dane

Kontekst: rsvpnano (ionutdecebal) ma dwie rzeczy, których u nas brakuje i będzie
brakować coraz bardziej — generyczny, walidowany layer ustawień (u nich przez
`glaze`+TOML+`BoundedValue`) i tłumaczenia jako plik danych, nie kod. Nie
kopiujemy ich zależności (inny standard C++, inne biblioteki, inny framework
UI) — robimy to samo prościej, pod nasz stack.

Stan obecny (sprawdzony w kodzie, nie z pamięci):

- Ustawienia: każdy plugin sam czyta/pisze surowe bajty przez
  `PluginStorageService` (`firmware/src/plugins/sdk/PluginStorageService.h` —
  tylko `readFile`/`writeFile`/`fileExists`/`mkdir`, żadnej walidacji zakresów,
  żadnego wspólnego formatu). Nowy `FocusTimerPlugin` i `DictaphonePlugin`
  robią to każdy inaczej.
- Tłumaczenia: trzy równoległe systemy.
  1. `polish(pl, en)` w `App.cpp` — ~105 miejsc, tylko PL/EN, reszta języków
     dostaje angielski.
  2. `TrKey`/`TrKey2` w `firmware/src/app/Translations.h` — switch-case na
     6 języków, ale każda nowa/zmieniona linijka to zmiana w C++ i rebuild.
  3. `DictStr`/`dictText()` w `DictaphonePlugin.cpp` — własna, osobna kopia
     tego samego wzorca, bo plugin nie może włączyć `app/Localization.h`
     (musi być odseparowany od kodu appki).

Cel refaktoru: jeden generyczny mechanizm ustawień z walidacją zakresów i
jedna tabela tłumaczeń wczytywana z danych, bez dociągania `glaze`,
`std::expected` czy innego cudzego frameworka UI.

---

## Zasady pracy nad tym planem

- Osobna gałąź/branch od `main` — refaktor nie idzie na tej samej gałęzi co
  bieżące zmiany w App.cpp/DisplayManager.cpp/main.cpp/StorageManager.cpp/h,
  żeby dało się je testować i wypuszczać niezależnie.
- Każdy etap kompiluje się i wychodzi jako osobny tag na `staging`, testowany
  fizycznie przed przejściem do następnego. Nie łączyć etapów 1 i 2 w jednym
  commicie.
- Zero nowych zależności zewnętrznych (bez `glaze`, bez TOML/JSON bibliotek).
  Format plików własny, prosty tekstowy `key=value` albo mały binarny
  generowany build-time — cokolwiek da się sparsować bez wyjątków i bez STL
  ponad to, co już jest w projekcie.

---

## Etap 1 — SettingsStore (wspólna warstwa ustawień z walidacją)

Problem: nowy plugin (np. FocusTimer) musi sam wymyślić format zapisu na SD,
sam pilnować że wartość z presetu nie wyjdzie poza sensowny zakres. Więcej
pluginów = więcej powielonego, niewalidowanego kodu.

Co robimy:

1. Nowy plik `firmware/src/plugins/sdk/SettingsStore.h/.cpp` — generyczne API:
   - `bool loadInt(const char* key, int32_t& out, int32_t min, int32_t max, int32_t def)`
   - `bool saveInt(const char* key, int32_t value)`
   - analogicznie dla bool/string (krótki bufor, bez `std::string`).
   - Zakres (`min`/`max`) pilnowany w jednym miejscu — poza zakresem, wraca
     `def` i loguje ostrzeżenie, nigdy nie zapisuje śmieci na SD.
2. Format pliku: `/plugins/{id}/settings.cfg`, czysty tekst `klucz=wartosc`
   po linii — trywialne do parsowania ręcznie (bez biblioteki), trywialne do
   podglądu przez USB mass storage jeśli coś nie działa.
3. Rozszerzyć `PluginStorageService` o wskaźniki na te dwie funkcje (tak jak
   już jest `readFile`/`writeFile`), żeby pluginy dostawały je przez tę samą
   strukturę co teraz.
4. Pierwszy użytkownik: `FocusTimerPlugin` — on i tak zapisuje wybrany
   preset na SD, więc migracja jest małym, izolowanym testem nowego API bez
   ryzyka dla działających funkcji.
5. Drugi użytkownik, po potwierdzeniu że etap 1 działa na sprzęcie:
   `DictaphonePlugin` (ustawienia mikrofonu/poziomów, jeśli takie ma) i
   ekran typografii (Tracking/Anchor/Width/Gap ze zdjęcia referencyjnego) —
   to jest dokładnie przypadek "4 liczby z zakresem", idealny do walidacji
   przez `BoundedValue`-like helper.

## Etap 2 — Tłumaczenia jako dane, nie switch w C++

Problem: zmiana jednego stringa w jednym języku = grzebanie w C++ + pełny
rebuild + reflash. Do tego trzy niezależne, niekomunikujące się ze sobą
systemy tłumaczeń (patrz "Stan obecny" wyżej).

Co robimy:

1. Jeden plik źródłowy `tools/translations.csv` w repo — kolumny:
   `key;pl;en;es;fr;de;ro`. To zastępuje treść `TrKey`, `TrKey2` i `DictStr`
   w jednym miejscu, edytowalnym bez znajomości C++.
2. Skrypt `tools/gen_translations.py` (build-time, nie runtime) — czyta CSV,
   generuje `firmware/src/app/generated/TranslationsData.h` jako statyczną
   tablicę `const char* const kTranslations[][6]` plus enum kluczy
   generowany automatycznie z pierwszej kolumny. To eliminuje ręczne
   przepisywanie switchy, ale nie dodaje żadnej biblioteki do firmware —
   wynik i tak jest zwykłym C++ tablicowym, kompilowanym normalnie.
3. `tr()`/`tr2()` w `Translations.h` zmieniają ciało: zamiast switcha,
   indeksują `kTranslations[key][langIndex]`. Sygnatura funkcji (interfejs
   używany w ~105 miejscach `App.cpp`) się nie zmienia — zero zmian w
   miejscach wywołania.
4. `polish(pl, en)` w `App.cpp` — po tym etapie każde jego wywołanie trzeba
   ręcznie zamienić na `tr()` z nowym kluczem w CSV (to jest ta duża,
   znana już robota 105 miejsc — tu tylko zmienia się docelowy mechanizm,
   z switcha na tabelę).
5. `DictStr`/`dictText()` w `DictaphonePlugin.cpp` — po potwierdzeniu że
   `PluginDisplayService` już eksponuje `languageIndex()` (jest, sprawdzone),
   dodać do SDK pluginów wskaźnik na funkcję `pluginTr(key, lang)` czytającą
   z tej samej wygenerowanej tabeli, i podmienić `dictText()` na wywołanie
   tego wskaźnika. Usuwa drugą, osobną kopię 13 stringów.
6. Later/opcjonalnie: przenieść samą tabelę z Flash (PROGMEM) na SD, żeby
   dawało się poprawić tłumaczenie bez reflashowania — to jest już czysty
   bonus nad tym co ma rsvpnano (u nich trzeba edytować plik na karcie i
   restart, u nas identycznie), ale nie jest wymagane do samego etapu 2.

---

## Czego NIE robimy

- Żadnego `glaze`/TOML — nasz format `key=value` i CSV robi to samo dla
  naszej skali (kilka pluginów, nie edytowalna-przez-usera biblioteka
  configów).
- Żadnego `std::expected`/C++23 — projekt stoi na starszym standardzie,
  błędy wracają przez `bool`+`out`-parametr, jak reszta kodu.
- Żadnej apki towarzyszącej, żadnego wsparcia dwóch płytek jednym kodem —
  to nie jest to, czego nam brakuje.

---

## Kolejność w praktyce

1. Etap 1 (SettingsStore) na FocusTimerPlugin — osobny tag na staging, test
   fizyczny.
2. Etap 1 rozszerzony na ekran typografii (Tracking/Anchor/Width/Gap).
3. Etap 2 (tabela tłumaczeń) — najpierw sam mechanizm + `TrKey`/`TrKey2`,
   osobny tag, test fizyczny.
4. Etap 2 — przepisanie 105 miejsc `polish()` na `tr()` z nowymi kluczami.
5. Etap 2 — `DictaphonePlugin` przechodzi na wspólną tabelę, `DictStr`
   usunięty.
