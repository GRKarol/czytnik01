/**
 * Lightweight i18n module for the Flower reader PWA.
 *
 * - Flat key lookup from bundled JSON locale files
 * - Fallback chain: active locale → English → key string as-is
 * - Invalid language code → defaults to Polish (app default)
 * - Emits "lang-changed" CustomEvent on document for reactive updates
 */

// Static locale imports — bundled by Vite via resolveJsonModule.
// Locale files are created in subsequent tasks; empty objects act as safe defaults
// until real translations are provided.
import enLocale from "./locales/en.json";
import esLocale from "./locales/es.json";
import frLocale from "./locales/fr.json";
import deLocale from "./locales/de.json";
import roLocale from "./locales/ro.json";
import plLocale from "./locales/pl.json";

export type SupportedLang = "en" | "es" | "fr" | "de" | "ro" | "pl";

const SUPPORTED_LANGS: ReadonlySet<string> = new Set<SupportedLang>([
  "en",
  "es",
  "fr",
  "de",
  "ro",
  "pl",
]);

const DEFAULT_LANG: SupportedLang = "pl";

type LocaleMap = Record<string, string>;

const locales: Record<SupportedLang, LocaleMap> = {
  en: enLocale as LocaleMap,
  es: esLocale as LocaleMap,
  fr: frLocale as LocaleMap,
  de: deLocale as LocaleMap,
  ro: roLocale as LocaleMap,
  pl: plLocale as LocaleMap,
};

const LANG_STORAGE_KEY = "flower.lang";

/**
 * Check whether a given string is a valid SupportedLang.
 */
function isSupportedLang(lang: string): lang is SupportedLang {
  return SUPPORTED_LANGS.has(lang);
}

/**
 * Odczytuje język wybrany przy pierwszym uruchomieniu (onboarding-wizard)
 * albo ostatnio zsynchronizowany z czytnika — dzięki temu appka nie wraca
 * do polskiego przy każdym odświeżeniu, zanim ktoś połączy się z
 * urządzeniem.
 */
function loadPersistedLang(): SupportedLang {
  try {
    const raw = localStorage.getItem(LANG_STORAGE_KEY);
    if (raw && isSupportedLang(raw)) return raw;
  } catch {
    // localStorage niedostępny — zostań przy domyślnym.
  }
  return DEFAULT_LANG;
}

let currentLang: SupportedLang = loadPersistedLang();

/**
 * Translate a key using the current locale.
 * Fallback chain: active locale → English → key string as-is.
 * Empty key returns empty string.
 */
export function t(key: string): string {
  if (key === "") return "";

  const activeTranslations = locales[currentLang];
  if (activeTranslations && key in activeTranslations) {
    return activeTranslations[key];
  }

  // Fallback to English
  const enTranslations = locales.en;
  if (enTranslations && key in enTranslations) {
    return enTranslations[key];
  }

  // Final fallback: return key as-is
  return key;
}

/**
 * Set the active language. Invalid codes default to Polish.
 * Emits a "lang-changed" CustomEvent on document for reactive updates.
 */
export function setLang(lang: SupportedLang | string): void {
  const resolvedLang: SupportedLang = isSupportedLang(lang) ? lang : DEFAULT_LANG;
  currentLang = resolvedLang;
  try {
    localStorage.setItem(LANG_STORAGE_KEY, resolvedLang);
  } catch {
    // localStorage niedostępny — język nie przetrwa odświeżenia, ale appka działa.
  }
  document.dispatchEvent(new CustomEvent("lang-changed", { detail: { lang: resolvedLang } }));
}

/**
 * Get the currently active language.
 */
export function getLang(): SupportedLang {
  return currentLang;
}

/**
 * Subscribe to language changes. Returns an unsubscribe function.
 */
export function onLangChange(cb: () => void): () => void {
  document.addEventListener("lang-changed", cb);
  return () => {
    document.removeEventListener("lang-changed", cb);
  };
}
