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

pdfjs.GlobalWorkerOptions.workerSrc = workerSrc as string;

export async function parsePdf(file: File): Promise<ParsedBook> {
  const buf = new Uint8Array(await file.arrayBuffer());
  const pdf = await pdfjs.getDocument({ data: buf }).promise;

  const meta = await pdf.getMetadata().catch(() => null);
  const info = (meta?.info ?? {}) as Record<string, unknown>;
  const title = typeof info.Title === "string" ? info.Title : "";
  const author = typeof info.Author === "string" ? info.Author : "";

  const events: BookEvent[] = [];
  for (let pageNum = 1; pageNum <= pdf.numPages; pageNum++) {
    const page = await pdf.getPage(pageNum);
    const content = await page.getTextContent();
    const text = joinTextItems(content.items as Array<TextItemLike>);
    if (!text.trim()) continue;
    // Rozbij stronę na akapity po pustych liniach.
    for (const para of text.split(/\n\s*\n+/)) {
      const t = para.replace(/\s+/g, " ").trim();
      if (t) events.push({ kind: "paragraph", text: t });
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
