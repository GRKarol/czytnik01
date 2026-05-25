/**
 * Wspólna konfiguracja dla flashera i aplikacji klienta.
 * Wartości BASE_URL pochodzą z Vite (import.meta.env.BASE_URL).
 */

const BASE = import.meta.env.BASE_URL.replace(/\/?$/, "/");

export const FIRMWARE_MANIFEST_URL = `${BASE}firmware/manifest.json`;
export const APP_URL = `${BASE}app/`;
export const FLASHER_URL = BASE;

export const APP_NAME = "Czytnik01";
export const APP_VERSION = __APP_VERSION__;
