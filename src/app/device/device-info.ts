/**
 * Dane o czytniku, które appka i tak dostaje przy okazji zwykłej komunikacji
 * (wifi-link.ts) — reszta appki (np. ekran Aktualizacje, nagłówek appki)
 * czyta je stąd zamiast robić własny request.
 */

let firmwareVersion: string | null = null;
const fwListeners = new Set<(version: string | null) => void>();

export function setFirmwareVersion(version: string | null): void {
  if (version === firmwareVersion) return;
  firmwareVersion = version;
  for (const l of fwListeners) l(firmwareVersion);
}

export function getFirmwareVersion(): string | null {
  return firmwareVersion;
}

export function onFirmwareVersionChange(handler: (version: string | null) => void): () => void {
  fwListeners.add(handler);
  return () => fwListeners.delete(handler);
}

/** Procent baterii czytnika — z /api/info, odświeżane co jakiś czas gdy połączony. */
let batteryPercent: number | null = null;
const batteryListeners = new Set<(percent: number | null) => void>();

export function setBatteryPercent(percent: number | null): void {
  if (percent === batteryPercent) return;
  batteryPercent = percent;
  for (const l of batteryListeners) l(batteryPercent);
}

export function getBatteryPercent(): number | null {
  return batteryPercent;
}

export function onBatteryPercentChange(handler: (percent: number | null) => void): () => void {
  batteryListeners.add(handler);
  return () => batteryListeners.delete(handler);
}
