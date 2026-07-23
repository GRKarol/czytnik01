/**
 * Rozdziały to po prostu eventy `{kind:"chapter"}` wplecione w strumień
 * paragrafów (zob. rsvp.ts). Auto-wykrywanie (epub.ts/text-formats.ts) czasem
 * się myli — te funkcje pozwalają użytkownikowi ręcznie skorygować podział
 * już po sparsowaniu, przed wysłaniem na czytnik.
 */

import type { BookEvent } from "./rsvp";

export interface ChapterView {
  /** Indeks w płaskiej tablicy `events`, pod którym leży ten event "chapter". */
  eventIndex: number;
  title: string;
  /** Indeksy (w `events`) paragrafów należących do tego rozdziału. */
  paragraphIndices: number[];
}

export interface ChaptersModel {
  /**
   * Paragrafy przed pierwszym znacznikiem rozdziału — typowe dla zwykłego
   * .txt (wklejony/udostępniony tekst) gdzie parser w ogóle nie wykrywa
   * rozdziałów (parseTxt w text-formats.ts: wszystko = paragrafy). Bez tego
   * pola taka książka nie miałaby w UI ŻADNEGO akapitu do podziału.
   */
  preamble: number[];
  chapters: ChapterView[];
}

/** Grupuje płaski strumień eventów na widok rozdziałów (do UI). */
export function splitIntoChapters(events: BookEvent[]): ChaptersModel {
  const preamble: number[] = [];
  const chapters: ChapterView[] = [];
  let current: ChapterView | null = null;
  events.forEach((ev, i) => {
    if (ev.kind === "chapter") {
      current = { eventIndex: i, title: ev.text, paragraphIndices: [] };
      chapters.push(current);
    } else if (current) {
      current.paragraphIndices.push(i);
    } else {
      preamble.push(i);
    }
  });
  return { preamble, chapters };
}

export function renameChapter(events: BookEvent[], eventIndex: number, title: string): BookEvent[] {
  const ev = events[eventIndex];
  if (!ev || ev.kind !== "chapter") return events;
  const trimmed = title.trim();
  if (!trimmed) return events;
  const next = events.slice();
  next[eventIndex] = { kind: "chapter", text: trimmed };
  return next;
}

/** Usuwa znacznik rozdziału — jego paragrafy dołączają do poprzedniego rozdziału. */
export function mergeChapterUp(events: BookEvent[], eventIndex: number): BookEvent[] {
  const ev = events[eventIndex];
  if (!ev || ev.kind !== "chapter") return events;
  const next = events.slice();
  next.splice(eventIndex, 1);
  return next;
}

/** Wstawia nowy podział rozdziału tuż przed wskazanym paragrafem. */
export function splitChapterBeforeParagraph(
  events: BookEvent[],
  paragraphEventIndex: number,
  title: string,
): BookEvent[] {
  const ev = events[paragraphEventIndex];
  if (!ev || ev.kind !== "paragraph") return events;
  const trimmed = title.trim();
  if (!trimmed) return events;
  const next = events.slice();
  next.splice(paragraphEventIndex, 0, { kind: "chapter", text: trimmed });
  return next;
}

/** Krótki podgląd tekstu paragrafu do listy w UI. */
export function previewText(text: string, maxLen = 90): string {
  const clean = text.replace(/\s+/g, " ").trim();
  return clean.length > maxLen ? clean.slice(0, maxLen).trim() + "…" : clean || "(pusty akapit)";
}
