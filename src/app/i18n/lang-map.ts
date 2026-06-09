/**
 * Maps firmware ui_lang NVS index (0–5) to SupportedLang code,
 * and maps the device API's Language string to SupportedLang.
 *
 * Firmware NVS mapping:
 *   0 = en, 1 = es, 2 = fr, 3 = de, 4 = ro, 5 = pl
 *
 * The device API Language type uses string codes that mostly overlap
 * with SupportedLang, except "it" (Italian) which isn't supported
 * by the i18n module — it falls back to Polish (app default).
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
 * Handles both the current app Language type and firmware-reported codes.
 * "it" (Italian) is not supported by the i18n module — defaults to "pl".
 */
const DEVICE_LANG_TO_SUPPORTED: Record<string, SupportedLang> = {
  en: "en",
  es: "es",
  fr: "fr",
  de: "de",
  pl: "pl",
  ro: "ro",
  it: "pl", // Italian not supported by i18n → fall back to Polish
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
