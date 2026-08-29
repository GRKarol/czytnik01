# 017-claude-eod

## Podsumowanie dnia — co zrobiono, co nie działa, co dalej

### Co wdrożono dziś (app)

- **impl-4 (0.3.0):** WiFi panel w Settings, upload .rsvp w Library, UpdatesScreen z OTA
- **Hotfix 030b:** ręczne parowanie (token + BLE name bez QR)
- **Hotfix 030c:** odblokowanie przycisku przy tokenie ≠ 32 znaki, scan BLE bez filtra nazwy

### Co NIE działa — wyniki testów

**Problem 1: QR scan → „Nieznany kod QR"**
QR wyświetlany na czytniku (ekran WiFi) nie zawiera formatu `flower://pair?t=...&n=...`.
Stary flow. App odrzuca wszystko co nie startuje z `flower://pair`.
→ **Fix po stronie Kiro:** QR screen na e-inku w impl-5.

**Problem 2: Ręczne parowanie → scan zawiesza się na 3+ minuty**
App wywołuje `scanForDevice('')` (pusty filtr) → natywny plugin Java dostaje `{}` jako opts.
Nie wiadomo jak `FlowerBlePlugin.startScan({})` zachowuje się gdy brak `name` — może crashuje,
może filtruje wszystko, może Android BLE wymaga czegoś innego.
→ **Do zbadania:** kod `FlowerBlePlugin.java` metoda `startScan`. Czy pusty `name` jest
   obsługiwany? Czy trzeba alternatywnie filtrować po Service UUID?

### Co Kiro musi zrobić w impl-5 (priorytet #1)

1. **QR screen na e-inku** — wyświetl `flower://pair?t=TOKEN&n=BLE_NAME` jako QR.
   To odblokuje podstawowy flow parowania i sprawi że testy będą możliwe.
   BEZ tego nie możemy testować nic więcej.

### Co ja (Claude) muszę zrobić gdy Kiro wróci z QR screen

1. Zbadać `FlowerBlePlugin.java` → `startScan` → jak obsługuje brak/pusty `name`
2. Ewentualnie: dodać scan po Service UUID jako fallback (Android API to wspiera)
3. Usunąć tryb ręcznego parowania gdy QR screen będzie gotowy (lub zostawić jako debug)

### Stan APK

Ostatni zainstalowany: `flower-030c.apk`
Kod: `C:\Users\karol\Documents\flower` — kompiluje się OK, brak błędów TS.

---

Do jutra lub na następną sesję. Kiro — zrób QR screen, to odblokuje wszystko.

_turn → kiro_
