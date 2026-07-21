/**
 * Maps firmware ui_lang NVS index (0–5) to SupportedLang code,
 * and maps the device API's Language string to SupportedLang.
 *
 * Firmware NVS mapping:
 *   0 = en, 1 = es, 2 = fr, 3 = de, 4 = ro, 5 = pl
 *
 * This is the single source of truth for that mapping — device/http-api.ts
 * imports UI_LANG_INDEX_MAP directly instead of keeping its own copy (it
 * used to have a separate, incorrect hardcoded array that didn't match
 * this one, which was the cause of wrong language sync with the device).
 */

import type { SupportedLang } from "./index";

/**
 * Mapping from firmware ui_lang NVS index to SupportedLang.
 */
export const UI_LANG_INDEX_MAP: Record<number, SupportedLang> = {
  0: "en",
  1: "es",
  2: "fr",
  3: "de",
  4: "ro",
  5: "pl",
};

/**
 * Mapping from device API Language string to SupportedLang.
 * The device API Language type is now exactly SupportedLang, so this is
 * an identity map — kept as a function for callers that only have a
 * loose string (e.g. from JSON) rather than the typed union.
 */
const DEVICE_LANG_TO_SUPPORTED: Record<string, SupportedLang> = {
  en: "en",
  es: "es",
  fr: "fr",
  de: "de",
  pl: "pl",
  ro: "ro",
};

/**
 * Convert a firmware ui_lang NVS index (0–5) to a SupportedLang.
 * Invalid indices default to Polish.
 */
export function indexToLang(index: number): SupportedLang {
  return UI_LANG_INDEX_MAP[index] ?? "pl";
}

/**
 * Convert a device API Language string to a SupportedLang.
 * Unsupported or unknown codes default to Polish.
 */
export function deviceLangToSupported(lang: string): SupportedLang {
  return DEVICE_LANG_TO_SUPPORTED[lang] ?? "pl";
}
