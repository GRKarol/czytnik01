"""Captioned preview sheet of the scroll-mode delays (nanosim output) for Karol."""
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFont

OUT = Path(__file__).resolve().parent / "out"
DEST = Path(r"C:\Users\karol\Desktop\pliki_bota")
FONT = ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", 30)
FONT_B = ImageFont.truetype(r"C:\Windows\Fonts\segoeuib.ttf", 38)
W = 1280
SCALE = 2


def shot(name, dim=None):
    im = Image.open(OUT / f"{name}.ppm").convert("RGB")
    im = im.resize((im.width * SCALE, im.height * SCALE), Image.NEAREST)
    if dim is not None:
        im = ImageEnhance.Brightness(im).enhance(dim)
    return im


def sheet(filename, title, items):
    blocks = []
    for caption, im in items:
        blocks.append((caption, im))
    height = 70 + sum(54 + im.height + 18 for _, im in blocks)
    out = Image.new("RGB", (W, height), (24, 24, 28))
    d = ImageDraw.Draw(out)
    d.text((16, 14), title, font=FONT_B, fill=(255, 255, 255))
    y = 70
    for caption, im in blocks:
        d.text((16, y + 8), caption, font=FONT, fill=(230, 200, 120))
        y += 54
        out.paste(im, (0, y))
        y += im.height + 18
    DEST.mkdir(exist_ok=True)
    out.save(DEST / filename, optimize=True)
    print("saved", DEST / filename)

sheet("v0412_przewijanie_opoznienia.png", "Ustawienia czytania w trybie przewijania", [
    ("Strona 1: układ tekstu + pauza", shot("v12_01_przewijanie_uklad")),
    ("Strona 2: tempo i opóźnienia, osobne od RSVP", shot("v12_02_przewijanie_tempo")),
    ("Pomoc (?) przy Reset tempa", shot("v12_05_pomoc_reset")),
])
