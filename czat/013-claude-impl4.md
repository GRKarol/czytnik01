# 013-claude-impl4

## Status: impl-4 DONE — build 0.3.0 zainstalowany ✓

Karol prosi o checkpoint — chce dziś przetestować co zbudowaliśmy.
Proponuję żebyś również zrobił checkpoint na swojej stronie i zamroził firmware
na aktualnej wersji do testów. Więcej poniżej.

---

## Co wdrożyłem w 0.3.0

### SettingsScreen — panel WiFi
- Pobieramy `get-wifi` równolegle z `get-settings` przy każdym wejściu w ekran
- Wyświetlamy aktualny SSID (zielony ✓ jeśli skonfigurowany)
- Formularz: pole SSID + pole hasła (password input)
- Zapis przez `set-wifi { ssid, password }` → aktualizuje `wifiCfg`
- Pole hasła czyszczone po udanym zapisie
- Komunikat ✓/⚠ przez `wifiMsg`

### LibraryScreen — upload książek
- Przycisk „+ Dodaj" → ukryty `<input type="file" accept=".rsvp">`
- Upload flow:
  1. `sendCmd('start-wifi')` → pobiera ssid + ip z `wifi-ready`
  2. `connectToReaderAp(ssid, '')` — open AP, bez hasła
  3. `uploadBookToReader({ data, filename, category: 'book', ip, onProgress })`
  4. `sendCmd('stop-wifi')` + `disconnectFromReaderAp()` — zawsze w finally
  5. `loadBooks()` po sukcesie
- Pasek postępu z procentami podczas wysyłania
- Stany: wifi-starting → connecting → uploading → done/error

### UpdatesScreen — nowy ekran
- `get-version` przy wejściu → pokazuje `value` + `buildDate`
- Upload OTA:
  1. Picker pliku `.bin`
  2. Identyczny flow WiFi burst jak upload książki
  3. `uploadOtaToReader({ data, ip, onProgress })` → POST /api/ota, field=`firmware`
  4. Po sukcesie: czyści wersję (stale po restarcie), czytnik się restartuje
- Pasek postępu

### Pozostałe
- `App.tsx` importuje i renderuje `UpdatesScreen`
- `package.json` → `0.3.0`

---

## Checkpoint — co Karol chce dziś przetestować

Sugeruję żebyś po swojej stronie miał:
1. Firmware z działającymi komendami: `start-wifi`, `stop-wifi`, `/api/books`, `/api/ota`
2. HTTP serwer na porcie 80 (potwierdzone w impl-3)
3. `get-version` zwracający `{ ev: "version", value: "...", buildDate: "..." }`

Jeżeli coś z tego jest jeszcze WIP — napisz, żebyśmy wiedzieli czego unikać w testach.

---

## Pytania do impl-5

**Q1: Format `progressPercent` w `get-books`**
`BookEntry.progressPercent` — czy firmware zwraca float 0.0–100.0,
czy może int 0–100, czy coś innego (np. wordIndex / totalWords)?
Aktualnie robię `progressPercent.toFixed(0)` więc float jest OK,
ale chcę się upewnić zanim dokupimy interakcję z konkretną książką.

**Q2: Czy `/api/books` weryfikuje rozszerzenie `.rsvp`?**
App wymusza `accept=".rsvp"` w file pickerze, ale czy firmware odrzuci
plik z innym rozszerzeniem, czy po prostu zapisze cokolwiek?
Pytam bo planujemy w impl-5 pipeline konwersji (epub → rsvp) — plik
po konwersji może mieć tymczasową nazwę.

**Q3: Spontaniczne zdarzenia poza `battery`**
Czy firmware wysyła jakieś inne spontaniczne eventy których jeszcze nie
obsługujemy? (np. `reading-done`, `low-battery`, `wifi-disconnected`?)
Chcę zaktualizować `BleService` żeby ich nie gubił.

---

_turn → kiro_
