# Plan: fonty książki wczytywane z karty SD zamiast z flasha

Status: Etap 1-3 zrobione (patrz niżej), Etap 4 czeka.
Branch roboczy: `main` na staging (ten sam co dotychczasowy refaktor typografii).

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

### Etap 4 — Rozszerzenie katalogu do ~20 fontów
- Dobrać kolejnych ~10–13 krojów pod czytanie książek (flash już nie jest
  ograniczeniem), wygenerować `.fnt` dla wszystkich przez narzędzie z Etapu 1.
- Sprawdzić pokrycie znaków zgodne z obecnym zestawem tłumaczeń (polskie
  znaki ą/ę/ś/ć/ź/ż/ń/ó/ł — już wymagane, patrz `firstChar`/`lastChar`
  istniejących fontów).

### Etap 5 — Dystrybucja plików fontów na kartę
- Pliki `.fnt` nie mogą jechać w binarce firmware — muszą trafić na SD osobno.
- Paczka `fonts-pack-vX.zip` jako dodatkowy asset w GitHub Release (obok
  `flower-firmware.bin`), do ręcznego rozpakowania na kartę SD przez czytnik
  kart (albo przez USB Mass Storage, które czytnik już obsługuje —
  `UsbMassStorageManager.cpp`).
- Przy boocie: sprawdzić czy `/fonts/` ma komplet plików z manifestu
  wbudowanego w firmware; jeśli brakuje — jasny komunikat, nie cichy fallback.
- Automatyczne dosyłanie fontów przez BLE/companion app (na wzór sync
  książek) — świadomie poza zakresem tego planu, do rozważenia później.

### Etap 6 — Ekran wyboru fontu
- Wdrożyć decyzję z sekcji wyżej (rekomendacja: opcja A, pre-renderowane
  miniaturki nazw).
- Zweryfikować na sprzęcie: czas otwarcia ekranu wyboru fontu przy 20
  pozycjach, czas przełączenia fontu w czytniku (odczyt SD), oraz że fallback
  Atkinson faktycznie działa przy braku karty/pliku.

## Co świadomie zostaje bez zmian
- Reszta ekranu typografii (suwaki rozmiar/odstępy/kotwica/guide, słowa
  widma, kolor podświetlenia, potwierdzenie resetu) — już zrobione i
  potwierdzone fizycznie, ten plan tego nie rusza.
- Kompresja danych fontu — pominięta świadomie, bo margines miejsca na karcie
  4 GB jest na tyle duży (patrz sekcja wyżej), że nie opłaca się komplikować
  loadera kompresją na start. Można wrócić do tematu, jeśli realny czas
  wczytywania z SD_MMC okaże się problemem w Etapie 2.
