/**
 * Wspólny model dla wszystkich parserów: każdy konwerter zwraca strumień
 * eventów `BookEvent` (rozdział lub paragraf tekstu), które writer
 * serializuje do pliku .rsvp.
 */

export interface BookMetadata {
  title: string;
  author: string;
  source: string;
}

export type BookEvent =
  | { kind: "chapter"; text: string }
  | { kind: "paragraph"; text: string };

export interface ParsedBook {
  metadata: BookMetadata;
  events: BookEvent[];
}

const RSVP_VERSION = "1";
const WRAP_WIDTH = 96;

function directiveSafe(s: string): string {
  return s.replace(/\s+/g, " ").trim();
}

/** Zawija tekst akapitu na linie ≤ WRAP_WIDTH, nie łamiąc słów. */
function wrap(text: string, width = WRAP_WIDTH): string[] {
  const words = text.split(/\s+/).filter(Boolean);
  const out: string[] = [];
  let line = "";
  for (const w of words) {
    if (!line) {
      line = w;
    } else if (line.length + 1 + w.length <= width) {
      line += " " + w;
    } else {
      out.push(line);
      line = w;
    }
  }
  if (line) out.push(line);
  return out;
}

/** Zamienia ParsedBook na string .rsvp gotowy do zapisu na pliku/SD. */
export function writeRsvp(book: ParsedBook): string {
  const { title, author, source } = book.metadata;
  const lines: string[] = [
    `@rsvp ${RSVP_VERSION}`,
    `@title ${directiveSafe(title || "Bez tytułu")}`,
  ];
  if (author) lines.push(`@author ${directiveSafe(author)}`);
  if (source) lines.push(`@source ${directiveSafe(source)}`);
  lines.push("");

  let chapterCount = 0;
  for (const ev of book.events) {
    if (ev.kind === "chapter") {
      chapterCount++;
      lines.push("");
      lines.push(`@chapter ${directiveSafe(ev.text)}`);
    } else {
      for (let chunk of wrap(directiveSafe(ev.text))) {
        if (chunk.startsWith("@")) chunk = "@" + chunk;
        lines.push(chunk);
      }
    }
  }

  if (chapterCount === 0) {
    // Wstaw pojedynczy rozdział po nagłówku, żeby firmware miał pierwsze
    // kotwiczne paragrafy do nawigacji.
    lines.splice(4, 0, `@chapter ${directiveSafe(title || "Bez tytułu")}`);
  }

  return lines.join("\n").trim() + "\n";
}
