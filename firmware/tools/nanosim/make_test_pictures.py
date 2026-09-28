"""Test cover and spine pictures for the simulator, in the reader's format.

Writes extras_cover.img (92x116) and extras_spine.img (36x72): "FBI1",
width and height as little-endian uint16, then RGB565 pixels, little-endian
-- the same bytes the Flower app uploads to /api/books/picture.
Pass a picture path to crop it instead of drawing a sample:
    python make_test_pictures.py [photo.jpg]
"""
import struct
import sys

from PIL import Image, ImageDraw, ImageFont


def encode(image: Image.Image) -> bytes:
    image = image.convert("RGB")
    out = bytearray(b"FBI1" + struct.pack("<HH", image.width, image.height))
    raw = image.tobytes()
    for i in range(0, len(raw), 3):
        r, g, b = raw[i], raw[i + 1], raw[i + 2]
        out += struct.pack("<H", ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
    return bytes(out)


def crop_to(image: Image.Image, width: int, height: int) -> Image.Image:
    scale = max(width / image.width, height / image.height)
    resized = image.resize((round(image.width * scale), round(image.height * scale)), Image.LANCZOS)
    left = (resized.width - width) // 2
    top = (resized.height - height) // 2
    return resized.crop((left, top, left + width, top + height))


def sample_cover() -> Image.Image:
    image = Image.new("RGB", (92, 116))
    draw = ImageDraw.Draw(image)
    for y in range(116):
        t = y / 115
        draw.line([(0, y), (91, y)], fill=(int(20 + 40 * t), int(40 + 90 * t), int(90 + 60 * t)))
    draw.ellipse([46, 18, 82, 54], fill=(250, 196, 80))
    draw.polygon([(0, 116), (0, 80), (30, 62), (60, 84), (92, 70), (92, 116)], fill=(24, 36, 30))
    font = ImageFont.load_default()
    draw.text((16, 88), "QUO", fill=(255, 255, 255), font=font)
    draw.text((16, 100), "VADIS", fill=(255, 255, 255), font=font)
    return image


def sample_spine() -> Image.Image:
    image = Image.new("RGB", (36, 72), (120, 30, 40))
    draw = ImageDraw.Draw(image)
    draw.rectangle([0, 20, 35, 26], fill=(230, 190, 90))
    draw.rectangle([0, 50, 35, 52], fill=(230, 190, 90))
    draw.ellipse([10, 30, 26, 46], outline=(255, 255, 255))
    return image


if __name__ == "__main__":
    if len(sys.argv) > 1:
        photo = Image.open(sys.argv[1])
        cover = crop_to(photo, 92, 116)
        spine = crop_to(photo, 36, 72)
    else:
        cover = sample_cover()
        spine = sample_spine()
    with open("extras_cover.img", "wb") as f:
        f.write(encode(cover))
    with open("extras_spine.img", "wb") as f:
        f.write(encode(spine))
    print("extras_cover.img, extras_spine.img")
