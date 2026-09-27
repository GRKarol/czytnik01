#!/usr/bin/env python3
"""Generate src/display/NanoUiFonts.h: the Nano skin's proportional UI fonts.

Each family is rendered by FreeType (Pillow) at three pixel sizes -- the
strikes the Nano screens ask for as "size" 1/2/3 -- instead of scaling one
huge reader raster down or one tiny pixel font up. Glyphs are cropped to
their ink box and stored as 4-bit alpha, indexed by the firmware's
single-byte text encoding (src/text/LatinText.h), so Polish and the other
Central-European letters render natively.

Byte 0x7F (DEL, never produced by LatinText) carries U+2026 so truncated
labels end in a real ellipsis.

Sources are subsets (Latin + Latin Extended-A) of open-licensed fonts, kept
in tools/ui_fonts/ -- all SIL OFL 1.1:
  Inter, Noto Sans, Nunito (Google Fonts), Atkinson Hyperlegible (Braille
  Institute), Literata (TypeTogether), OpenDyslexic (Abbie Gonzalez).

Usage:  python tools/gen_nano_ui_fonts.py
"""

from __future__ import annotations

import pathlib

from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(__file__).resolve().parents[1]
FONT_DIR = ROOT / "tools" / "ui_fonts"
OUTPUT = ROOT / "src" / "display" / "NanoUiFonts.h"

# Keep the order in sync with DisplayManager::NanoUiFont.
# (id, display name, file, {axis name: value}, pixel sizes for strikes 0..2)
FAMILIES = [
    ("Inter", "Inter", "Inter-UI.ttf", {"Weight": 520}, (13, 17, 26)),
    ("NotoSans", "Noto Sans", "NotoSans-UI.ttf", {"Weight": 500, "Width": 100}, (13, 17, 26)),
    ("Atkinson", "Atkinson", "AtkinsonHyperlegible-UI.ttf", {}, (14, 18, 27)),
    ("Nunito", "Nunito", "Nunito-UI.ttf", {"Weight": 640}, (14, 18, 27)),
    ("Literata", "Literata", "Literata-UI.ttf", {}, (14, 18, 27)),
    ("OpenDyslexic", "OpenDyslexic", "OpenDyslexic-UI.ttf", {}, (12, 15, 23)),
]

FIRST_BYTE = 0x20
LAST_BYTE = 0xFF
ELLIPSIS_BYTE = 0x7F

# Same slot map as tools/generate_embedded_font.py / LatinText.h.
CUSTOM_GLYPH_CODEPOINTS = {
    0x01: 0x010E, 0x02: 0x010F, 0x03: 0x011A, 0x04: 0x011B, 0x05: 0x0147,
    0x06: 0x0148, 0x07: 0x0158, 0x08: 0x0159, 0x0E: 0x0164, 0x0F: 0x0165,
    0x10: 0x016E, 0x11: 0x016F, 0x12: 0x0150, 0x13: 0x0151, 0x14: 0x0170,
    0x15: 0x0171, 0x16: 0x00A1, 0x17: 0x00BF,
    0x80: 0x0152, 0x81: 0x0153, 0x82: 0x0141, 0x83: 0x0142,
    0x84: 0x010C, 0x85: 0x010D, 0x86: 0x0160, 0x87: 0x0161, 0x88: 0x017D,
    0x89: 0x017E, 0x8A: 0x0102, 0x8B: 0x0103, 0x8C: 0x0218, 0x8D: 0x0219,
    0x8E: 0x021A, 0x8F: 0x021B, 0x90: 0x011E, 0x91: 0x011F, 0x92: 0x015E,
    0x93: 0x015F, 0x94: 0x0130, 0x95: 0x0131, 0x96: 0x0104, 0x97: 0x0105,
    0x98: 0x0118, 0x99: 0x0119, 0x9A: 0x0106, 0x9B: 0x0107, 0x9C: 0x0143,
    0x9D: 0x0144, 0x9E: 0x015A, 0x9F: 0x015B, 0xB2: 0x0179, 0xB3: 0x017A,
    0xB4: 0x017B, 0xB5: 0x017C, 0xA1: 0x0100, 0xA2: 0x0101, 0xA3: 0x0112,
    0xA4: 0x0113, 0xA5: 0x0122, 0xA6: 0x0123, 0xA7: 0x012A, 0xA8: 0x012B,
    0xA9: 0x0136, 0xAA: 0x0137, 0xAB: 0x013B, 0xAC: 0x013C, 0xAE: 0x0145,
    0xAF: 0x0146, 0xB0: 0x0116, 0xB1: 0x0117, 0xB6: 0x012E, 0xB7: 0x012F,
    0xB8: 0x0172, 0xB9: 0x0173, 0xBA: 0x016A, 0xBB: 0x016B, 0xBC: 0x0110,
    0xBD: 0x0111, 0xBE: 0x014A, 0xBF: 0x014B, 0xD7: 0x0166, 0xF7: 0x0167,
    ELLIPSIS_BYTE: 0x2026,
}


def codepoint_for_byte(value: int) -> int:
    return CUSTOM_GLYPH_CODEPOINTS.get(value, value)


def load_font(family, size: int) -> ImageFont.FreeTypeFont:
    font = ImageFont.truetype(str(FONT_DIR / family[2]), size)
    axes = family[3]
    if axes:
        values = []
        for axis in font.get_variation_axes():
            name = axis["name"].decode() if isinstance(axis["name"], bytes) else axis["name"]
            values.append(axes.get(name, axis["default"]))
        font.set_variation_by_axes(values)
    return font


def render_strike(family, size: int):
    font = load_font(family, size)
    ascent, descent = font.getmetrics()
    cmap_font = font  # Pillow falls back to .notdef silently; detect via bbox of a known-missing char
    glyphs = []
    bitmap = bytearray()
    nibbles = []
    pad = size  # canvas margin for overhangs
    for value in range(FIRST_BYTE, LAST_BYTE + 1):
        cp = codepoint_for_byte(value)
        if cp < 0x20 or (0x7F <= cp < 0xA0 and value != ELLIPSIS_BYTE and value not in CUSTOM_GLYPH_CODEPOINTS):
            glyphs.append((len(nibbles), 0, 0, 0, 0, 0))
            continue
        ch = chr(cp)
        advance = font.getlength(ch)
        canvas = Image.new("L", (size * 3 + pad * 2, ascent + descent + pad * 2), 0)
        ImageDraw.Draw(canvas).text((pad, pad), ch, font=font, fill=255)
        box = canvas.getbbox()
        adv16 = max(0, round(advance * 16))
        if box is None:
            glyphs.append((len(nibbles), 0, 0, 0, 0, adv16))
            continue
        x0, y0, x1, y1 = box
        crop = canvas.crop(box)
        w, h = crop.size
        offset = len(nibbles)
        for px in crop.getdata():
            nibbles.append((px * 15 + 127) // 255)
        # x offset from pen, y offset of the top row relative to the baseline
        glyphs.append((offset, w, h, x0 - pad, y0 - pad - ascent, adv16))
    if len(nibbles) % 2:
        nibbles.append(0)
    for i in range(0, len(nibbles), 2):
        bitmap.append((nibbles[i] << 4) | nibbles[i + 1])
    # Cap height from 'H', x-height from 'x' (ink rows).
    def ink_height(ch):
        img = Image.new("L", (size * 3, ascent + descent + 4), 0)
        ImageDraw.Draw(img).text((0, 0), ch, font=font, fill=255)
        b = img.getbbox()
        return (b[3] - b[1]) if b else 0
    cap = ink_height("H")
    xh = ink_height("x")
    line = round(size * 1.28)
    return {
        "size": size,
        "ascent": ascent,
        "descent": descent,
        "cap": cap,
        "xheight": xh,
        "line": line,
        "glyphs": glyphs,
        "bitmap": bytes(bitmap),
    }


def emit():
    out = []
    out.append("#pragma once")
    out.append("")
    out.append("// AUTO-GENERATED by tools/gen_nano_ui_fonts.py -- do not edit by hand.")
    out.append("// Nano skin UI fonts: 3 strikes per family, 4-bit alpha, glyphs cropped to")
    out.append("// ink and indexed by the firmware's single-byte text encoding. Sources are")
    out.append("// SIL OFL 1.1 fonts, see tools/ui_fonts/.")
    out.append("")
    out.append("#include <stdint.h>")
    out.append("")
    out.append("struct NanoUiGlyph {")
    out.append("  uint32_t offset;   // first nibble in the strike's bitmap")
    out.append("  uint8_t width;")
    out.append("  uint8_t height;")
    out.append("  int8_t xOffset;    // ink left edge relative to the pen")
    out.append("  int8_t yOffset;    // ink top row relative to the baseline (negative = above)")
    out.append("  uint16_t advance;  // pen advance in 1/16 px")
    out.append("};")
    out.append("")
    out.append("struct NanoUiStrike {")
    out.append("  const uint8_t *bitmap;")
    out.append("  const NanoUiGlyph *glyphs;  // bytes 0x20..0xFF")
    out.append("  uint8_t pixelSize;")
    out.append("  uint8_t ascent;")
    out.append("  uint8_t descent;")
    out.append("  uint8_t capHeight;")
    out.append("  uint8_t xHeight;")
    out.append("  uint8_t lineHeight;")
    out.append("};")
    out.append("")
    out.append("struct NanoUiFamily {")
    out.append("  const char *name;")
    out.append("  NanoUiStrike strikes[3];")
    out.append("};")
    out.append("")
    out.append(f"constexpr uint8_t kNanoUiFirstByte = 0x{FIRST_BYTE:02X};")
    out.append(f"constexpr uint8_t kNanoUiLastByte = 0x{LAST_BYTE:02X};")
    out.append(f"constexpr char kNanoUiEllipsisByte = 0x{ELLIPSIS_BYTE:02X};")
    out.append("")
    total = 0
    family_entries = []
    for fam in FAMILIES:
        fid = fam[0]
        strikes = []
        for si, size in enumerate(fam[4]):
            s = render_strike(fam, size)
            strikes.append(s)
            name = f"kNanoUi{fid}{si}"
            total += len(s["bitmap"]) + len(s["glyphs"]) * 10
            out.append(f"// {fam[1]} {size}px: cap {s['cap']}px, line {s['line']}px, {len(s['bitmap'])} bytes")
            out.append(f"const uint8_t {name}Bitmap[] = {{")
            data = s["bitmap"]
            for i in range(0, len(data), 24):
                out.append("    " + ", ".join(f"0x{b:02X}" for b in data[i:i + 24]) + ",")
            out.append("};")
            out.append(f"const NanoUiGlyph {name}Glyphs[] = {{")
            for value, g in zip(range(FIRST_BYTE, LAST_BYTE + 1), s["glyphs"]):
                out.append(f"    {{{g[0]}, {g[1]}, {g[2]}, {g[3]}, {g[4]}, {g[5]}}},  // 0x{value:02X}")
            out.append("};")
            out.append("")
        entry = [f'    {{"{fam[1]}", {{']
        for si, s in enumerate(strikes):
            name = f"kNanoUi{fid}{si}"
            entry.append(
                f"        {{{name}Bitmap, {name}Glyphs, {s['size']}, {s['ascent']}, {s['descent']}, "
                f"{s['cap']}, {s['xheight']}, {s['line']}}},")
        entry.append("    }},")
        family_entries.append("\n".join(entry))
    out.append("const NanoUiFamily kNanoUiFamilies[] = {")
    out.extend(family_entries)
    out.append("};")
    out.append(f"constexpr uint8_t kNanoUiFamilyCount = {len(FAMILIES)};")
    out.append("")
    OUTPUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"wrote {OUTPUT} (~{total // 1024} KiB of font data)")


if __name__ == "__main__":
    emit()
