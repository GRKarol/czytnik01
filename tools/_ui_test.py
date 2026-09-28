"""Clicks through the Flower app (mock reader) and saves screenshots."""
import http.server
import pathlib
import socketserver
import sys
import threading

from PIL import Image, ImageDraw
from playwright.sync_api import sync_playwright

root = pathlib.Path(__file__).resolve().parent.parent
dist = root / "dist-capacitor"
out = root / "tools" / "_ui_shots"
out.mkdir(exist_ok=True)

# A photo-like test picture.
photo = out / "photo.png"
img = Image.new("RGB", (1200, 1600))
d = ImageDraw.Draw(img)
for y in range(1600):
    t = y / 1599
    d.line([(0, y), (1199, y)], fill=(int(30 + 200 * t), int(60 + 90 * t), int(140 - 60 * t)))
d.ellipse([600, 250, 1000, 650], fill=(255, 200, 90))
d.polygon([(0, 1600), (0, 1000), (400, 800), (800, 1100), (1200, 900), (1200, 1600)], fill=(30, 45, 35))
d.rectangle([80, 1250, 1120, 1450], fill=(240, 240, 240))
d.text((120, 1300), "QUO VADIS", fill=(20, 20, 20))
img.save(photo)


class Quiet(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=str(dist), **k)

    def log_message(self, *a):
        pass


server = socketserver.TCPServer(("127.0.0.1", 5199), Quiet)
threading.Thread(target=server.serve_forever, daemon=True).start()

errors = []
with sync_playwright() as p:
    browser = p.chromium.launch()
    page = browser.new_page(viewport={"width": 412, "height": 860}, device_scale_factor=2)
    page.on("pageerror", lambda e: errors.append(str(e)))
    page.on("console", lambda m: errors.append(m.text) if m.type == "error" else None)
    page.goto("http://127.0.0.1:5199/")
    page.wait_for_timeout(1500)
    # First-run overlays (language, onboarding) if any.
    for label in ["Polski", "Pomiń", "Zamknij", "Dalej"]:
        btn = page.get_by_role("button", name=label, exact=True)
        if btn.count() > 0 and btn.first.is_visible():
            btn.first.click()
            page.wait_for_timeout(300)
    page.screenshot(path=str(out / "01_home.png"))

    page.locator("nav button", has_text="Książki").click()
    page.wait_for_timeout(1200)
    page.screenshot(path=str(out / "02_library.png"))

    page.locator("library-panel .tool", has_text="Okładka").first.click()
    page.wait_for_timeout(500)
    page.locator("cover-editor input[type=file]").set_input_files(str(photo))
    page.wait_for_timeout(800)
    frame = page.locator("cover-editor .frame")
    box = frame.bounding_box()
    page.mouse.move(box["x"] + box["width"] / 2, box["y"] + box["height"] / 2)
    page.mouse.down()
    page.mouse.move(box["x"] + box["width"] / 2 + 30, box["y"] + box["height"] / 2 + 10, steps=5)
    page.mouse.up()
    page.screenshot(path=str(out / "03_cover_crop.png"))

    page.get_by_role("button", name="Dalej: grzbiet").click()
    page.wait_for_timeout(300)
    page.get_by_role("button", name="Użyj zdjęcia okładki").click()
    page.wait_for_timeout(500)
    page.screenshot(path=str(out / "04_spine_crop.png"))

    page.get_by_role("button", name="Dalej: podgląd").click()
    page.wait_for_timeout(800)
    page.screenshot(path=str(out / "05_preview.png"))
    page.get_by_role("button", name="Wyślij na czytnik").click()
    page.wait_for_timeout(2500)
    page.screenshot(path=str(out / "06_library_after.png"))

    page.locator("library-panel .tool", has_text="Rozdziały").first.click()
    page.wait_for_timeout(2500)
    page.screenshot(path=str(out / "07_chapters.png"))
    page.locator("chapter-editor .remove").first.click()
    page.locator("chapter-editor .remove").first.click()
    page.wait_for_timeout(300)
    page.screenshot(path=str(out / "07b_chapters_removed.png"))
    page.get_by_role("button", name="Dodaj wszystkie").click()
    page.wait_for_timeout(400)
    page.screenshot(path=str(out / "08_chapters_added.png"))
    page.locator("chapter-editor .mark input").first.fill("Początek")
    page.get_by_role("button", name="Zapisz (12)").click()
    page.wait_for_timeout(800)
    page.screenshot(path=str(out / "09_chapters_saved.png"))
    browser.close()

server.shutdown()
print("errors:", errors if errors else "none")
