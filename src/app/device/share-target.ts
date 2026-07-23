/**
 * Odbieranie tekstu/linków udostępnionych z innych aplikacji (Android
 * "Udostępnij" → Flower), jak w rsvpnano. Poza Androidem (PWA/przeglądarka)
 * nie ma takiego mostka, więc wszystko tu jest no-opem.
 */

import { Capacitor, registerPlugin } from "@capacitor/core";
import type { PluginListenerHandle } from "@capacitor/core";

interface ShareTargetPlugin {
  getPending(): Promise<{ text: string | null }>;
  addListener(
    eventName: "shareReceived",
    listenerFunc: (data: { text: string }) => void,
  ): Promise<PluginListenerHandle>;
}

const ShareTarget = registerPlugin<ShareTargetPlugin>("ShareTarget");

/** Sprawdza czy appka została uruchomiona z zawisłym udostępnionym tekstem (cold start). */
export async function consumePendingSharedText(): Promise<string | null> {
  if (Capacitor.getPlatform() !== "android") return null;
  try {
    const { text } = await ShareTarget.getPending();
    return text?.trim() ? text : null;
  } catch (err) {
    console.warn("[share-target] getPending nie powiodło się:", err);
    return null;
  }
}

/** Nasłuchuje na udostępniony tekst gdy appka już działa (warm start / w tle). */
export function onSharedTextReceived(handler: (text: string) => void): () => void {
  if (Capacitor.getPlatform() !== "android") return () => {};
  const handlePromise = ShareTarget.addListener("shareReceived", (data) => {
    if (data.text?.trim()) handler(data.text);
  });
  return () => {
    void handlePromise.then((h) => h.remove());
  };
}
