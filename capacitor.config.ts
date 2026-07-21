import type { CapacitorConfig } from "@capacitor/cli";

const config: CapacitorConfig = {
  appId: "pl.flower.reader",
  appName: "Flower",
  webDir: "dist-capacitor",
  // Domyślnie Capacitor serwuje appkę pod wirtualnym https://localhost, co
  // sprawia że WebView traktuje ją jak zwykłą stronę HTTPS i blokuje
  // "Mixed Content" przy fetchu do http://192.168.4.1 — DOKŁADNIE ten sam
  // problem co w przeglądarce (patrz docs/roadmap.md, Faza 6). Zmiana na
  // http sprawia, że appka i czytnik gadają tym samym protokołem, więc
  // blokada nie ma zastosowania. Zweryfikowane na fizycznym telefonie.
  server: {
    androidScheme: "http",
  },
};

export default config;
