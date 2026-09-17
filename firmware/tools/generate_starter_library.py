#!/usr/bin/env python3
"""Builds the starter library shipped as GitHub Release assets.

Onboarding krok 2.4 ("Prawie gotowe! Co dzis czytamy?") pobiera po cichu do
5 tytulow domeny publicznej dla jezyka wybranego w kroku 1, patrz
App::bookDownloadTask (firmware/src/app/App.cpp). Ten skrypt generuje te
assety: pobiera zrodlowy plik (EPUB lub plain-text UTF-8) dla kazdej pozycji
w MANIFEST, konwertuje go do formatu .rsvp uzywajac tej samej logiki co
firmware/tools/sd_card_converter/convert_books.py (RsvpWriter, ekstrakcja
zdarzen z EPUB/HTML/tekstu), i zapisuje jako
"starter-<kod jezyka>-<1..5>.rsvp" w katalogu wyjsciowym.

Rozszerzenie MUSI byc .rsvp — StorageManager na urzadzeniu czyta dyrektywy
@title/@author/@chapter tylko z plikow .rsvp (patrz hasRsvpExtension() w
firmware/src/storage/StorageManager.cpp).

Uzycie (jak w build-fonts.yml/generate_font_pack.sh):
    python3 firmware/tools/generate_starter_library.py dist/books
"""

from __future__ import annotations

import pathlib
import re
import sys
import tempfile
import time
import unicodedata
import urllib.error
import urllib.request

# Book titles/authors contain non-ASCII characters (accents, Polish/Romanian
# diacritics). CI runners default to UTF-8 stdout, but a local Windows
# console often doesn't — reconfigure so progress prints never crash the
# whole run over a console-encoding mismatch.
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent / "sd_card_converter"))
import convert_books as cb  # noqa: E402  (path must be set up first)

MAX_WORDS = 0  # unlimited, same default as convert_books.py
USER_AGENT = (
    "Mozilla/5.0 (compatible; czytnik01-starter-library-builder/1.0; "
    "+https://github.com/GRKarol/czytnik01)"
)
REQUEST_TIMEOUT_S = 30
RETRY_COUNT = 3
RETRY_DELAY_S = 4

GUTENBERG_START_RE = re.compile(
    r"\*\*\*\s*START OF (THE|THIS) PROJECT GUTENBERG EBOOK.*?\*\*\*", re.IGNORECASE | re.DOTALL
)
GUTENBERG_END_RE = re.compile(
    r"\*\*\*\s*END OF (THE|THIS) PROJECT GUTENBERG EBOOK", re.IGNORECASE
)

# Per-language chapter-heading keywords for plain-text (.txt) sources, mirroring
# the on-device keyword list StorageManager uses for un-annotated .txt books
# (firmware/src/storage/StorageManager.cpp) so generated chapters line up with
# what a human would expect to see for that language. Only used for the .txt
# fallback path — EPUB sources get real chapter markers from HTML headings.
CHAPTER_KEYWORDS = {
    "en": ("chapter", "part", "book"),
    "pl": ("rozdzial", "czesc"),
    "de": ("kapitel", "teil"),
    "fr": ("chapitre", "partie"),
    "es": ("capitulo",),
    "ro": ("capitolul", "partea"),
}


def strip_diacritics(text: str) -> str:
    decomposed = unicodedata.normalize("NFKD", text)
    return "".join(ch for ch in decomposed if not unicodedata.combining(ch))


def looks_like_chapter_lang(line: str, lang: str) -> str | None:
    trimmed = cb.clean_text(line)
    if not trimmed or len(trimmed) > 64:
        return None
    if trimmed.startswith("#"):
        title = trimmed.lstrip("#").strip()
        return title or None
    keywords = CHAPTER_KEYWORDS.get(lang, CHAPTER_KEYWORDS["en"])
    ascii_trimmed = strip_diacritics(trimmed).lower()
    for keyword in keywords:
        if re.match(rf"^{re.escape(keyword)}\b", ascii_trimmed):
            return trimmed
    return None


def text_events_lang(text: str, lang: str) -> list[tuple[str, str]]:
    events: list[tuple[str, str]] = []
    paragraph_parts: list[str] = []

    def flush_paragraph() -> None:
        if paragraph_parts:
            events.append(("text", cb.clean_text(" ".join(paragraph_parts))))
            paragraph_parts.clear()

    for raw_line in text.splitlines():
        line = raw_line.strip()
        chapter = looks_like_chapter_lang(line, lang)
        if chapter:
            flush_paragraph()
            events.append(("chapter", chapter))
            continue
        if not line:
            flush_paragraph()
            continue
        paragraph_parts.append(line)

    flush_paragraph()
    return events


def drop_empty_chapter_runs(events: list[tuple[str, str]]) -> list[tuple[str, str]]:
    """Collapses back-to-back chapter markers with no text between them to
    just the last one. Plain-text Gutenberg sources often print a table of
    contents ("Chapter I. The Cyclone" / "Chapter II. ..." / ...) ahead of the
    real story; looks_like_chapter_lang() matches each TOC line the same way
    it matches the real heading later, producing a run of empty chapters."""
    result: list[tuple[str, str]] = []
    for event in events:
        if (
            event[0] == "chapter"
            and result
            and result[-1][0] == "chapter"
        ):
            result[-1] = event
        else:
            result.append(event)
    return result


def strip_gutenberg_boilerplate(text: str) -> str:
    start_match = GUTENBERG_START_RE.search(text)
    if start_match:
        text = text[start_match.end():]
    end_match = GUTENBERG_END_RE.search(text)
    if end_match:
        text = text[: end_match.start()]
    return text.strip()


def fetch(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    last_error: Exception | None = None
    for attempt in range(1, RETRY_COUNT + 1):
        try:
            with urllib.request.urlopen(request, timeout=REQUEST_TIMEOUT_S) as response:
                return response.read()
        except (urllib.error.URLError, TimeoutError) as exc:
            last_error = exc
            print(f"  fetch attempt {attempt}/{RETRY_COUNT} failed: {exc}")
            if attempt < RETRY_COUNT:
                time.sleep(RETRY_DELAY_S)
    raise RuntimeError(f"could not fetch {url}: {last_error}")


# Gutenberg wraps every EPUB (regardless of the book's own language) in the
# same English legal boilerplate as separate spine documents: title page,
# table of contents, "produced by"/transcriber notes, and a huge license
# block at the end. events_for_file()/epub_events_and_metadata() turn each
# into its own ("chapter", ...) section, which would otherwise show up as
# junk chapters ahead of/after the real story. Marker text is always English,
# so this is safe to apply to sources in any of the 6 UI languages.
BOILERPLATE_HEADING_MARKERS = (
    "gutenberg",
    "license",
    "transcriber",
    "produced by",
    "illustrations",
    "colophon",
    "title page",
    "frontispiece",
    "table of contents",
    "preface",
    "introduction",
)
BOILERPLATE_MIN_WORDS = 60
# Some editions front-load a long run of short sections before the real text
# starts (title page + dedication + a table-of-contents block where every
# entry is its own short HTML heading, e.g. Pan Tadeusz's ~14-section front
# matter) — cap generous enough to clear those, while _section_is_boilerplate's
# word-count/heading checks still stop at the first substantial section, so a
# book that's genuinely short throughout (a poetry collection) isn't gutted.
BOILERPLATE_MAX_TRIM_SECTIONS = 20


def _split_into_sections(
    events: list[tuple[str, str]]
) -> list[tuple[str | None, list[str]]]:
    sections: list[tuple[str | None, list[str]]] = []
    current_title: str | None = None
    current_texts: list[str] = []
    for kind, value in events:
        if kind == "chapter":
            sections.append((current_title, current_texts))
            current_title = value
            current_texts = []
        else:
            current_texts.append(value)
    sections.append((current_title, current_texts))
    return [s for s in sections if s[0] is not None or s[1]]


def _section_is_boilerplate(title: str | None, texts: list[str]) -> bool:
    if title is not None:
        lowered = title.lower()
        if any(marker in lowered for marker in BOILERPLATE_HEADING_MARKERS):
            return True
    word_count = sum(len(t.split()) for t in texts)
    return word_count < BOILERPLATE_MIN_WORDS


def trim_boilerplate_sections(events: list[tuple[str, str]]) -> list[tuple[str, str]]:
    sections = _split_into_sections(events)
    if len(sections) <= 1:
        return events

    start = 0
    while (
        start < len(sections) - 1
        and start < BOILERPLATE_MAX_TRIM_SECTIONS
        and _section_is_boilerplate(*sections[start])
    ):
        start += 1

    end = len(sections)
    trimmed_from_end = 0
    while (
        end > start + 1
        and trimmed_from_end < BOILERPLATE_MAX_TRIM_SECTIONS
        and _section_is_boilerplate(*sections[end - 1])
    ):
        end -= 1
        trimmed_from_end += 1

    kept_events: list[tuple[str, str]] = []
    for title, texts in sections[start:end]:
        if title is not None:
            kept_events.append(("chapter", title))
        for text in texts:
            kept_events.append(("text", text))
    return kept_events


# Some Gutenberg epubs append the license footer to the SAME spine file as
# the final chapter (no separate heading), so section-level trimming above
# can't isolate it. This phrase essentially never occurs in 19th/20th-century
# prose, so treat its first appearance as "everything from here is boilerplate"
# and cut the stream there.
BOILERPLATE_INLINE_MARKERS = ("project gutenberg", "gutenberg-tm", "gutenberg™")


def truncate_at_first_inline_boilerplate(
    events: list[tuple[str, str]]
) -> list[tuple[str, str]]:
    result: list[tuple[str, str]] = []
    for kind, value in events:
        if kind == "text" and any(m in value.lower() for m in BOILERPLATE_INLINE_MARKERS):
            break
        result.append((kind, value))
    return result


def build_one(entry: dict, tmp_dir: pathlib.Path, out_dir: pathlib.Path) -> None:
    lang = entry["lang"]
    slot = entry["slot"]
    url = entry["url"]
    kind = entry["kind"]
    title = entry["title"]
    author = entry["author"]

    print(f"[{lang}-{slot}] {title} / {author} <- {url}")
    data = fetch(url)

    if kind == "epub":
        src_path = tmp_dir / f"src-{lang}-{slot}.epub"
        src_path.write_bytes(data)
        _, extracted_author, events = cb.epub_events_and_metadata(src_path)
        author = author or extracted_author
        events = trim_boilerplate_sections(events)
        events = truncate_at_first_inline_boilerplate(events)
    elif kind == "txt":
        text = data.decode("utf-8-sig", errors="replace")
        text = strip_gutenberg_boilerplate(text)
        events = text_events_lang(text, lang)
        events = drop_empty_chapter_runs(events)
    else:
        raise ValueError(f"unknown source kind: {kind}")

    writer = cb.RsvpWriter(title=title, author=author, source=url, max_words=MAX_WORDS)
    for event_kind, value in events:
        if event_kind == "chapter":
            writer.add_chapter(value)
            continue
        writer.begin_paragraph()
        if not writer.add_text(value):
            break

    if writer.word_count < 200:
        raise RuntimeError(
            f"[{lang}-{slot}] suspiciously short output ({writer.word_count} words) — "
            "source likely didn't parse as expected"
        )

    out_path = out_dir / f"starter-{lang}-{slot}.rsvp"
    writer.write_to(out_path, fallback_chapter=title)
    print(
        f"  -> {out_path.name} "
        f"({writer.word_count} words, {writer.chapter_count} chapters)"
    )


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <output-dir>", file=sys.stderr)
        return 2

    out_dir = pathlib.Path(sys.argv[1]).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    failures: list[str] = []
    with tempfile.TemporaryDirectory(prefix="starter-library-") as tmp:
        tmp_dir = pathlib.Path(tmp)
        for entry in MANIFEST:
            try:
                build_one(entry, tmp_dir, out_dir)
            except Exception as exc:  # noqa: BLE001 - report and continue, don't fail the whole pack
                label = f"{entry['lang']}-{entry['slot']}"
                failures.append(f"{label}: {exc}")
                print(f"[{label}] FAILED: {exc}")

    total = len(MANIFEST)
    ok = total - len(failures)
    print(f"\nBuilt {ok}/{total} starter books")
    if failures:
        print("Failures:")
        for line in failures:
            print(f"  - {line}")
    # A partial pack is still useful (missing assets are skipped silently by
    # the firmware, see startBackgroundBookDownload's comment in App.cpp) —
    # only fail the build if we produced dangerously few books overall.
    if ok < total // 2:
        return 1
    return 0


def _gutenberg(book_id: int) -> str:
    # .epub.noimages: same text content as the full epub, without cover/illustration
    # images we'd throw away anyway (RsvpWriter only extracts text) — much
    # smaller and faster to fetch/parse in CI.
    return f"https://www.gutenberg.org/ebooks/{book_id}.epub.noimages"


# 5 genre-varied, public-domain, originally-in-that-language books per UI
# language (see firmware/src/app/Localization.h for the language list).
# Sourced from Project Gutenberg; Polish additionally uses Wolne Lektury
# where Gutenberg's Polish shelf was too thin for genre variety. Every URL
# below was verified live (HTTP 200, application/epub+zip or a direct
# Wolne Lektury .epub) before being added here.
#
# Romanian is intentionally short (3, not 5): Project Gutenberg's entire
# Romanian catalog is 5 books total, of which 2 are disqualified (one is a
# 21st-century copyrighted pamphlet, one is a translation rather than an
# originally-Romanian work) and no equivalent of Wolne Lektury exists for
# Romanian with verifiable direct-download URLs. A short starter pack is not
# an error — see startBackgroundBookDownload()'s comment in App.cpp, missing
# slots are simply skipped and krok 2.4 shows fewer tiles for that language.
MANIFEST: list[dict] = [
    # --- English (en) ---
    {"lang": "en", "slot": 1, "kind": "epub", "url": _gutenberg(1342),
     "title": "Pride and Prejudice", "author": "Jane Austen"},
    {"lang": "en", "slot": 2, "kind": "epub", "url": _gutenberg(345),
     "title": "Dracula", "author": "Bram Stoker"},
    {"lang": "en", "slot": 3, "kind": "epub", "url": _gutenberg(1661),
     "title": "The Adventures of Sherlock Holmes", "author": "Arthur Conan Doyle"},
    {"lang": "en", "slot": 4, "kind": "epub", "url": _gutenberg(120),
     "title": "Treasure Island", "author": "Robert Louis Stevenson"},
    {"lang": "en", "slot": 5, "kind": "epub", "url": _gutenberg(35),
     "title": "The Time Machine", "author": "H. G. Wells"},

    # --- French (fr) ---
    {"lang": "fr", "slot": 1, "kind": "epub", "url": _gutenberg(13951),
     "title": "Les trois mousquetaires", "author": "Alexandre Dumas"},
    {"lang": "fr", "slot": 2, "kind": "epub", "url": _gutenberg(32854),
     "title": "Arsène Lupin, gentleman-cambrioleur", "author": "Maurice Leblanc"},
    {"lang": "fr", "slot": 3, "kind": "epub", "url": _gutenberg(62215),
     "title": "Le Fantôme de l'Opéra", "author": "Gaston Leroux"},
    {"lang": "fr", "slot": 4, "kind": "epub", "url": _gutenberg(2419),
     "title": "La dame aux camélias", "author": "Alexandre Dumas fils"},
    {"lang": "fr", "slot": 5, "kind": "epub", "url": _gutenberg(4791),
     "title": "Voyage au centre de la terre", "author": "Jules Verne"},

    # --- German (de) ---
    {"lang": "de", "slot": 1, "kind": "epub", "url": _gutenberg(35312),
     "title": "Aus dem Leben eines Taugenichts", "author": "Joseph von Eichendorff"},
    {"lang": "de", "slot": 2, "kind": "epub", "url": _gutenberg(50285),
     "title": "Dr. Mabuse, der Spieler", "author": "Norbert Jacques"},
    {"lang": "de", "slot": 3, "kind": "epub", "url": _gutenberg(76360),
     "title": "Die Elixiere des Teufels", "author": "E. T. A. Hoffmann"},
    {"lang": "de", "slot": 4, "kind": "epub", "url": _gutenberg(31538),
     "title": "Peter Schlemihls wundersame Geschichte", "author": "Adelbert von Chamisso"},
    {"lang": "de", "slot": 5, "kind": "epub", "url": _gutenberg(2229),
     "title": "Faust: Der Tragödie erster Teil", "author": "Johann Wolfgang von Goethe"},

    # --- Spanish (es) ---
    {"lang": "es", "slot": 1, "kind": "epub", "url": _gutenberg(2000),
     "title": "Don Quijote de la Mancha", "author": "Miguel de Cervantes"},
    {"lang": "es", "slot": 2, "kind": "epub", "url": _gutenberg(55514),
     "title": "Cuentos de amor", "author": "Emilia Pardo Bazán"},
    {"lang": "es", "slot": 3, "kind": "epub", "url": _gutenberg(49836),
     "title": "Niebla", "author": "Miguel de Unamuno"},
    {"lang": "es", "slot": 4, "kind": "epub", "url": _gutenberg(13507),
     "title": "Cuentos de Amor de Locura y de Muerte", "author": "Horacio Quiroga"},
    {"lang": "es", "slot": 5, "kind": "epub", "url": _gutenberg(53743),
     "title": "El misterio de un hombre pequeñito", "author": "Eduardo Zamacois"},

    # --- Romanian (ro) — only 3/5, see note above ---
    {"lang": "ro", "slot": 1, "kind": "epub", "url": _gutenberg(64597),
     "title": "Nuvele", "author": "Ion Luca Caragiale"},
    {"lang": "ro", "slot": 2, "kind": "epub", "url": _gutenberg(35323),
     "title": "Poezii", "author": "Mihai Eminescu"},
    {"lang": "ro", "slot": 3, "kind": "epub", "url": _gutenberg(62916),
     "title": "Povești", "author": "Ioan Slavici"},

    # --- Polish (pl) ---
    {"lang": "pl", "slot": 1, "kind": "epub", "url": _gutenberg(8119),
     "title": "Sklepy cynamonowe", "author": "Bruno Schulz"},
    {"lang": "pl", "slot": 2, "kind": "epub", "url": _gutenberg(31536),
     "title": "Pan Tadeusz", "author": "Adam Mickiewicz"},
    {"lang": "pl", "slot": 3, "kind": "epub",
     "url": "https://wolnelektury.pl/media/book/epub/dolega-mostowicz-prokurator-alicja-horn.epub",
     "title": "Prokurator Alicja Horn", "author": "Tadeusz Dołęga-Mostowicz"},
    {"lang": "pl", "slot": 4, "kind": "epub",
     "url": "https://wolnelektury.pl/media/book/epub/demon-ruchu.epub",
     "title": "Demon ruchu", "author": "Stefan Grabiński"},
    {"lang": "pl", "slot": 5, "kind": "epub", "url": _gutenberg(34635),
     "title": "Menażerya ludzka", "author": "Gabriela Zapolska"},
]


if __name__ == "__main__":
    raise SystemExit(main())
