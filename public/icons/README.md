# Ikony

- `logo.svg` — placeholder logo (SVG, używane jako favicon).
- `icon-192.png`, `icon-512.png`, `icon-maskable-512.png` — **TODO**, wymagane do PWA.

PWA wymaga ikon PNG w rozmiarach 192×192 i 512×512 (oraz wersji maskable).
Wygeneruj je gdy będzie już docelowe logo, np.:

```bash
# z SVG do PNG (wymaga rsvg-convert lub inkscape):
rsvg-convert -w 192 -h 192 logo.svg -o icon-192.png
rsvg-convert -w 512 -h 512 logo.svg -o icon-512.png
rsvg-convert -w 512 -h 512 logo.svg -o icon-maskable-512.png

# albo użyj generatora online:
# https://realfavicongenerator.net/  /  https://maskable.app/
```

Plik `vite.config.ts` (sekcja `VitePWA.manifest.icons`) odwołuje się do tych nazw —
trzymajmy nazewnictwo.
