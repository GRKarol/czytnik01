# Sklep pluginów Flower

Statyczny katalog pluginów do dystrybucji przez aplikację.

Aplikacja PWA pobiera `index.json`, pokazuje listę dostępnych pluginów i
po wybraniu przez klienta pobiera odpowiedni `.zip` i wysyła go na
urządzenie przez WiFi (komenda `install-plugin`).

## Schemat `index.json`

```jsonc
{
  "version": 1,
  "updated": "2026-05-25",
  "plugins": [
    {
      "id": "focus-timer",           // unikalny identyfikator
      "name": "Klepsydra",           // pokazywane klientowi
      "tagline": "...",              // krótki opis (do listy)
      "description": "...",          // długi opis (do szczegółów)
      "version": "1.0.0",            // semver
      "icon": "icons/focus-timer.svg",
      "package": "packages/focus-timer-1.0.0.zip",
      "sizeBytes": 12480,
      "tags": ["produktywność"],
      "minFirmware": "0.1.0",        // minimalna wersja firmware
      "author": "Flower",
      "status": "planned"            // pomijaj jeśli wydane
    }
  ]
}
```

## Format paczki `.zip`

```
focus-timer-1.0.0.zip
├── manifest.json    # metadane pluginu
├── icon.png         # ikona dla menu urządzenia
├── strings.json     # tłumaczenia (pl, en)
└── assets/          # dowolne zasoby (fonty, układy, audio)
```

`manifest.json` w paczce:

```jsonc
{
  "id": "focus-timer",
  "version": "1.0.0",
  "name": { "pl": "Klepsydra", "en": "Focus Timer" },
  "entry": "focus_timer",          // wewnętrzna nazwa modułu w firmware
  "menu": {
    "label": { "pl": "Klepsydra", "en": "Focus Timer" },
    "icon": "icon.png"
  },
  "config": [                       // opcjonalne, generuje ekran ustawień
    { "key": "workMinutes", "label": "Praca (min)", "type": "int", "default": 25 },
    { "key": "breakMinutes", "label": "Przerwa (min)", "type": "int", "default": 5 }
  ]
}
```

## Jak dodajemy nowy plugin

1. Zbuduj `.zip` zgodny ze schematem.
2. Wrzuć go do `public/plugins/packages/`.
3. Dopisz wpis do `index.json`.
4. Commit + push — CI deployuje na GitHub Pages, klienci widzą go w app.

## Status

- `index.json` jest na razie z 3 pluginami: `focus-timer` (klepsydra,
  status finalny), `voice-notes` (planowany), `music-player` (planowany).
- `.zip`-y i ikony pluginów **jeszcze nie istnieją** — to TODO. App
  pokaże je jako "planned" / "wkrótce" do momentu wgrania paczek.
