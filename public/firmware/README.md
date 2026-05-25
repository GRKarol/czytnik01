# Firmware binaries

Ten folder zawiera artefakty firmware używane przez stronę do flashowania (`/`).

## Pliki

- `manifest.json` — manifest [ESP Web Tools](https://esphome.github.io/esp-web-tools/).
  Definiuje, jakie chipy są wspierane i pod jakim offsetem trafia binarka.
- `czytnik01.bin` — **scalona** binarka firmware na ESP32-S3 (bootloader + partition table +
  aplikacja w jednym pliku, gotowa do zapisu pod offsetem `0x0`).
  Plik jest w `.gitignore` — nie commituj binarek do gita.

## Jak wygenerować scaloną binarkę

Z poziomu projektu firmware (PlatformIO/ESP-IDF):

```bash
# PlatformIO:
pio run -t buildfs   # opcjonalnie, jeśli używasz SPIFFS/LittleFS
pio run

# Scalanie (ESP-IDF / esptool):
esptool.py --chip esp32s3 merge_bin \
  -o czytnik01.bin \
  --flash_mode dio --flash_freq 80m --flash_size 16MB \
  0x0      bootloader.bin \
  0x8000   partitions.bin \
  0x10000  firmware.bin
```

Wynikowy `czytnik01.bin` wrzuć do `public/firmware/` (lokalnie albo z poziomu CI).

## Wiele wariantów / wiele wersji

Jeśli pojawi się więcej wariantów hardware lub release channeli, rozszerz
`manifest.json` o kolejne wpisy `builds[]` albo trzymaj kilka manifestów
i podmieniaj atrybut `manifest` na przycisku flashera.
