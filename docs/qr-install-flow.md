# Instalacja PWA przez kod QR

Jak to ma działać dla klienta i czego się spodziewać na różnych urządzeniach.

## Co skanujemy

Kod QR z URL:

```
https://grkarol.github.io/czytnik01/app/
```

(URL końcowy zależy od domeny — patrz `docs/roadmap.md`.)

## Co się dzieje po skanie

### Android (Chrome / Edge)

1. Aparat otwiera link w Chrome.
2. Strona ładuje się i rejestruje service worker.
3. Po krótkiej chwili przeglądarka pokazuje **mini-infobar** "Add Czytnik01
   to Home screen". Dodatkowo nasza aplikacja sama wywołuje
   `beforeinstallprompt` i pokazuje pasek instalacyjny (`czytnik-install-prompt`).
4. Klient klika → ikona ląduje na ekranie głównym.
5. Otwarcie z ikony → standalone (bez paska URL).

> **Uwaga:** "od razu zainstaluje się automatycznie" nie jest możliwe na
> żadnej platformie. Każdy system wymaga zgody użytkownika (1-2 kliknięcia).
> Najbliżej tego stanu jest Android + Chrome — tam wystarczy jeden tap na
> banner. Maksymalnie wygładzić to można tylko po stronie UX (duży, jasny CTA).

### iOS Safari

iOS **nie wspiera** `beforeinstallprompt`. Klient musi:

1. W Safari kliknąć ikonę Share.
2. Wybrać "Add to Home Screen".

Trzeba mu to pokazać — w PWA wykrywamy iOS i pokazujemy ilustrowaną
podpowiedź (TODO w `src/app/components/install-prompt.element.ts`).

> **Drugi problem na iOS:** Web Serial **nie jest wspierany** w Safari ani
> w żadnej przeglądarce na iOS. Jeśli klient ma iPhone'a i ma się łączyć
> z urządzeniem przez USB-C, musimy mu albo dać iOS app (Capacitor +
> CoreBluetooth, jeśli urządzenie obsłuży BLE), albo wymóc Android/desktop.
>
> **Decyzja do podjęcia:** czy wspieramy iOS na MVP? Jeśli tak, to
> docelowa transmisja to BLE, a USB tylko dla Androida/desktopu.

### Desktop (Chrome / Edge)

1. Link otwiera się w przeglądarce.
2. W pasku adresu pojawia się ikona "Install".
3. Aplikacja po instalacji działa jako okno standalone (bez tabów).

## Generowanie kodu QR

Mały skrypt do wygenerowania QR + nadruku można dodać później do
`tools/`. Na MVP wystarczy generator online (np. `qrcode.show/<URL>`).

Jeśli chcemy "deep linkować" do konkretnego flow (np. setup wizard ze
spakowanym device ID), używamy hash params:

```
https://grkarol.github.io/czytnik01/app/#setup&id=AB12CD34
```

Hash nie powoduje fetcha — aplikacja może go odczytać po starcie.

## Przyszłość: link do flashera dla klientów-developerów

Drugi kod QR (np. na pudełku, mniejszy) prowadzący do `/` — flashera, gdyby
trzeba było ratować urządzenie po nieudanej aktualizacji.
