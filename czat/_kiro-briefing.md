# Briefing dla nowego Kiro — projekt Flower

> Ten plik czytasz jeśli zaczynasz pracę nad projektem Flower jako nowa instancja Kiro.
> Aktualizowany przez Claude. Ostatnia aktualizacja: 2026-06-17.

---

## Co to jest Flower?

Czytnik e-booków oparty na ESP32-S3 z e-paper display.
- **Kiro** = firmware ESP32-S3 (C++ / Arduino / NimBLE)
- **Claude** = aplikacja Android (Capacitor + React + TypeScript + Vite)
- Komunikacja: **BLE GATT** (JSON Lines, własny protokół)
- Dodatkowe: **WiFi AP** do uploadu książek (.rsvp format)

---

## Gdzie są pliki

**Firmware Kiro:** gdzieś na Twoim komputerze (platforma PlatformIO lub Arduino)

**Aplikacja Claude:** `C:\Users\karol\Documents\flower\`
- `src/screens/` — ekrany React
- `src/services/BleService.ts` — warstwa BLE
- `src/services/WifiService.ts` — upload przez WiFi
- `android/app/src/main/java/com/flower/reader/` — natywne pluginy Android
  - `FlowerBlePlugin.java` — BLE plugin
  - `FlowerWifiPlugin.java` — WiFi plugin

**Chat między Claude i Kiro:** `C:\Users\karol\Desktop\nowy folder\czat\`
- Numerowane pliki markdown: `NNN-claude-*.md` lub `NNN-kiro-*.md`
- `_turn.txt` — kto ma ruch (claude/kiro)
- `_index.md` — indeks wiadomości

---

## Protokół BLE

**Service UUID:** `f10e7e10-e5c9-4b0a-9c3e-1f3a5b7c9d2e`
**CMD characteristic:** `f10e7e11-...` (write)
**EVT characteristic:** `f10e7e12-...` (notify)

**Framing (1 bajt nagłówek):**
- bit1 (0x02) = START (pierwszy chunk)
- bit0 (0x01) = MORE (są kolejne chunki)
- Payload = reszta bajtu po nagłówku

**Format wiadomości:** JSON Lines (`{"cmd":"...","key":"val"}\n`)

**Komendy → eventy:**
| CMD | EVT odpowiedź |
|-----|---------------|
| `auth` | `auth-ok` |
| `get-settings` | `settings` (nested: `data.reading`, `data.display`, ...) |
| `set-settings` | `settings-ok` (flat: `{"cmd":"set-settings","reading":{...}}`) |
| `get-books` | `books` (`data: [BookEntry]`) |
| `start-wifi` | `wifi-ready` (flat: `ssid`, `pass`, `ip`) |
| `stop-wifi` | `wifi-stopped` |
| `get-wifi` | `wifi` (`configured`, `ssid`) |

**WiFi upload flow:**
1. BLE `start-wifi` → czytnik tworzy AP (`Flower-XXXXXX`, open)
2. `wifi-ready` event: `{ssid, pass:"", ip:"192.168.4.1"}`
3. Android łączy się z AP przez `WifiNetworkSpecifier`
4. HTTP multipart POST `http://{ip}/upload` (plik .rsvp)
5. BLE `stop-wifi` → czytnik wyłącza AP

---

## Format .rsvp

Własny format e-booków dla czytnika.  
Metadane: tytuł, autor, kategoria, postęp.  
Pliki przechowywane w `/books/` na SPIFFS/LittleFS.

**WAŻNE (aktualny bug):** tytuł w metadanych .rsvp może mieć złe enkodowanie
(bajty 0x82, 0xB5 zamiast poprawnego UTF-8 dla Ł, ż).
Ścieżka pliku (pole `name`) ma poprawne UTF-8 — to jest workaround po stronie Android.

---

## Historia bugów i aktualny status

### ✅ Fix: signed char w jsonEscape() (deployed)
ESP32 `char` jest signed. Bajty ≥ 0x80 (polskie znaki UTF-8) porównywały się jako ujemne,
co powodowało ich filtrowanie.
**Fix Kiro:** `uint8_t c = static_cast<uint8_t>(s[i]);`

### ✅ Fix: klik w książkę w LibraryScreen
Był brak `onClick` na przyciskach książek.
**Fix Claude:** bottom sheet modal przy kliknięciu.

### ✅ Fix: WiFi upload (format wifi-ready)
Aplikacja pobierała `ssid` z zagnieżdżonego `data.ssid` zamiast z flat `ssid`.
**Fix Claude:** flat format, 300ms delay.

### ❌ Aktualny bug: title z .rsvp ma złe enkodowanie
Pole `name` (ścieżka pliku) ma poprawne UTF-8, pole `title` (z metadanych .rsvp) ma złe bajty.
- `Ł` (U+0141, UTF-8: 0xC5 0x81) → przychodzi jako 0x82
- `ż` (U+017C, UTF-8: 0xC5 0xBC) → przychodzi jako 0xB5
**Workaround po stronie Android (wdrożony):** jeśli `title` zawiera znak zastępczy U+FFFD,
użyj nazwy pliku jako tytułu.
**Wymaga fixu po stronie firmware:** jak Kiro czyta/przechowuje tytuł w .rsvp?

---

## Zasady współpracy

1. **Kiro robi firmware. Claude robi Android/web.** Nie wchodź w swój obszar bez pytania.
2. **Jeden problem na raz.** Nie dodawaj 12 rzeczy naraz.
3. **BLOCKED:user** tylko dla decyzji które realnie zmieniają produkt lub łamią backward compat.
4. **Po każdej wiadomości do Kiro:** zaktualizuj `_turn.txt` na "kiro".
5. **Numeracja wiadomości:** `NNN-kiro-*.md` lub `NNN-claude-*.md`, NNN rośnie monotoniczne.
6. **Build:** `cd C:\Users\karol\Documents\flower && build_apk.bat`
   (wymaga `JAVA_HOME=C:\Program Files\Android\Android Studio\jbr`)
7. **Install:** `C:\Users\karol\AppData\Local\Android\Sdk\platform-tools\adb.exe -s R5CN30BJRCW install -r C:\Users\karol\Documents\flower\android\app\build\outputs\apk\debug\app-debug.apk`

---

## Aktualny build

**Wersja:** 0.3.0-030m (lub wyżej, sprawdź package.json)
**Zainstalowany na telefonie:** tak (Samsung Galaxy, ID: R5CN30BJRCW)
**Czytnik BLE:** Flower-F61B44 (MAC: 44:1B:F6:85:56:65)
