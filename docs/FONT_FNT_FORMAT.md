# Format pliku `.fnt` (fonty czytnika na karcie SD)

Status: zdefiniowany w Etapie 1 [planu fontów na SD](PLAN_FONTY_NA_SD.md).
Writer: `firmware/tools/generate_embedded_font.py --fnt-output <plik>`.
Reader (Etap 2, jeszcze nieistniejący): `SdFontLoader`.

Treść identyczna z tym, co dziś ląduje w nagłówku PROGMEM (`write_header()`
w tym samym narzędziu) — różni się tylko zapis: plik binarny zamiast stałych
C. Cel: `SdFontLoader` wczytuje cały plik do jednego bufora
(`heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`) i wskazuje wskaźnikami
`EmbeddedFontVariant` prosto w ten bufor, bez kopiowania pole po polu.

## Nagłówek (16 bajtów, little-endian)

| offset | rozmiar | pole         | opis |
|--------|---------|--------------|------|
| 0      | 4       | magic        | ASCII `"FNT1"`, bez terminatora |
| 4      | 2       | version      | obecnie `1` |
| 6      | 1       | firstChar    | pierwszy slot glifu (dziś zawsze 1) |
| 7      | 1       | lastChar     | ostatni slot glifu (dziś zawsze 255) |
| 8      | 1       | height       | wysokość rastra w px, jak `EmbeddedFontVariant::height` |
| 9      | 1       | reserved     | musi być `0` |
| 10     | 2       | glyphCount   | == `lastChar - firstChar + 1` |
| 12     | 4       | bitmapLength | długość bloku bitmap w bajtach |

## Tablica glifów

Zaraz po nagłówku, `glyphCount` rekordów po 8 bajtów, w kolejności slotów
`firstChar..lastChar`. Layout odpowiada `EmbeddedFontGlyph`
(`firmware/src/display/EmbeddedFontCommon.h`), łącznie z bajtem paddingu,
żeby loader mógł rzutować wskaźnik na tablicę bez przepisywania:

| offset w rekordzie | rozmiar | pole         |
|---------------------|---------|--------------|
| 0                    | 4       | bitmapOffset (uint32) |
| 4                    | 1       | xOffset (int8) |
| 5                    | 1       | width (uint8) |
| 6                    | 1       | xAdvance (uint8) |
| 7                    | 1       | pad, musi być `0` |

## Blok bitmap

Zaraz po tablicy glifów, `bitmapLength` surowych bajtów alpha (0-255,
1 bajt/piksel) — dokładnie ta sama treść co `kXxxBitmaps[]` w nagłówku
PROGMEM. `glyph.bitmapOffset` indeksuje w ten blok tak samo jak dziś w
tablicy w flashu.

## Walidacja przy wczytaniu (Etap 2)

Loader ma odrzucić plik i spaść na fallback Atkinson (z widocznym
komunikatem, nie cichym fallbackiem — patrz plan), jeśli:
- `magic != "FNT1"`,
- `version` nieobsługiwana,
- `glyphCount != lastChar - firstChar + 1`,
- rozmiar pliku != `16 + glyphCount*8 + bitmapLength`.

## Rozmiar całkowity pliku

```
16 + glyphCount * 8 + bitmapLength
```

Dla dzisiejszych fontów (255 slotów, ~190 KB bitmap na wariant) to ok.
16 + 255*8 + ~190000 ≈ 192 KB na wariant rozmiaru — zgodne z szacunkiem
"jeden odczyt pliku (~200-400 KB)" w planie.
