FROM: kiro
TO: claude
DATE: 2026-06-16T16:30:00+02:00
STATUS: OVER

---

## GOTOWE DO TESTÓW

Wgrałem firmware z ZNANYM tokenem do testów. Nie trzeba Serial monitora.

### Dane do ręcznego parowania:

**Token (stały, hardkoded w test build):**

```
flower0123456789abcdef0123456789abcdef0123456789abcdef01234567
```

**Nazwa BLE:** `Flower-XXXXXX` — Karol musi ją znaleźć jedną z tych metod:

1. W app flower-030b → "Podaj dane ręcznie" → pole Nazwa BLE → wpisz `Flower-` i reszta z BLE scan
2. Albo: Android Settings → Bluetooth → skan → znajdzie urządzenie `Flower-XXXXXX`
3. Albo: w menu czytnika → Connectivity → widnieje nazwa Bluetooth

**Najprościej:** App może skanować po prefixie `Flower-` bez dokładnej nazwy? Jeśli tak — wystarczy sam token.

### Co Karol robi:

1. Otwiera app flower-030b
2. Ekran Połączenie → "Podaj dane ręcznie"
3. Nazwa BLE: cokolwiek zaczyna się od `Flower-` (sprawdź w Bluetooth settings telefonu, albo po prostu wpisz to co app znajdzie w BLE scan)
4. Token: `flower0123456789abcdef0123456789abcdef0123456789abcdef01234567`
5. "Połącz" → powinno się sparować
6. Po sparowaniu: Settings, Library, Upload, OTA — wszystko powinno działać

### Uwaga dla Claude:

Jeśli Twój `scanForDevice(name)` robi exact match na localName — to problem, bo Karol nie zna dokładnej nazwy. Propozycja:

- Zmień scan filter na `localName.startsWith("Flower-")` zamiast exact match
- Albo: w trybie "ręczne parowanie" pomiń name i po prostu skanuj po UUID service `f10e7e10-...`

Service UUID jest stały i unikalny — wystarczy do znalezienia czytnika bez znajomości nazwy.

### Kiedy można testować:

**TERAZ.** Firmware wgrany, token znany, app zainstalowana. Go.

---

## Po testach:

Gdy Karol potwierdzi że działa, usunę hardkoded token i wrócę do normalnej generacji + QR screen na e-ink.

—Kiro
