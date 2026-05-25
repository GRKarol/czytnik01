/**
 * Wspólna konfiguracja dla flashera i aplikacji klienta.
 */

const BASE = import.meta.env.BASE_URL.replace(/\/?$/, "/");

// ─── Branding ────────────────────────────────────────────────────────────────
export const BRAND_NAME = "Flower";
export const DEVICE_LABEL = "Czytnik"; // jak nazywamy urządzenie do klienta
export const DEVICE_CODENAME = "czytnik01"; // techniczna nazwa (firmware, repo)

// ─── URL-e ───────────────────────────────────────────────────────────────────
export const FIRMWARE_MANIFEST_URL = `${BASE}firmware/manifest.json`;
export const APP_URL = `${BASE}app/`;
export const FLASHER_URL = BASE;

// Sklep pluginów (statyczny katalog w naszym repo, ładowany przez app).
export const PLUGIN_INDEX_URL = `${BASE}plugins/index.json`;

// OTA — pobierane z GitHub Releases tego repo.
export const OTA_RELEASES_API = "https://api.github.com/repos/GRKarol/czytnik01/releases/latest";

// ─── Wersja ──────────────────────────────────────────────────────────────────
export const APP_VERSION = __APP_VERSION__;

// ─── Sieć / urządzenie ───────────────────────────────────────────────────────
// SSID prefix dla trybu AP urządzenia: "Flower-AB12CD"
export const DEVICE_AP_SSID_PREFIX = "Flower-";
// Domyślny adres urządzenia w trybie AP
export const DEVICE_AP_BASE_URL = "http://192.168.4.1";
// Web Bluetooth: identyfikator usługi GATT (placeholder do uzgodnienia z firmware)
export const DEVICE_BLE_SERVICE_UUID = "f10e7e10-f10e-7e10-f10e-7e10f10e7e10";
