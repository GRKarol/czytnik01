import type { ParsedBook } from "./rsvp";
import { writeRsvp } from "./rsvp";
import { parseTxt, parseMarkdown, parseHtml } from "./text-formats";
import { parseEpub } from "./epub";

export type SupportedFormat = "txt" | "md" | "html" | "epub";
export type PlannedFormat = "pdf" | "mobi" | "azw" | "azw3";

const SUPPORTED_EXT: Record<string, SupportedFormat> = {
  txt: "txt",
  text: "txt",
  md: "md",
  markdown: "md",
  mdown: "md",
  html: "html",
  htm: "html",
  xhtml: "html",
  epub: "epub",
};

const PLANNED_EXT: Record<string, PlannedFormat> = {
  pdf: "pdf",
  mobi: "mobi",
  azw: "azw",
  azw3: "azw3",
};

export interface DetectionResult {
  kind: "supported" | "planned" | "unknown";
  format?: SupportedFormat | PlannedFormat;
}

export function detectFormat(file: File): DetectionResult {
  const ext = (file.name.split(".").pop() ?? "").toLowerCase();
  if (ext in SUPPORTED_EXT) return { kind: "supported", format: SUPPORTED_EXT[ext] };
  if (ext in PLANNED_EXT) return { kind: "planned", format: PLANNED_EXT[ext] };
  return { kind: "unknown" };
}

export async function parseFile(file: File, format: SupportedFormat): Promise<ParsedBook> {
  switch (format) {
    case "txt":
      return parseTxt(file);
    case "md":
      return parseMarkdown(file);
    case "html":
      return parseHtml(file);
    case "epub":
      return parseEpub(file);
  }
}

export { writeRsvp };
export type { ParsedBook, BookEvent, BookMetadata } from "./rsvp";
