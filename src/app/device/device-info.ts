/**
 * Wersja firmware faktycznie zainstalowana na czytniku — przychodzi w
 * odpowiedzi /api/hello (wifi-link.ts), którą i tak odpytujemy co 8s
 * (keep-alive) i przy każdym connect(). Reszta appki (np. ekran
 * Aktualizacje) czyta ją stąd zamiast robić własny request.
 */

let firmwareVersion: string | null = null;
const listeners = new Set<(version: string | null) => void>();

export function setFirmwareVersion(version: string | null): void {
  if (version === firmwareVersion) return;
  firmwareVersion = version;
  for (const l of listeners) l(firmwareVersion);
}

export function getFirmwareVersion(): string | null {
  return firmwareVersion;
}

export function onFirmwareVersionChange(handler: (version: string | null) => void): () => void {
  listeners.add(handler);
  return () => listeners.delete(handler);
}
