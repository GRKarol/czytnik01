import { defineConfig } from "vite";
import { resolve } from "node:path";
import { readFileSync } from "node:fs";

// Osobny build TYLKO appki klienta (do owinięcia w Capacitor/Android).
// Bez flashera (Web Serial nie ma sensu w natywnej appce na telefon) i bez
// vite-plugin-pwa (appka natywna nie jest kontekstem instalacji PWA — nie
// potrzebuje własnego service workera/manifestu, Capacitor już serwuje
// wszystko lokalnie). Wyjście: dist-capacitor/, wskazywane jako webDir
// w capacitor.config.ts.
const pkg = JSON.parse(readFileSync(resolve(__dirname, "package.json"), "utf8"));

export default defineConfig({
  root: resolve(__dirname, "app"),
  base: "/",
  publicDir: resolve(__dirname, "public"),
  define: {
    __APP_VERSION__: JSON.stringify(pkg.version),
  },
  resolve: {
    alias: {
      // main.ts jest współdzielony z buildem PWA, który ładuje ten
      // wirtualny moduł przez vite-plugin-pwa. Tu tego pluginu nie ma,
      // więc podstawiamy no-op zamiast dublować main.ts.
      "virtual:pwa-register": resolve(__dirname, "src/app/pwa-register-noop.ts"),
    },
  },
  build: {
    target: "es2022",
    sourcemap: true,
    outDir: resolve(__dirname, "dist-capacitor"),
    emptyOutDir: true,
  },
});
