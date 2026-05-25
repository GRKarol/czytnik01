import JSZip from "jszip";
import { extractEventsFromElement } from "./text-formats";
import type { BookEvent, ParsedBook } from "./rsvp";

/**
 * Port `firmware/tools/epub_to_rsvp.py` na TypeScript:
 * 1. Otwórz EPUB jako ZIP.
 * 2. `META-INF/container.xml` → ścieżka do `.opf`.
 * 3. `.opf` → tytuł, autor, lista spine itemów (XHTML w kolejności czytania).
 * 4. Dla każdego XHTML wyciągnij rozdziały (`<hN>`) i paragrafy.
 */
export async function parseEpub(file: File): Promise<ParsedBook> {
  const zip = await JSZip.loadAsync(await file.arrayBuffer());

  const containerXml = await readZipText(zip, "META-INF/container.xml");
  const opfPath = findRootfilePath(containerXml);
  if (!opfPath) {
    throw new Error("EPUB nie zawiera ścieżki do pliku OPF (uszkodzony plik?).");
  }

  const opfXml = await readZipText(zip, opfPath);
  const opfDoc = new DOMParser().parseFromString(opfXml, "application/xml");
  const title = firstLocalText(opfDoc, "title") || stripExt(file.name);
  const author = firstLocalText(opfDoc, "creator");

  const manifest = new Map<string, string>();
  for (const item of Array.from(opfDoc.getElementsByTagName("*"))) {
    if (localName(item) !== "item") continue;
    const id = item.getAttribute("id");
    const href = item.getAttribute("href");
    if (id && href) manifest.set(id, joinZipPath(opfPath, href));
  }

  const spinePaths: string[] = [];
  for (const ref of Array.from(opfDoc.getElementsByTagName("*"))) {
    if (localName(ref) !== "itemref") continue;
    const idref = ref.getAttribute("idref");
    if (!idref) continue;
    const path = manifest.get(idref);
    if (path && /\.(x?html?|htm)$/i.test(path)) spinePaths.push(path);
  }

  if (!spinePaths.length) {
    throw new Error("EPUB nie zawiera czytalnych dokumentów XHTML.");
  }

  const events: BookEvent[] = [];
  let i = 0;
  for (const path of spinePaths) {
    i++;
    const xhtml = await readZipText(zip, path);
    const doc = new DOMParser().parseFromString(xhtml, "application/xhtml+xml");
    // Niektóre EPUB-y mają błędy parserowe — fallback na text/html.
    const body =
      doc.querySelector("parsererror")
        ? new DOMParser().parseFromString(xhtml, "text/html").body
        : doc.querySelector("body");

    const chunk = extractEventsFromElement(body);
    if (!chunk.length || !chunk.some((e) => e.kind === "paragraph")) continue;
    if (!chunk.some((e) => e.kind === "chapter")) {
      chunk.unshift({ kind: "chapter", text: fallbackChapterTitle(path, i) });
    }
    events.push(...chunk);
  }

  return {
    metadata: { title, author, source: file.name },
    events,
  };
}

// ─── helpers ────────────────────────────────────────────────────────────────

async function readZipText(zip: JSZip, name: string): Promise<string> {
  const f = zip.file(name);
  if (!f) throw new Error(`Brak pliku w EPUB: ${name}`);
  return f.async("string");
}

function findRootfilePath(containerXml: string): string {
  const doc = new DOMParser().parseFromString(containerXml, "application/xml");
  for (const el of Array.from(doc.getElementsByTagName("*"))) {
    if (localName(el) === "rootfile") {
      return el.getAttribute("full-path") ?? "";
    }
  }
  return "";
}

function localName(el: Element): string {
  const t = el.tagName;
  const c = t.indexOf(":");
  return (c >= 0 ? t.slice(c + 1) : t).toLowerCase();
}

function firstLocalText(doc: Document, want: string): string {
  for (const el of Array.from(doc.getElementsByTagName("*"))) {
    if (localName(el) === want && el.textContent) {
      return el.textContent.replace(/\s+/g, " ").trim();
    }
  }
  return "";
}

function joinZipPath(base: string, href: string): string {
  const decoded = decodeURIComponent(href.split("#")[0]);
  const baseDir = base.replace(/[^/]+$/, "");
  const combined = baseDir + decoded;
  // Uprość ./ i ../
  const parts: string[] = [];
  for (const p of combined.split("/")) {
    if (p === "" || p === ".") continue;
    if (p === "..") parts.pop();
    else parts.push(p);
  }
  return parts.join("/");
}

function stripExt(name: string): string {
  return name.replace(/\.[^.]+$/, "");
}

function fallbackChapterTitle(path: string, index: number): string {
  const stem = path.replace(/^.*\//, "").replace(/\.[^.]+$/, "");
  const cleaned = stem.replace(/[_-]+/g, " ").trim();
  return cleaned || `Rozdział ${index}`;
}
