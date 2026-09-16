# Plan: fonty książki wczytywane z karty SD zamiast z flasha

Status: Etap 1-6 zrobione (patrz niżej). Plan zamknięty — zostaje tylko
fizyczna weryfikacja na sprzęcie (patrz sekcja Etap 6).
Branch roboczy: `main` na staging (ten sam co dotychczasowy refaktor typografii).

## Zrobione — Etap 6: redesign ekranu wyboru fontu (miniaturki nazw)

Wybrana opcja A z sekcji "Kluczowa decyzja" niżej: pre-renderowane
miniaturki nazw fontów, trzymane w flashu.

**`tools/generate_embedded_font.py`**: nowy tryb `--thumbnail-output` (+
`--thumbnail-target-height`, domyślnie 24px) — generuje ten sam nagłówek co
`--output`, ale ograniczony do zakresu drukowalnego ASCII (32-126, 95
znaków) zamiast pełnego zakresu 1-255, i przy dużo mniejszym punkcie
(przeskalowanym proporcjonalnie od już skalibrowanego `point_size` głównego
wyjścia). Nazwy fontów w UI to zawsze zwykły ASCII (`typefaceDisplayName()`
w `App.cpp`), więc ograniczenie do drukowalnego ASCII nie traci żadnego
znaku, a redukuje rozmiar per font z ~320KB (pełny zakres, wysokość 52px)
do ~23-28KB (ASCII-only, wysokość ~26-30px) — zmierzone, nie szacowane.

**`tools/generate_font_thumbnails.sh`** (nowy, wzorowany na
`generate_font_pack.sh`): pobiera tych samych 17 fontów Google Fonts (OFL) i
generuje `firmware/src/display/thumbnails/Embedded<Nazwa>Thumbnail.h` dla
każdego. W przeciwieństwie do `generate_font_pack.sh` (którego wyjście na SD
nigdy nie trafia do repo), wyjście tego skryptu **jest commitowane** — to
dane flashowe, ta sama zasada co 3 wbudowane fonty (Atkinson/Serif/
OpenDyslexic). Odtworzyć i zacommitować diff przy każdej zmianie katalogu
fontów SD.

**`DisplayManager.cpp`**: 17 nowych `EmbeddedFontVariant` (po jednym na font
SD, zbudowane z nagłówków miniaturek) + funkcja `thumbnailFontVariant()`
mapująca `ReaderTypeface -> const EmbeddedFontVariant *`. Jedna zmiana w
`extraFontVariant()`: gdy pytany o typeface **inny** niż aktualnie wczytany
z SD (`gSdFontLoadedTypeface`) — czyli dokładnie przypadek podglądu na
przycisku, bo normalne czytanie zawsze pyta o aktywny font — zwraca
miniaturkę z flasha zamiast Atkinsona. Żadna funkcja rysująca/mierząca
tekst nie wymagała zmian: `glyphFor`, `drawSerifTextScaledAt`,
`baseGlyphHeightForTypeface` już przyjmowały `EmbeddedFontVariant` przez tę
jedną funkcję, więc podmiana źródła danych wystarczyła. Aktywny font wciąż
renderuje się z prawdziwego pliku SD (bez zmian w tej ścieżce) — miniaturka
dotyczy wyłącznie pozostałych 16 przycisków.

**Zweryfikowane realnie**: build czysty na obu środowiskach. Flash
`waveshare_esp32s3` 42.1% -> **48.9%**, `waveshare_esp32s3_usb_msc` 42.6% ->
**49.4%** — wzrost ~413-424KB odpowiada zmierzonej sumie 17 miniaturek
(426 944 B surowych danych PROGMEM). Duży margines do pełnej partycji
(6.25 MB) zostaje.

**Świadomie nietestowane fizycznie**: wygląd miniaturek na prawdziwym
ekranie (czy 26% skalowanie z ~26-30px źródła daje czytelny, nierozmazany
tekst), czas otwarcia ekranu wyboru fontu przy 20 pozycjach, i że aktywny
font nadal renderuje się z SD (nie miniaturki) po wybraniu. Wymaga
fizycznego testu na sprzęcie z flashem `v0.3.40`+ (kolejny savepoint).

## Zrobione — Etap 5 dokończony: automatyczne pobieranie w tle

Karol poprosił, żeby fonty pobierały się same — bez ręcznego rozpakowywania
zipa na kartę — w momencie kiedy czytnik ma zapisane Wi-Fi (np. po sparowaniu
z apką Flower), bez żadnego działania użytkownika, i żeby do tego czasu
biblioteka fontów pokazywała tylko to co realnie jest na karcie.

**Nowe API `DisplayManager`** (`isSdBackedTypeface()`, `sdFontFileBaseName()`,
`isTypefaceAvailableOnSd()`) — publiczne, statyczne wrappery na dotychczasowe
funkcje w anonimowej przestrzeni nazw `DisplayManager.cpp`, żeby `App.cpp`
mógł pytać "czy font X jest na karcie" bez duplikowania tabeli nazw plików.

**`OtaUpdater::downloadAsset()`** — nowa publiczna metoda obok
`installAsset()`: pobiera nazwany asset z Release'u (ten sam mechanizm
resolve-URL/redirect co OTA), ale zamiast flashować przez `HTTPUpdate`,
strumieniuje na kartę SD (plik `.part` + atomowy `rename`, żeby przerwane
pobieranie nie zostawiło uszkodzonego `.fnt`). `connectWiFi()`/`disconnectWiFi()`
są teraz publiczne, żeby wywołujący mógł pobrać wiele plików w jednej sesji
Wi-Fi zamiast łączyć się od nowa za każdym razem.

**`App.cpp`** — nowy stan analogiczny do istniejącego auto-update OTA
(`maybeAutoCheckForUpdates`/`startBackgroundOtaCheck`/`otaCheckTask`/
`pollOtaCheckResult`), ta sama para kolejka+task:
- `maybeAutoDownloadFonts(nowMs)` — co 60 s (`kFontDownloadRetryIntervalMs`)
  sprawdza `otaUpdater_.isConfigured()` (czy jest zapisane SSID); jeśli nie,
  cicho czeka na kolejną próbę. Jeśli tak, odpala `startBackgroundFontDownload()`.
  Wywoływane przy boocie, co klatkę w `update()`, i po udanym
  `runSdCardRepair()` (świeżo sformatowana/naprawiona karta też odpala
  sprawdzenie od razu, nie czeka do restartu).
- `fontDownloadTask()` (FreeRTOS task, core 0) — liczy które z 17 fontów SD
  faktycznie brakują (`isTypefaceAvailableOnSd()`), łączy Wi-Fi raz, pobiera
  tylko brakujące (`<nazwa>.fnt` + `<nazwa>_70.fnt` jako osobne assety
  Release'u o nazwie 1:1 z plikiem na SD), rozłącza Wi-Fi, wrzuca wynik do
  kolejki. Częściowa/nieudana partia po prostu zostaje do następnej próby za
  60 s — brakuje-only re-scan sprawia, że retry nie pobiera ponownie tego, co
  się już udało.
- `openTypographyFontPicker()` filtruje listę do fontów faktycznie obecnych
  na karcie (3 wbudowane zawsze, 17 SD-owych tylko gdy pobrane) — stąd nowy
  wektor `typographyFontPickerTypefaceForIndex_` (indeks wyświetlany ≠ numer
  `ReaderTypeface`, bo lista jest filtrowana).

**CI**: nowy `.github/workflows/build-fonts.yml`, wołany z `release.yml`
analogicznie do `build-plugins.yml` — uruchamia
`tools/generate_font_pack.sh` i wgrywa 34 pliki `.fnt` jako osobne, nazwane
assety Release'u (nie zip), bo `downloadAsset()` dopasowuje po dokładnej
nazwie pliku z JSON-a Release'u, a na urządzeniu nie ma biblioteki do
rozpakowywania zipów (sprawdzone: `platformio.ini` nie ma żadnej zależności
zip/unzip). Ważne: to zadziała dopiero dla **kolejnego** taga wypchniętego po
tej zmianie — istniejące release'y (np. `v0.3.38` z ręcznie doklejonym
`fonts-pack-v0.3.38.zip`) nie mają tych pojedynczych assetów.

**Świadomie nietestowane fizycznie**: cała ścieżka (Wi-Fi się łączy →
pobieranie w tle → fonty pojawiają się w bibliotece bez restartu) wymaga
prawdziwego releasu z assetami z nowego workflow i prawdziwego czytnika ze
skonfigurowanym Wi-Fi.

## Zrobione — Etap 4 i 5

Katalog rozszerzony z 7 do 17 fontów SD (20 krojów łącznie z 3 fontami w
flashu: Standard/OpenDyslexic/Atkinson) — dokładnie w widełkach planu
(~10-13 nowych, dodano 10). Nowe kroje, wszystkie sprawdzone pod kątem
pokrycia polskich/środkowoeuropejskich znaków przez `fontTools` cmap
(`CUSTOM_GLYPH_CODEPOINTS` z `generate_embedded_font.py` + 9 polskich liter
ą/ć/ę/ł/ń/ó/ś/ź/ż) przed wyborem — wszystkie 10 mają 95-100% pokrycia:

- PT Serif (`ptserif`), IBM Plex Serif (`ibmplexserif`), Cardo (`cardo`),
  Zilla Slab (`zillaslab`), Old Standard TT (`oldstandard`), Domine
  (`domine`), Alegreya (`alegreya`), Newsreader (`newsreader`), Noto Serif
  (`notoserif`), Spectral (`spectral`) — wszystkie Google Fonts, licencja
  OFL, ten sam mechanizm co istniejące 7 (`isExtraTypeface()`,
  `sdFontBaseName()`, brak dedykowanego kodu per-font).
- Odrzucone po sprawdzeniu: Neuton (tylko 15% pokrycia CE — pomija większość
  polskich znaków), Cormorant/Cormorant Garamond/Libre Baskerville/Crimson
  Pro (100% pokrycia, ale odrzucone dla różnorodności stylistycznej — zestaw
  i tak już ma kilka fontów o podobnym, wąskim/eleganckim rysunku).

**Zmiany w kodzie** (mechaniczne, bez nowej logiki — architektura z Etapu 2
już była generyczna po `ReaderTypeface::Count`):
- `DisplayManager.h`: enum `ReaderTypeface` rozszerzony o 10 wpisów
  (`PtSerif`..`Spectral`), `Count` 10 -> 20.
- `DisplayManager.cpp`: `sdFontBaseName()` o 10 nowych `case`.
- `App.cpp`: `typefaceDisplayName()` o 10 nowych `case` (etykiety do UI).
- `CompanionSyncManager.cpp`: `kMaxReaderTypeface` 9 -> 19 — bez tej zmiany
  aplikacja mobilna przycinałaby zsynchronizowaną wartość kroju z powrotem
  do starego zakresu 0-9, przez co nowe fonty nigdy nie zostałyby wybrane
  przez sync. Jedyne miejsce w kodzie poza samym enumem, które twardo
  zakładało `Count == 10`.
- Ekran wyboru fontu (`App::openTypographyFontPicker()`) i limit indeksu w
  `selectTypographyFontPickerItem()`/`annotateTypographyFontPickerButton()`
  już iterowały `0..Count-1` dynamicznie — zero zmian potrzebnych.

**Narzędzie generujące** (`tools/generate_embedded_font.py`): `--output`
(nagłówek `.h` do flasha) jest teraz opcjonalny — nowe kroje nie potrzebują
wersji flashowej, więc generacja leci tylko z `--fnt-output`. Dodano
`tools/generate_font_pack.sh <output_dir>` — reprodukowalny skrypt, który
pobiera wszystkie 17 fontów SD ze `github.com/google/fonts` (OFL, nie
commitowane do repo — ten sam brak commitowania źródeł co przy oryginalnych
7 fontach) i generuje 34 pliki `.fnt` (17 fontów x 2 rozmiary) przy
`--target-height 52` (podstawowy) / `39` ("70") — wartości dobrane tak, żeby
wysokość glifów wyszła zbliżona do już działających 7 fontów (52-54px /
39-41px w oryginalnych nagłówkach).

**Build zweryfikowany na obu środowiskach** (`waveshare_esp32s3`,
`waveshare_esp32s3_usb_msc`) — flash **42.1-42.6%, bez zmian** względem
stanu po Etapie 3 (oczekiwane: nowe fonty to tylko `case` w switchu, żadnych
nowych danych PROGMEM).

**Dystrybucja**: `fonts-pack.zip` (34 pliki `.fnt`, ~8.2 MB rozpakowane,
~1.2 MB spakowane) dołączony jako dodatkowy asset do release'u na
`staging` — do rozpakowania na karcie SD w `/fonts/`.

**Świadomie nietestowane fizycznie**: jak w Etapie 2/3, realne wczytanie i
wygląd 17 fontów wymaga fizycznego testu na sprzęcie z plikami z paczki na
karcie. Podgląd nazw fontów na przyciskach ekranu wyboru był w tym momencie
planu jeszcze ograniczony do aktualnie aktywnego fontu — rozwiązane później
w Etapie 6 (patrz sekcja wyżej) przez flashowe miniaturki ASCII.

## Zrobione — Etap 2 i 3 (odstępstwo od pierwotnego opisu Etapu 3)

`SdFontLoader` (`firmware/src/display/SdFontLoader.h/.cpp`) czyta jeden plik
`.fnt` do bufora PSRAM i wskazuje w niego `EmbeddedFontVariant` — bez kopiowania
pole po polu (`static_assert(sizeof(EmbeddedFontGlyph) == 8)` pilnuje, że
layout pliku faktycznie odpowiada strukturze w pamięci). `DisplayManager.cpp`
trzyma po jednej instancji loadera na rozmiar (`gSdFontLoader`/`gSdFontLoader70`),
wczytuje przy zmianie kroju w `setTypographyConfig()` (`ensureExtraTypefaceLoaded()`),
i pyta o plik `/fonts/<nazwa>_70.fnt` dla wariantu "70". **Konwencja nazw
plików** (nigdzie wcześniej nie spisana wprost): `sdFontBaseName()` w
`DisplayManager.cpp` — `literata`, `merriweather`, `lora`, `bitter`,
`ebgaramond`, `vollkorn`, `gelasio` (lowercase, bez spacji), pliki
`/fonts/<nazwa>.fnt` (podstawowy) i `/fonts/<nazwa>_70.fnt` (wariant "70").
Brak pliku/uszkodzony plik -> fallback na Atkinson (który zawsze jest w
flashu) + widoczny komunikat `renderStatus("Font", "Not found on SD", "Using
Atkinson")` w `App::selectTypographyFontPickerItem()` (jedyne miejsce, gdzie
to leci — inne wywołania `applyTypographySettings()` zdarzają się też przed
`display_.begin()` na starcie, gdzie renderStatus nie ma sensu).

**Odstępstwo od pierwotnego opisu Etapu 3**: tekst niżej mówił o usunięciu 9 z
10 fontów z flasha, czyli też Serif (=Standard) i OpenDyslexic — to by
zepsuło domyślny krój czytnika, bo ich kod (`serifGlyphForByte`,
`glyphFor`/`glyph70For` switch-case) nigdy nie został objęty loaderem SD, ani
w tym planie, ani w Etapie 2. Zostały w flashu, nietknięte. Usunięto tylko
dane 7 "extra" krojów (Literata..Gelasio) — **i to się okazało w praktyce
zbędne jako osobny krok**: samo przełączenie `extraFontVariant()`/`70()` na
loader w Etapie 2 już uczyniło stare tablice `kExtraFontVariants[]`
nieużywane, więc linker (`--gc-sections`) wyrzucił ich dane z binarki przed
jakimkolwiek ręcznym usuwaniem nagłówków — flash spadł z 83.4% na **42.6%**
(zmierzone, `waveshare_esp32s3_usb_msc`) zanim usunąłem choć jeden plik.
Usunięcie 14 nagłówków (`EmbeddedLiterataFont.h` itd.) było więc czystym
porządkiem w repo (mniej plików, szybszy build), nie warunkiem odzyskania
miejsca we flashu. Zbudowane i zweryfikowane na `waveshare_esp32s3` i
`waveshare_esp32s3_usb_msc` — flash 42.1-42.6%, brak błędów kompilacji.

**Świadomie nietestowane fizycznie w tej sesji**: bez plików `.fnt` na karcie
SD (Etap 4/5 jeszcze nie zrobione) wybranie Literata/Merriweather/Lora/
Bitter/EB Garamond/Vollkorn/Gelasio pokaże fallback Atkinson + komunikat —
to jest oczekiwane, nie błąd. Realny test wczytywania z SD wymaga fizycznego
pliku `/fonts/literata.fnt` (itd.) na karcie.

## Punkt wyjścia — zmierzone fakty, nie szacunki

Zbudowałem firmware na `main` (10 krojów: Standard/Atkinson/Serif/OpenDyslexic +
7 nowych: Literata/Merriweather/Lora/Bitter/EB Garamond/Vollkorn/Gelasio) i
sprawdziłem realny rozmiar:

- Partycja aplikacji (flash): **6 553 600 B (6.25 MB)**, zajęte **5 467 937 B
  (83.4%)**, wolne **~1.03 MB**.
- Dodanie 7 fontów (po 2 rozmiary każdy: normalny + "70") podniosło zajętość
  z ok. 2.79 MB do 5.47 MB, czyli **~383 KB na font (obie odmiany rozmiaru
  razem)**.
- Przy tej samej średniej, 20 fontów w tym samym formacie co dziś to
  **~7.7 MB** — nie mieści się w 6.25 MB partycji. To jest twardy powód,
  dla którego trzeba zejść z flasha, nie kaprys.
- Karta SD 4 GB: 7.7 MB fontów to **0.19% pojemności**. Nawet 100 fontów w
  tym formacie (~38 MB) to wciąż <1%. Biblioteka epubów (typowo 1–5 MB/książka)
  i pliki pluginów (rząd KB–niskie MB) nie rywalizują realnie o miejsce z
  fontami — margines jest ogromny. **Odpowiedź na pytanie „czy się zmieści":
  tak, bez kompresji, z dużym zapasem.**
- Chip: `esp32-s3-r8-opi` = **8 MB PSRAM** (osobne od flasha, prawie nieużywane
  dziś — 89 664/327 680 B to tylko wewnętrzny SRAM). PSRAM jest już używany
  gdzie indziej w kodzie przez `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`
  (`EpubConverter.cpp:112`, `DeviceServicesBridge.cpp:743`) — wzorzec do
  skopiowania, nie wymyślania od nowa.

## Dlaczego to się da zrobić bez przepisywania renderera

Format glifów to własne antyaliasowane rastry (1 bajt/piksel = alpha), nie
u8g2. Struktura (`EmbeddedFontCommon.h`):

```cpp
struct EmbeddedFontGlyph { uint32_t bitmapOffset; int8_t xOffset; uint8_t width; uint8_t xAdvance; };
struct EmbeddedFontVariant { const uint8_t *bitmaps; const EmbeddedFontGlyph *glyphs; uint8_t firstChar; uint8_t lastChar; uint8_t height; };
```

`DisplayManager.cpp` czyta to przez zwykłe wskaźniki (`glyph.bitmap[i]`,
`DisplayManager.cpp:1487`) — na ESP32-S3 flash jest mapowany w pamięci, więc
te same wskaźniki działają identycznie, czy dane leżą w PROGMEM, czy w buforze
PSRAM wypełnionym z pliku na SD. Nie trzeba przepisywać `drawGlyph`,
`drawSerifGlyphScaled` ani żadnej z funkcji rysujących — tylko to, **skąd**
`EmbeddedFontVariant` bierze swoje wskaźniki.

Katalogi na SD już istnieją i mają ten sam wzorzec (`StorageManager.cpp`,
`PluginLoader.cpp:279`): `/books`, `/plugins/<id>/`, `/config`. Dodanie
`/fonts/` to naturalne rozszerzenie, nie nowy pomysł architektoniczny.

## Kluczowa decyzja do podjęcia w nowym czacie: ekran wyboru fontu

Ekran font-pickera (v0.3.35, `App.cpp` ok. linii 5250, `previewTypeface`)
rysuje każdy przycisk nazwą fontu **w tym właśnie foncie** — dziś tanie, bo
wszystko jest w flashu. Po przeniesieniu na SD, samo otwarcie tego ekranu
przy 20 fontach oznaczałoby wczytanie 20 plików z karty tylko po to, żeby
narysować etykietki przycisków. Dwie opcje, wybór zostaje do nowego czatu:

- **A. Pre-renderowane miniaturki nazw** — w tool-chainie eksportu fontu
  generujemy dodatkowo mały bitmap podglądu (sama nazwa fontu, ten sam raster
  co reszta) i pakujemy te miniaturki osobno (mogą zostać w flashu, są małe —
  rząd kilku KB łącznie na 20 fontów). Ekran wyboru nigdy nie dotyka pełnych
  plików fontów z SD. Prostsze, przewidywalny czas otwarcia ekranu.
- **B. Leniwe wczytywanie na żywo** — loader czyta z SD tylko te glify, które
  faktycznie potrzebne (litery z nazwy fontu), z małym cache LRU. Bardziej
  „żywe", ale więcej ruchomych części i zależne od realnej szybkości SD_MMC.

Rekomendacja: **A** — mniej ryzyka, ekran otwiera się natychmiast niezależnie
od liczby fontów.

## Etapy

### Etap 1 — Format pliku i narzędzie eksportu
- Zdefiniować binarny format `.fnt`: nagłówek (magic, wersja, firstChar,
  lastChar, height, liczba glifów) + tablica glifów + blob bitmap — dokładnie
  ta sama treść co dziś w `EmbeddedFontVariant`, tylko zapisana do pliku
  zamiast do tablicy C.
- Rozszerzyć `tools/generate_embedded_font.py` o drugi tryb wyjścia (pisze
  `.fnt` zamiast/obok `.h`) — logika doboru glifów z CSV zostaje, zmienia się
  tylko writer.
- Zostawić dokładnie **jeden** font w dotychczasowym formacie flash/PROGMEM
  jako twardy fallback (proponuję Atkinson — już ma uzasadnienie czytelności
  w istniejącym kodzie/nazwie).

### Etap 2 — Runtime loader z SD (zrobione)
- Nowa klasa `SdFontLoader` (wzorzec z `PluginLoader.cpp`/`EpubConverter.cpp`):
  otwiera `/fonts/<nazwa>_<rozmiar>.fnt` przez `SD_MMC`, całość do bufora
  `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`, waliduje magic/wersję/zakresy,
  zwraca `EmbeddedFontVariant` wskazujący w ten bufor.
- Cache: trzymać rezydentnie tylko aktualnie wybrany font (2 warianty
  rozmiaru) + fallback Atkinson zawsze dostępny bez SD. Zmiana fontu w
  ustawieniach = jeden odczyt pliku (~200–400 KB), nie coś w hot-path
  renderowania.
- Miejsce spięcia: `extraFontVariant()` / `extraFontVariant70()`
  (`DisplayManager.cpp:348`, `:357`) — dziś indeksują stałą tablicę
  `kExtraFontVariants`, docelowo pytają loader. Reszta `DisplayManager.cpp`
  nie wie o różnicy.
- Obsługa braku/uszkodzenia pliku: fallback na Atkinson + widoczny komunikat
  (nie cichy fallback — użytkownik ma wiedzieć, że czcionka się nie wczytała).

### Etap 3 — Odchudzenie flasha (zrobione, ze zmianą zakresu — patrz sekcja "Zrobione" wyżej)
- Usunąć z firmware 9 z 10 obecnych fontów wbudowanych (Serif, OpenDyslexic,
  Literata, Merriweather, Lora, Bitter, EB Garamond, Vollkorn, Gelasio),
  zostawić tylko Atkinson jako fallback.
- Zweryfikować build: flash powinien spaść z 83.4% z powrotem w okolice
  bazowej wartości (firmware + 1 font).

### Etap 4 — Rozszerzenie katalogu do ~20 fontów (zrobione)
- Dobrać kolejnych ~10–13 krojów pod czytanie książek (flash już nie jest
  ograniczeniem), wygenerować `.fnt` dla wszystkich przez narzędzie z Etapu 1.
- Sprawdzić pokrycie znaków zgodne z obecnym zestawem tłumaczeń (polskie
  znaki ą/ę/ś/ć/ź/ż/ń/ó/ł — już wymagane, patrz `firstChar`/`lastChar`
  istniejących fontów).

### Etap 5 — Dystrybucja plików fontów na kartę (zrobione częściowo)
- Pliki `.fnt` nie mogą jechać w binarce firmware — muszą trafić na SD osobno.
- Paczka `fonts-pack-vX.zip` jako dodatkowy asset w GitHub Release (obok
  `flower-firmware.bin`), do ręcznego rozpakowania na kartę SD przez czytnik
  kart (albo przez USB Mass Storage, które czytnik już obsługuje —
  `UsbMassStorageManager.cpp`).
- **Zrobione** (patrz sekcja "Automatyczne pobieranie w tle" wyżej): przy
  boocie, po SD repair, i co 60 s dopóki paczka nie jest kompletna,
  `App::refreshFontPackComplete()` sprawdza `/fonts/` względem 17 fontów;
  brakujące pobierają się same przez Wi-Fi z Release'u, bez interakcji
  użytkownika i bez BLE/companion app (zwykłe stacja Wi-Fi + GitHub Releases,
  ten sam kanał co OTA firmware).

### Etap 6 — Ekran wyboru fontu (zrobione, patrz sekcja "Zrobione" na górze)
- Wdrożono opcję A: pre-renderowane miniaturki nazw, w flashu.
- Pozostaje do zweryfikowania na sprzęcie: czas otwarcia ekranu wyboru fontu
  przy 20 pozycjach, czas przełączenia fontu w czytniku (odczyt SD), wygląd
  miniaturek na realnym ekranie, oraz że fallback Atkinson faktycznie działa
  przy braku karty/pliku dla aktywnego fontu.

## Co świadomie zostaje bez zmian
- Reszta ekranu typografii (suwaki rozmiar/odstępy/kotwica/guide, słowa
  widma, kolor podświetlenia, potwierdzenie resetu) — już zrobione i
  potwierdzone fizycznie, ten plan tego nie rusza.
- Kompresja danych fontu — pominięta świadomie, bo margines miejsca na karcie
  4 GB jest na tyle duży (patrz sekcja wyżej), że nie opłaca się komplikować
  loadera kompresją na start. Można wrócić do tematu, jeśli realny czas
  wczytywania z SD_MMC okaże się problemem w Etapie 2.
