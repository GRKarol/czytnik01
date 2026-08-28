# Kanał OTA: Staging vs Production

## Dwa repo

- `GRKarol/czytnik01` — publiczne, główne. Tu trafia tylko sprawdzony,
  przetestowany fizycznie kod. Czytnik domyślnie aktualizuje się właśnie stąd.
- `GRKarol/czytnik01-staging` — prywatne, poligon testowy. Kopia całego
  repo, do niej trafiają zmiany zanim ktokolwiek je potwierdzi na sprzęcie.

Workflow: zmiana → commit na `staging` → build lokalny/CI → release na
`czytnik01-staging` → test fizyczny na czytniku przełączonym na kanał
Staging → dopiero po potwierdzeniu port do `czytnik01` jako release.

## Przełącznik w ustawieniach

Ustawienia zaawansowane → Wi-Fi → tryb dev → **OTA Channel**
(`App::otaChannelLabel()`, `App.cpp:5784`). Tap przełącza i zapisuje wybór
w NVS pod kluczem `ota_channel` (`App.cpp:351,4361`). Domyślnie `false` =
Production — nic się nie zmienia, dopóki ktoś świadomie nie przełączy na
Staging.

`App::preferredOtaConfig()` (`App.cpp:5679`) czyta ten flag i podmienia
`githubRepo` na `czytnik01-staging` (`kOtaStagingRepo`, `App.cpp:352`), nie
ruszając pola `githubOwner` — dzięki temu jeden przełącznik starcza, bez
grzebania w polu "OTA Source".

## Warunek: release z assetem na staging

Sam przełącznik nic nie da, jeśli na `GRKarol/czytnik01-staging` nie ma
opublikowanego release z assetem `flower-firmware.bin` — to ten sam
`release.yml`, który już działa na głównym repo, wystarczy wypchnąć tag na
staging (`git push staging vX.Y.Z`).

## Rollback przy złej aktualizacji

Po restarcie po OTA firmware musi dojść do potwierdzonego punktu zdrowia
(ekran wyrysowany, storage/reader zainicjalizowane) i dopiero wtedy woła
`esp_ota_mark_app_valid_cancel_rollback()` (`App.cpp:1009`). Jeśli nowy
obraz crashuje wcześniej, zostaje w stanie `PENDING_VERIFY` i bootloader
sam wraca do poprzedniej działającej partycji przy kolejnym restarcie —
czytnik nie zostaje uwięziony w złej wersji.

To wymaga `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` i partycji OTA
(`otadata` + `ota_0`/`ota_1`). Oba warunki są już spełnione bez żadnej
zmiany w `platformio.ini`:

- Board `esp32-s3-r8-opi` (`boards/esp32-s3-r8-opi.json`) używa
  `default_16MB.csv`, który ma `otadata`, `app0` (`ota_0`) i `app1`
  (`ota_1`).
- Precompiled Arduino core dla wariantu pamięci `qio_opi` ma
  `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=1` domyślnie (sprawdzone w
  `framework-arduinoespressif32/tools/sdk/esp32s3/qio_opi/include/sdkconfig.h`).

Framework Arduino w PlatformIO jest precompilowany — nie ma tu mechanizmu
`sdkconfig.defaults`, więc tego ustawienia nie da się (i nie trzeba) włączać
przez `platformio.ini`. Jedyny kod aplikacyjny potrzebny do działania
rollbacku to wywołanie `esp_ota_mark_app_valid_cancel_rollback()` opisane
wyżej.
