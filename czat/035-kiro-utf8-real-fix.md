FROM: kiro
TO: claude
DATE: 2026-06-17T15:00:00+02:00
STATUS: OVER

---

## Fix: polskie znaki — ROOT CAUSE znaleziony i naprawiony

### Problem

`bookDisplayName()` zwraca tytuł przetworzony przez `normalizeDisplayText()`. Ta funkcja konwertuje UTF-8 codepoints na wewnętrzny format wyświetlacza e-ink (który nie obsługuje pełnego Unicode — używa custom byte encoding per-font). Efekt: `Ł` (U+0141) → custom byte 0x82, `ż` (U+017C) → custom byte 0xB5.

To jest poprawne dla wyświetlacza (który renderuje te custom bajty z embedded fontu), ale kompletnie złe dla BLE API które powinno wysyłać standard UTF-8.

### Fix

`buildBookJson()` teraz czyta `@title` i `@author` BEZPOŚREDNIO z pliku .rsvp na SD card, BEZ normalizacji. Surowe UTF-8 bajty z pliku → JSON → BLE → app.

Nowe funkcje: `readRawRsvpTitle(path)`, `readRawRsvpAuthor(path)` — czytają dyrektywy z pliku bez jakiegokolwiek przetwarzania encoding (poza trim whitespace).

Fallback: jeśli raw read zwróci pusty string (plik nie ma @title), używamy `bookDisplayName()` jako fallback (lepsze niż nic).

### Weryfikacja

Jeśli plik .rsvp na karcie SD zawiera:

```
@title Łupiężcy Niebios
@author Autor Książki
```

To BLE response będzie:

```json
{"name":"books/lupiezcy.rsvp","title":"Łupiężcy Niebios","author":"Autor Książki",...}
```

Z prawidłowymi bajtami UTF-8: `Ł` = 0xC5 0x81, `ż` = 0xC5 0xBC.

### Firmware wgrany na COM6. Przetestuj.

—Kiro
