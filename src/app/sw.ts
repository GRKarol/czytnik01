/// <reference lib="webworker" />
import { precacheAndRoute } from "workbox-precaching";
import { NavigationRoute, registerRoute } from "workbox-routing";
import { NetworkFirst, NetworkOnly } from "workbox-strategies";

declare let self: ServiceWorkerGlobalScope;

// Precache all assets from the build manifest
precacheAndRoute(self.__WB_MANIFEST);

// Navigation fallback: serve app/index.html for all navigations
const navigationHandler = new NetworkFirst({
  cacheName: "navigations",
});
registerRoute(
  new NavigationRoute(navigationHandler, {
    denylist: [/^\/firmware/, /^\/$/, /^\/index\.html$/],
  }),
);

// Don't cache .bin firmware files
registerRoute(({ url }) => url.pathname.endsWith(".bin"), new NetworkOnly());

// ─── Web Share Target handler ────────────────────────────────────────────────
// When Android sends a file via "Share → Flower", it POSTs to /app/share-receive.
// We intercept that POST here, extract the file from FormData, stash it in
// IndexedDB, then redirect the client to the app with a query param so the app
// knows to pick up the shared file.

const SHARE_DB_NAME = "flower-share";
const SHARE_STORE_NAME = "pending-files";

async function storeSharedFile(file: File): Promise<string> {
  const id = `share-${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(SHARE_DB_NAME, 1);
    request.onupgradeneeded = () => {
      const db = request.result;
      if (!db.objectStoreNames.contains(SHARE_STORE_NAME)) {
        db.createObjectStore(SHARE_STORE_NAME);
      }
    };
    request.onsuccess = () => {
      const db = request.result;
      const tx = db.transaction(SHARE_STORE_NAME, "readwrite");
      const store = tx.objectStore(SHARE_STORE_NAME);
      store.put({ file, name: file.name, size: file.size, timestamp: Date.now() }, id);
      tx.oncomplete = () => {
        db.close();
        resolve(id);
      };
      tx.onerror = () => {
        db.close();
        reject(tx.error);
      };
    };
    request.onerror = () => reject(request.error);
  });
}

self.addEventListener("fetch", (event: FetchEvent) => {
  const url = new URL(event.request.url);

  // Only intercept POST to our share-receive endpoint
  if (event.request.method !== "POST" || !url.pathname.endsWith("/share-receive")) {
    return;
  }

  event.respondWith(
    (async () => {
      try {
        const formData = await event.request.formData();
        const file = formData.get("file") as File | null;

        if (file && file.size > 0) {
          const id = await storeSharedFile(file);
          // Redirect to the app with the share ID so it can pick up the file
          return Response.redirect(`${url.origin}/app/?shared=${id}`, 303);
        }

        // No file — just redirect to app
        return Response.redirect(`${url.origin}/app/`, 303);
      } catch (err) {
        console.error("[sw] share-receive error:", err);
        return Response.redirect(`${url.origin}/app/`, 303);
      }
    })(),
  );
});
