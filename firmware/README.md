# Firmware (Czytnik01 / Flower)

Kod firmware uruchamiany na urządzeniu (**ESP32-S3**, Waveshare Touch-LCD 3.49).

## Pochodzenie

Punkt startowy: **[ionutdecebal/rsvpnano](https://github.com/ionutdecebal/rsvpnano)**
(MIT). Cała struktura `src/` + `tools/` + `boards/` + `third_party/` + `platformio.ini`
została skopiowana 1:1 i będzie dalej rozwijana pod naszą markę.

Upstream README zachowany jako [`README.upstream.md`](README.upstream.md).

## Struktura

```
firmware/
├── src/
│   ├── main.cpp
│   ├── app/          # AppState, App, Localization
│   ├── audio/        # AudioManager
│   ├── board/        # BoardConfig (piny, hardware mapping)
│   ├── display/      # AXS15231B driver + wbudowane fonty (Serif, Atkinson, OpenDyslexic)
│   ├── input/        # ButtonHandler, TouchHandler
│   ├── reader/       # ReadingLoop, BookContent, BookWordSource
│   ├── rss/          # RssFeedManager (czytnik RSS przez WiFi)
│   ├── storage/      # StorageManager, IndexedBookStore, EpubConverter (on-device)
│   ├── sync/         # CompanionSyncManager (sync z aplikacją towarzyszącą)
│   ├── text/         # LatinText (typografia)
│   ├── timer/        # FocusTimer (klepsydra — przyszły kandydat na "plugin")
│   ├── update/       # OtaUpdater (OTA przez HTTP — gotowy fundament pod nasz flow)
│   └── usb/          # UsbMassStorageManager (USB MSC tryb wgrywania książek)
├── tools/            # Skrypty hosta (eksport binarki, konwerter EPUB→.rsvp, fonty)
├── boards/           # PlatformIO board definitions
├── third_party/      # Fonty z licencjami (OFL)
└── platformio.ini    # Środowiska buildu
```

## Format książek

Urządzenie czyta natywnie pliki **`.rsvp`** (plain text z dyrektywami):

```
@rsvp 1
@title Tytuł książki
@author Autor
@source /books/source.epub
@chapter Rozdział 1
@para
… linie tekstu, dzielone na słowa przez firmware …
```

Konwersja na urządzeniu (jeśli włączone `RSVP_ON_DEVICE_EPUB_CONVERSION=1`):

- `.epub` → `.rsvp` (cache zostaje na karcie SD obok źródła)

Konwersja desktopowa (`tools/sd_card_converter/`) obsługuje dodatkowo:

- `.txt`, `.md`/`.markdown`, `.html`/`.htm`/`.xhtml`

**Nieobsługiwane jeszcze:** `.pdf`, `.mobi`, `.azw` — do dodania w naszym
JS converterze po stronie PWA.

## Build (lokalnie, opcjonalnie)

Tylko jeśli chcesz kompilować firmware. Standardowe użycie tego repo NIE
wymaga tego — PWA i flasher są niezależne.

```bash
# Instaluj PlatformIO Core:
pip install --upgrade platformio

cd firmware
pio run                            # build (domyślne env: waveshare_esp32s3_usb_msc)
pio run -t upload                  # upload przez USB
pio device monitor                 # serial monitor 115200

# Eksport scalonej binarki do web flashera:
python tools/export_web_firmware.py
# tworzy: ../public/firmware/czytnik01.bin
```

## Status

Na tym etapie firmware jest **kopią startową**. Plan dalszej pracy
([../docs/roadmap.md](../docs/roadmap.md)):

1. Rebrand: `RSVP Nano` → `Flower` / `Czytnik01` w stringach UI.
2. Komunikacja z aplikacją: WiFi (HTTP/WebSocket) + BLE (zadecydowane
   po pytaniach z Karolem).
3. Pluginowanie: wydzielić `FocusTimer` (klepsydra) jako pierwszy plugin
   i zdefiniować runtime pluginów.
4. OTA z naszego GitHub Releases zamiast obecnego endpointu rsvpnano.

## Licencja

MIT (dziedziczy po rsvpnano), patrz `LICENSE` w tym folderze.
