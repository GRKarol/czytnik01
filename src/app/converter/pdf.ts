/**
 * PDF → .rsvp przez pdfjs. Każda strona = paragraf; pdfjs zwraca text
 * items per page, sklejamy je z heurystyką: nowy item w nowej linii →
 * spacja, w nowej kolumnie → nowy paragraf.
 *
 * Pdfjs worker pochodzi z paczki (`pdfjs-dist/build/pdf.worker.min.mjs`).
 * Vite z `?url` zwraca URL do zbundlowanej kopii.
 */

import * as pdfjs from "pdfjs-dist";
import workerSrc from "pdfjs-dist/build/pdf.worker.min.mjs?url";
import type { BookEvent, ParsedBook } from "./rsvp";
import { looksLikeChapterHeading } from "./text-formats";

pdfjs.GlobalWorkerOptions.workerSrc = workerSrc as string;

/**
 * PDF nie ma znaczników `<hN>` — rozdziały trzeba znaleźć inaczej.
 * Najpewniejsze źródło to prawdziwy spis treści (outline/zakładki), jeśli
 * autor go osadził w pliku: mapujemy go na numer strony, na której zaczyna
 * się każdy wpis. Gdy PDF nie ma outline'u, wracamy do heurystyki z
 * pierwszej linii strony (ten sam detektor co dla .txt).
 */
async function buildChapterPageMap(
  pdf: Awaited<ReturnType<typeof pdfjs.getDocument>["promise"]>,
): Promise<Map<number, string>> {
  const map = new Map<number, string>();
  try {
    const outline = await pdf.getOutline();
    if (!outline?.length) return map;
    const flat: { title: string; dest: unknown }[] = [];
    const collect = (items: typeof outline) => {
      for (const item of items) {
        if (item.title && item.dest) flat.push({ title: item.title, dest: item.dest });
        if (item.items?.length) collect(item.items);
      }
    };
    collect(outline);

    for (const entry of flat) {
      try {
        const dest =
          typeof entry.dest === "string" ? await pdf.getDestination(entry.dest) : entry.dest;
        const ref = Array.isArray(dest) ? dest[0] : null;
        if (!ref) continue;
        const pageIndex = await pdf.getPageIndex(ref);
        const title = entry.title.replace(/\s+/g, " ").trim();
        if (title && !map.has(pageIndex)) map.set(pageIndex, title);
      } catch {
        // Zepsuty pojedynczy wpis outline'u — pomiń go, reszta i tak się przyda.
      }
    }
  } catch {
    // Brak/zepsuty outline — zostaje pusta mapa, parsePdf spadnie na heurystykę.
  }
  return map;
}

export async function parsePdf(file: File): Promise<ParsedBook> {
  const buf = new Uint8Array(await file.arrayBuffer());
  const pdf = await pdfjs.getDocument({ data: buf }).promise;

  const meta = await pdf.getMetadata().catch(() => null);
  const info = (meta?.info ?? {}) as Record<string, unknown>;
  const title = typeof info.Title === "string" ? info.Title : "";
  const author = typeof info.Author === "string" ? info.Author : "";

  const chapterPageMap = await buildChapterPageMap(pdf);
  const hasOutline = chapterPageMap.size > 0;

  const pages: string[][] = [];
  for (let pageNum = 1; pageNum <= pdf.numPages; pageNum++) {
    const page = await pdf.getPage(pageNum);
    const content = await page.getTextContent();
    pages.push(joinTextItems(content.items as Array<TextItemLike>).split("\n"));
  }
  dropRunningHeaders(pages);

  const events: BookEvent[] = [];
  let guessedChapters = 0;
  let textPages = 0;
  for (let pageIndex = 0; pageIndex < pages.length; pageIndex++) {
    const text = pages[pageIndex].join("\n");
    if (!text.trim()) continue;
    textPages++;

    const outlineTitle = chapterPageMap.get(pageIndex);
    if (outlineTitle) events.push({ kind: "chapter", text: outlineTitle });

    // Rozbij stronę na akapity po pustych liniach.
    let first = true;
    for (const para of text.split(/\n\s*\n+/)) {
      const t = para.replace(/\s+/g, " ").trim();
      if (!t) continue;
      // Bez outline'u sprawdź, czy pierwszy akapit strony wygląda jak
      // nagłówek rozdziału (np. "Rozdział 3" na osobnej linii). Sama liczba
      // to w PDF-ie prawie zawsze numer strony, nie rozdział.
      if (!hasOutline && first && !outlineTitle && !/^\d{1,4}\.?$/.test(t) && looksLikeChapterHeading(t)) {
        events.push({ kind: "chapter", text: t });
        guessedChapters++;
      } else {
        events.push({ kind: "paragraph", text: t });
      }
      first = false;
    }
  }

  // A "chapter" on every few pages is a page header the filter missed,
  // not a chapter: keep the text, drop the guesses.
  if (guessedChapters > Math.max(4, textPages / 3)) {
    for (let i = 0; i < events.length; i++) {
      if (events[i].kind === "chapter") events[i] = { kind: "paragraph", text: events[i].text };
    }
  }

  if (!events.length) {
    throw new Error(
      "Z tego PDF-a nie udało się wyciągnąć tekstu — to pewnie skan obrazów. OCR wymaga osobnej rundy.",
    );
  }

  return {
    metadata: {
      title: title || stripExt(file.name),
      author,
      source: file.name,
    },
    events,
  };
}

/**
 * Clears running headers and footers: page numbers and lines (book title,
 * author, chapter name) repeated at the top or bottom of many pages. Left
 * in, they came out as a "chapter" on nearly every page ("Dżuma": 186).
 */
function dropRunningHeaders(pages: string[][]): void {
  const edgeLines = (lines: string[]) => {
    const filled = lines.flatMap((l, i) => (l.trim() ? [i] : []));
    return [...new Set([...filled.slice(0, 2), ...filled.slice(-2)])];
  };
  const key = (line: string) => line.toLocaleLowerCase().replace(/[\d\s\-–—.·|]+/g, "");

  const seen = new Map<string, number>();
  for (const lines of pages) {
    const keys = new Set(edgeLines(lines).map((i) => key(lines[i])).filter(Boolean));
    for (const k of keys) seen.set(k, (seen.get(k) ?? 0) + 1);
  }
  const textPages = pages.filter((lines) => lines.some((l) => l.trim())).length;
  const repeatAt = Math.max(3, Math.ceil(textPages * 0.25));

  for (const lines of pages) {
    for (const i of edgeLines(lines)) {
      const line = lines[i].trim();
      const pageNumber = /^[-–—(]?\s*\d{1,4}\s*[-–—)]?$/.test(line);
      if (pageNumber || (seen.get(key(line)) ?? 0) >= repeatAt) lines[i] = "";
    }
  }
}

interface TextItemLike {
  str: string;
  hasEOL?: boolean;
  transform?: number[];
}

function joinTextItems(items: TextItemLike[]): string {
  let out = "";
  let lastY: number | null = null;
  for (const it of items) {
    const y = it.transform?.[5] ?? null;
    if (lastY !== null && y !== null && Math.abs(y - lastY) > 2) {
      out += "\n";
    }
    out += it.str;
    if (it.hasEOL) out += "\n";
    lastY = y;
  }
  return out;
}

function stripExt(name: string): string {
  return name.replace(/\.[^.]+$/, "");
}
