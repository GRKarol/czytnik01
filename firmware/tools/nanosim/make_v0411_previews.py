"""Captioned preview sheets of the v0.4.11 screens (nanosim output) for Karol."""
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


sheet("v0411_1_kreator_kolory_a.png", "Kreator: kolory kroków (1/2)", [
    ("Wygląd menu: cały ekran w wybranym motywie menu", shot("v11_02_kreator_wyglad_menu")),
    ("Motyw czytania, wybrany Jasny: ekran w kolorach czytania", shot("v11_03_kreator_motyw_jasny")),
    ("Motyw czytania, wybrany Nocny", shot("v11_03_kreator_motyw_nocny")),
    ("Następny krok (czcionka menu): powrót do motywu menu", shot("v11_04_kreator_czcionka_menu")),
])
sheet("v0411_1_kreator_kolory_b.png", "Kreator: kolory kroków (2/2)", [
    ("Czcionka do czytania: znów kolory czytania", shot("v11_05_kreator_czcionka_czytania")),
    ("Sposób czytania: kolory czytania", shot("v11_06_kreator_sposob_czytania")),
    ("Kolor podświetlenia: przyciski w wybranym kolorze, pełne nazwy", shot("v11_07_kreator_kolor_litery")),
    ("Aplikacja: powrót do motywu menu", shot("v11_08_kreator_aplikacja")),
])
sheet("v0411_2_podglad_czytania.png", "Podgląd czytania = prawdziwy ekran czytania", [
    ("RSVP, rozmiar Średni, po polsku (słowo się zmienia)", shot("v11_09_podglad_rsvp_a")),
    ("RSVP, kolejne słowo", shot("v11_09_podglad_rsvp_b")),
    ("Przewijanie strony: Średni rozmiar, wyjustowane, strona płynie", shot("v11_10_podglad_przewijania")),
])
sheet("v0411_3_wylaczanie_wygaszacz.png", "Wyłączanie i wygaszacz", [
    ("Wyłączanie (przytrzymany PWR): pierścień się zamyka...", shot("v11_11_wylaczanie_a")),
    ("...dalej...", shot("v11_11_wylaczanie_b")),
    ("...pełny, potem obraz powoli gaśnie", shot("v11_11_wylaczanie_c")),
    ("Wygaszacz po czasie pyta na przyciemnionym ekranie (tu symulacja)", shot("v11_12_wygaszacz_pytanie", dim=0.45)),
    ("Ustawienia wygaszacza: styl Słowa, Wyłącz po 30 s", shot("v11_13_wygaszacz_ustawienia")),
    ("Opis (?) przy Wyłącz po", shot("v11_14_wylacz_po_pomoc")),
])
sheet("v0411_4_ustawienia.png", "Tempo, Prowadnica, Ekran, bateria", [
    ("Czytanie: długie słowa i złożoność 100 ms, przecinki 100 ms", shot("v11_15_tempo_a")),
    ("Czytanie, strona 2: osobno Kropki 150 ms", shot("v11_15_tempo_b")),
    ("Wygląd czytania > Prowadnica: Strona + wszystko z pełnymi napisami", shot("v11_16_prowadnica_font4")),
    ("To samo w innej czcionce menu", shot("v11_16_prowadnica_font0")),
    ("System > Ekran: bez Prowadnicy, Wskaźnik baterii w całości", shot("v11_17_ekran_ustawienia")),
    ("Po stuknięciu Wskaźnik baterii: ikona w rogu pojawia się...", shot("v11_18_bateria_podglad_wejscie")),
    ("...i po sekundzie znika", shot("v11_18_bateria_podglad_pelny")),
])
sheet("v0411_5_karta_i_start.png", "Pierwszy ekran i karta SD", [
    ("Pierwszy ekran kreatora: bez napisu o przycisku Reboot", shot("v11_01_kreator_jezyk")),
    ("Nowa, niesformatowana karta", shot("v11_19_karta_niesformatowana")),
    ("Karta bez plików czytnika", shot("v11_20_karta_pobieranie")),
])
