/**
 * Shim dla `virtual:pwa-register` używany tylko w buildzie Capacitora
 * (vite.capacitor.config.ts nie ładuje vite-plugin-pwa — appka natywna nie
 * jest kontekstem instalacji PWA i nie potrzebuje własnego service workera).
 */
export function registerSW(_opts?: unknown): void {
  // no-op
}
