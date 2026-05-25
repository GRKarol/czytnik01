import { defineConfig, type Plugin } from "vite";
import { VitePWA } from "vite-plugin-pwa";
import { resolve } from "node:path";
import { readFileSync } from "node:fs";

// vite-plugin-pwa wstrzykuje <link rel="manifest"> do *każdego* entry HTML.
// Flasher (/index.html) nie ma być PWA — usuwamy go po wygenerowaniu bundle'a.
function stripPwaFromFlasher(): Plugin {
  return {
    name: "strip-pwa-from-flasher",
    enforce: "post",
    apply: "build",
    generateBundle(_options, bundle) {
      for (const [fileName, asset] of Object.entries(bundle)) {
        if (asset.type !== "asset") continue;
        if (!fileName.endsWith("index.html")) continue;
        if (fileName === "app/index.html") continue;
        if (typeof asset.source !== "string") continue;
        asset.source = asset.source
          .replace(/\s*<link[^>]+rel="manifest"[^>]*>/g, "")
          .replace(/\s*<script[^>]+id="vite-plugin-pwa:register-sw"[^>]*>[\s\S]*?<\/script>/g, "");
      }
    },
  };
}

const pkg = JSON.parse(readFileSync(resolve(__dirname, "package.json"), "utf8"));
const repoBase = process.env.VITE_BASE ?? "/";

export default defineConfig({
  base: repoBase,
  define: {
    __APP_VERSION__: JSON.stringify(pkg.version),
  },
  build: {
    target: "es2022",
    sourcemap: true,
    rollupOptions: {
      input: {
        flasher: resolve(__dirname, "index.html"),
        app: resolve(__dirname, "app/index.html"),
      },
    },
  },
  server: {
    host: true,
    port: 5173,
  },
  plugins: [
    stripPwaFromFlasher(),
    VitePWA({
      strategies: "generateSW",
      registerType: "autoUpdate",
      injectRegister: false,
      scope: `${repoBase}app/`,
      base: `${repoBase}app/`,
      includeAssets: ["icons/*.png", "icons/*.svg"],
      manifest: {
        id: "/app/",
        name: "Flower",
        short_name: "Flower",
        description: "Aplikacja Flower — czytnik RSVP, biblioteka, konwerter, pluginy.",
        start_url: `${repoBase}app/`,
        scope: `${repoBase}app/`,
        display: "standalone",
        orientation: "portrait",
        background_color: "#e8f4fd",
        theme_color: "#2e8eff",
        lang: "pl",
        icons: [
          {
            src: "icons/icon-192.png",
            sizes: "192x192",
            type: "image/png",
            purpose: "any",
          },
          {
            src: "icons/icon-512.png",
            sizes: "512x512",
            type: "image/png",
            purpose: "any",
          },
          {
            src: "icons/icon-maskable-512.png",
            sizes: "512x512",
            type: "image/png",
            purpose: "maskable",
          },
        ],
      },
      workbox: {
        navigateFallback: `${repoBase}app/index.html`,
        navigateFallbackDenylist: [/^\/firmware/, /^\/$/, /^\/index\.html$/],
        globPatterns: ["**/*.{js,css,html,svg,png,webmanifest}"],
        runtimeCaching: [
          {
            urlPattern: ({ url }) => url.pathname.endsWith(".bin"),
            handler: "NetworkOnly",
          },
        ],
      },
      devOptions: {
        enabled: false,
      },
    }),
  ],
});
