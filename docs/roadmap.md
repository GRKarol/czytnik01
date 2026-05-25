# Roadmap

Plan dalszej pracy. Lista, nie kontrakt — układ ma sens dopiero po
przemyśleniu z klientem hardware'u.

## Faza 0 — Podkładka (TO JEST TERAZ)

- [x] Wipe starego repo, orphan branch.
- [x] Scaffold Vite + TS + Lit + vite-plugin-pwa.
- [x] Strona flashera z esp-web-tools, manifest `public/firmware/manifest.json`.
- [x] Stub PWA klienta z połączeniem przez Web Serial.
- [x] CI: build + auto-deploy na GitHub Pages.
- [x] Dokumentacja architektury, flow QR.

## Faza 1 — Firmware

Decyzje do podjęcia z klientem przed startem:

- [ ] Dokładny model płytki ESP32-S3 (Waveshare? DevKit? własna PCB?).
- [ ] Layout partycji (16 MB? 8 MB? OPI PSRAM?).
- [ ] Czy potrzebujemy NVS dla ustawień klienta, SPIFFS/LittleFS dla danych?
- [ ] Czy firmware ma własną webappkę przez WiFi AP (drugi tor obok PWA)?
- [ ] Stack: Arduino / ESP-IDF / Zephyr?

Repo firmware: osobne (czytnik01-firmware?). To repo będzie tylko
konsumować scaloną binarkę.

## Faza 2 — Protokół

- [ ] Spisać polecenia (`cmd`) i eventy (`ev`) między hostem i urządzeniem
      w `src/shared/device-protocol.ts`.
- [ ] Zdecydować: surowy JSON Lines czy COBS + binary frames z CRC?
- [ ] Wersjonowanie protokołu (`{"cmd": "hello", "v": 1}` ⇒ `{"ev": "hello", "v": 1, "fw": "..."}`).

## Faza 3 — PWA klienta — funkcje

> "Apka jak od DJI / drona" — duża, czytelna kontrola głównej funkcji,
> przyciski statusu, telemetria, ustawienia.

Konkrety zależą od tego, co umie urządzenie. Wstępne kandydatury:

- [ ] Dashboard: stan urządzenia (bateria, połączenie, tryb pracy).
- [ ] Główna funkcja (TBD wraz z klientem).
- [ ] Ustawienia (jasność, dźwięk, język, kalibracja).
- [ ] Aktualizacja firmware z poziomu aplikacji (OTA przez Serial /
      przeniesienie usera do flashera).
- [ ] Wsparcie / kontakt.

## Faza 4 — Dystrybucja

- [ ] Domena własna (np. `app.czytnik01.pl`) zamiast GitHub Pages.
- [ ] Generator etykiet QR (nadruk na pudełku) — skrypt w `tools/`.
- [ ] Sygnowanie firmware (opcjonalnie, dla anti-tampering).

## Otwarte pytania

- iOS: czy klient musi wspierać iPhone'y? Jeśli tak → BLE w firmware
  + albo Capacitor wrapper, albo Web Bluetooth (lepiej dziala na Androidzie
  niż na iOS Safari, który WB nie wspiera w ogóle).
- WiFi: czy urządzenie ma w ogóle dostęp do internetu, czy działa tylko
  point-to-point z PWA?
- Wielojęzyczność: PL only, czy też EN?
