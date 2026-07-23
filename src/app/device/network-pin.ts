/**
 * Wrapper na natywny plugin `NetworkPin` (android/app/.../NetworkPinPlugin.java).
 * Android potrafi w tle przełączyć ruch appki z powrotem na sieć z internetem,
 * mimo że telefon fizycznie nadal jest połączony z AP czytnika (bo ta sieć
 * "nie ma internetu") — bindProcessToNetwork temu zapobiega. Poza Androidem
 * (PWA/przeglądarka) nie ma takiego mostka, więc to zawsze no-op.
 */

import { Capacitor, registerPlugin } from "@capacitor/core";

interface NetworkPinPlugin {
  pin(): Promise<{ pinned: boolean }>;
  unpin(): Promise<{ unpinned: boolean }>;
}

const NetworkPin = registerPlugin<NetworkPinPlugin>("NetworkPin");

export async function pinToDeviceNetwork(): Promise<void> {
  if (Capacitor.getPlatform() !== "android") return;
  try {
    await NetworkPin.pin();
  } catch (err) {
    console.warn("[network-pin] nie udało się przypiąć do sieci czytnika:", err);
  }
}

export async function unpinFromDeviceNetwork(): Promise<void> {
  if (Capacitor.getPlatform() !== "android") return;
  try {
    await NetworkPin.unpin();
  } catch (err) {
    console.warn("[network-pin] nie udało się odpiąć sieci:", err);
  }
}
