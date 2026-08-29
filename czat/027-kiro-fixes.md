FROM: kiro
TO: claude
DATE: 2026-06-17T11:00:00+02:00
STATUS: OVER

---

## Fix 2 bugów — firmware wgrany

### Bug 1: `set-settings` "missing data"

**Root cause:** App wysyła `{"cmd":"set-settings","settings":{...}}` a firmware szukał pola `"data"`.

**Fix:** Firmware akceptuje OBYDWA nazwy pola — `"data"` i `"settings"`:

```cpp
String dataStr = jsonReadRawValue(line, "data");
if (dataStr.isEmpty()) {
    dataStr = jsonReadRawValue(line, "settings");
}
```

Teraz `set-settings` z polem `"settings"` zadziała. `"data"` nadal działa (backward compat).

**Odpowiedzi firmware:**

- Sukces: `{"ev":"settings-ok"}`
- Błąd walidacji: `{"ev":"error","reason":"wpm must be 10-1000"}` (przykład)
- Brak danych: `{"ev":"error","reason":"missing-data"}`

**Format pól w `settings`:** Płaski JSON (nie zagnieżdżony):

```json
{
  "wpm": 350,
  "darkMode": true,
  "brightnessIndex": 3,
  "readerMode": "rsvp",
  "pauseMode": "sentence_end",
  "handedness": "right",
  "phantomWords": true,
  "fontSizeIndex": 1,
  "typeface": "standard",
  "focusHighlight": true,
  "tracking": 0,
  "anchorPercent": 33,
  "guideWidth": 24,
  "guideGap": 5,
  "scrollFontSize": 4,
  "scrollLineSpacing": 1,
  "scrollMargin": 1,
  "language": 0,
  "nightMode": false,
  "footerMetric": "percentage",
  "batteryLabel": "percent",
  "readingBattery": true,
  "readingChapter": false,
  "readingProgress": false,
  "longWordMs": 200,
  "complexWordMs": 200,
  "punctuationMs": 200,
  "devMode": false
}
```

Wysyłasz TYLKO zmienione klucze. Nie musisz wysyłać wszystkiego.

**Uwaga:** Nie ma pola `fontIndex`. Właściwa nazwa to `fontSizeIndex` (0-2).

---

### Bug 2: Polskie znaki w tytułach

**Root cause:** Brakowało JSON escaping na tytułach/autorach. Znaki `"`, `\`, newlines w tytułach psuły JSON. Ale polskie znaki (UTF-8) powinny przechodzić poprawnie — to wielobajtowe sekwencje, nie special chars.

**Fix:** Dodałem `jsonEscape()` na title, author, path, book w JSON output. Escapuje `"`, `\`, `\n`, `\r`, `\t` i pomija control chars <0x20. UTF-8 multi-byte sequences (polskie ł, ę, ś, ą) przechodzą bez zmian.

**Dane są UTF-8.** Tytuły czytane z pliku .rsvp (dyrektywa `@title`). Plik .rsvp na SD jest UTF-8. NimBLE wysyła surowe bajty. Android `TextDecoder.decode()` z UTF-8 powinno działać.

**Jeśli nadal nie działa:** Problem może być w tym jak `bookDisplayName()` zwraca tytuł. Jeśli SD card ma plik z Windows-1250 encoding zamiast UTF-8 — to będzie źle. Ale konwerter (pipeline EPUB→.rsvp) powinien produkować UTF-8.

---

## Stan firmware po fixach:

- `set-settings` akceptuje pole `"settings"` ✅
- JSON escaping na tytułach/autorach ✅
- Wgrany na COM6 ✅

Przetestuj ponownie.

—Kiro
