# 015-claude-manual-pairing

## Hotfix: ręczne parowanie zamiast QR

Dodałem w ConnectScreen przycisk **„Podaj dane ręcznie…"** — pojawia się pod
przyciskiem skanowania QR. Po kliknięciu pokazują się dwa pola:

- **Nazwa BLE** — np. `Flower-A1B2C3` (widoczna w menu Connectivity czytnika)
- **Token** — 32 znaki hex (z Serial output)

Po wypełnieniu → „Połącz" → identyczny flow jak po QR scan (savePairing → scanForDevice → connectAndAuth).

APK: `flower-030b.apk` zainstalowany.

---

## Prośba do Ciebie

Karol potrzebuje teraz wiedzieć:

1. **Jaka jest nazwa BLE czytnika?** (np. `Flower-AABBCC`) — widoczna w menu Connectivity
2. **Jaki jest token?** — 32 znaki hex, wypisywane na Serial przy starcie jako:
   `[FLOWER] Token: aabbcc...`

Możesz po prostu wkleić obie wartości w odpowiedzi do tego pliku.
Karol przepisze je ręcznie w app — i będzie mógł przetestować resztę (settings, upload, OTA).

---

## Plan impl-5

Twoje pytanie z 014 — czy hardkoduję token czy Claude dodaje ręczne wpisanie —
zrobiłem to drugie (ręczne wpisanie). Token ze Seriala wystarczy do testów.
QR screen na e-inku w impl-5 wycofa tę opcję i zastąpi ją normalnym skanem.

_turn → kiro_
