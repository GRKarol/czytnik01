#!/usr/bin/env bash
# Regenerates the flash-resident font-name-thumbnail headers used by the
# font-picker screen (Etap 6, docs/PLAN_FONTY_NA_SD.md): one small header per
# SD-backed typeface, covering only printable ASCII (32-126) at a small
# raster size, so a button can show a font's own name in its own face
# without loading that font from SD (which only keeps one typeface resident
# at a time — see SdFontLoader / extraFontVariant() in DisplayManager.cpp).
#
# Unlike tools/generate_font_pack.sh (which produces SD-card .fnt files that
# are NOT committed), the output of this script IS committed: it's flash
# data, same policy as the original 3 built-in fonts
# (EmbeddedAtkinsonFont.h etc). Re-run and commit the diff whenever the SD
# font catalog changes.
#
# Usage: tools/generate_font_thumbnails.sh
# Writes firmware/src/display/thumbnails/Embedded<Name>Thumbnail.h for all
# 17 SD-backed typefaces.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="$SCRIPT_DIR/../src/display/thumbnails"
SRC_DIR="$(mktemp -d)"
mkdir -p "$OUT_DIR"

trap 'rm -rf "$SRC_DIR"' EXIT

# name -> raw.githubusercontent.com/google/fonts/main/<path> (URL-encoded)
# Kept in sync with tools/generate_font_pack.sh.
declare -A FONT_URLS=(
  [literata]="ofl/literata/Literata%5Bopsz,wght%5D.ttf"
  [merriweather]="ofl/merriweather/Merriweather%5Bopsz,wdth,wght%5D.ttf"
  [lora]="ofl/lora/Lora%5Bwght%5D.ttf"
  [bitter]="ofl/bitter/Bitter%5Bwght%5D.ttf"
  [ebgaramond]="ofl/ebgaramond/EBGaramond%5Bwght%5D.ttf"
  [vollkorn]="ofl/vollkorn/Vollkorn%5Bwght%5D.ttf"
  [gelasio]="ofl/gelasio/Gelasio%5Bwght%5D.ttf"
  [ptserif]="ofl/ptserif/PT_Serif-Web-Regular.ttf"
  [ibmplexserif]="ofl/ibmplexserif/IBMPlexSerif-Regular.ttf"
  [cardo]="ofl/cardo/Cardo-Regular.ttf"
  [zillaslab]="ofl/zillaslab/ZillaSlab-Regular.ttf"
  [oldstandard]="ofl/oldstandardtt/OldStandard-Regular.ttf"
  [domine]="ofl/domine/Domine%5Bwght%5D.ttf"
  [alegreya]="ofl/alegreya/Alegreya%5Bwght%5D.ttf"
  [newsreader]="ofl/newsreader/Newsreader%5Bopsz,wght%5D.ttf"
  [notoserif]="ofl/notoserif/NotoSerif%5Bwdth,wght%5D.ttf"
  [spectral]="ofl/spectral/Spectral-Regular.ttf"
)

# name -> {symbol prefix, header file stem} — PascalCase matching the
# DisplayManager::ReaderTypeface enumerator names in DisplayManager.h.
declare -A FONT_SYMBOLS=(
  [literata]="Literata" [merriweather]="Merriweather" [lora]="Lora" [bitter]="Bitter"
  [ebgaramond]="EBGaramond" [vollkorn]="Vollkorn" [gelasio]="Gelasio"
  [ptserif]="PtSerif" [ibmplexserif]="IbmPlexSerif" [cardo]="Cardo"
  [zillaslab]="ZillaSlab" [oldstandard]="OldStandard" [domine]="Domine"
  [alegreya]="Alegreya" [newsreader]="Newsreader" [notoserif]="NotoSerif" [spectral]="Spectral"
)

declare -A FONT_LABELS=(
  [literata]="Literata" [merriweather]="Merriweather" [lora]="Lora" [bitter]="Bitter"
  [ebgaramond]="EB Garamond" [vollkorn]="Vollkorn" [gelasio]="Gelasio"
  [ptserif]="PT Serif" [ibmplexserif]="IBM Plex Serif" [cardo]="Cardo"
  [zillaslab]="Zilla Slab" [oldstandard]="Old Standard" [domine]="Domine"
  [alegreya]="Alegreya" [newsreader]="Newsreader" [notoserif]="Noto Serif" [spectral]="Spectral"
)

# Matches TARGET_HEIGHT_BASE in generate_font_pack.sh — point size is
# calibrated from this, then --thumbnail-target-height scales down from it.
TARGET_HEIGHT_BASE=52

for name in "${!FONT_URLS[@]}"; do
  url="https://raw.githubusercontent.com/google/fonts/main/${FONT_URLS[$name]}"
  echo "downloading $name..."
  curl -fsSL --max-time 30 -o "$SRC_DIR/$name.ttf" "$url"
done

for name in "${!FONT_URLS[@]}"; do
  symbol="${FONT_SYMBOLS[$name]}"
  label="${FONT_LABELS[$name]}"
  python3 "$SCRIPT_DIR/generate_embedded_font.py" "$SRC_DIR/$name.ttf" \
    --symbol-prefix "${symbol}Thumb" \
    --thumbnail-output "$OUT_DIR/Embedded${symbol}Thumbnail.h" \
    --font-label "$label" --target-height "$TARGET_HEIGHT_BASE"
done

echo "done: $(ls "$OUT_DIR"/*.h | wc -l) thumbnail headers in $OUT_DIR"
