/**
 * Android only (Capacitor): keeps the app's requests on the reader's WiFi.
 *
 * The reader's own network (Flower-XXXX, 192.168.4.1) has no internet, so
 * Android routes the app's traffic over mobile data or another WiFi and
 * every request to the reader times out. NetworkPinPlugin.java binds the
 * app to the reader's network; on Android 10+ it can also ask the system to
 * join "Flower-…" directly, one tap in a system dialog, no trip to settings.
 *
 * While pinned the app has no internet, so anything that talks to GitHub
 * (update checks, firmware downloads) runs inside withInternet().
 * In a browser every function here is a no-op.
 */

import { Capacitor, registerPlugin } from "@capacitor/core";

interface NetworkPinPlugin {
  pin(): Promise<{ pinned: boolean }>;
  unpin(): Promise<{ unpinned: boolean }>;
  connectToReader(): Promise<{ connected: boolean; supported: boolean; replaced?: boolean }>;
  connectToNetwork(options: {
    ssid: string;
    password?: string;
  }): Promise<{ connected: boolean; supported: boolean; replaced?: boolean }>;
  releaseReader(): Promise<void>;
  openWifiSettings(): Promise<void>;
}

const NetworkPin = registerPlugin<NetworkPinPlugin>("NetworkPin");

let pinned = false;

export function isNativeApp(): boolean {
  return Capacitor.isNativePlatform();
}

/** Binds the app to the current WiFi (the reader's). False in a browser. */
export async function pinToReaderNetwork(): Promise<boolean> {
  if (!isNativeApp()) return false;
  try {
    pinned = (await NetworkPin.pin()).pinned;
  } catch {
    pinned = false;
  }
  return pinned;
}

export async function unpinReaderNetwork(): Promise<void> {
  if (!isNativeApp()) return;
  try {
    await NetworkPin.unpin();
  } catch {
    /* nothing to undo */
  }
  pinned = false;
}

/**
 * Android 10+: system dialog listing the reader's "Flower-…" network; the
 * app is bound to it once the user picks it. `supported: false` on older
 * Android and in a browser (the user joins the network in settings).
 */
export async function joinReaderNetwork(): Promise<{
  connected: boolean;
  supported: boolean;
  replaced?: boolean;
}> {
  if (!isNativeApp()) return { connected: false, supported: false };
  try {
    const result = await NetworkPin.connectToReader();
    pinned = result.connected;
    return result;
  } catch {
    return { connected: false, supported: false };
  }
}

/**
 * QR fallback: joins the exact network read from the reader's QR code.
 * A still-open "Flower-…" request from joinReaderNetwork() is cancelled
 * (it resolves with `replaced: true`).
 */
export async function joinNetworkFromQr(
  ssid: string,
  password: string,
): Promise<{ connected: boolean; supported: boolean }> {
  if (!isNativeApp()) return { connected: false, supported: false };
  try {
    const result = await NetworkPin.connectToNetwork({ ssid, password });
    pinned = result.connected;
    return result;
  } catch {
    return { connected: false, supported: false };
  }
}

/**
 * Network name (and password) from a code on the reader's screen: the
 * standard WiFi QR ("WIFI:T:nopass;S:Flower-3A7F;;") or the pairing link
 * ("flower://pair?t=…&n=Flower-3A7F", the name is also the network's).
 */
export function readerNetworkFromQr(text: string): { ssid: string; password: string } | null {
  const raw = text.trim();
  if (/^WIFI:/i.test(raw)) {
    const field = (key: string) => {
      // Values escape ; , : and \ with a backslash.
      const match = raw.match(new RegExp(`[:;]${key}:((?:\\\\.|[^;])*)`, "i"));
      return match ? match[1].replace(/\\(.)/g, "$1") : "";
    };
    const ssid = field("S");
    return ssid ? { ssid, password: field("P") } : null;
  }
  if (/^flower:\/\/pair/i.test(raw)) {
    const query = raw.slice(raw.indexOf("?") + 1);
    const name = new URLSearchParams(query).get("n") ?? "";
    return name ? { ssid: name, password: "" } : null;
  }
  return null;
}

/** Lets the phone leave the reader's network again. */
export async function releaseReaderNetwork(): Promise<void> {
  if (!isNativeApp()) return;
  try {
    await NetworkPin.releaseReader();
  } catch {
    /* already released */
  }
  pinned = false;
}

export async function openWifiSettings(): Promise<void> {
  if (isNativeApp()) {
    try {
      await NetworkPin.openWifiSettings();
      return;
    } catch {
      /* fall through to the intent URL */
    }
  }
  window.location.href = "intent:#Intent;action=android.settings.WIFI_SETTINGS;end";
}

/** Runs `task` with internet access (unpinned), pins back afterwards. */
export async function withInternet<T>(task: () => Promise<T>): Promise<T> {
  const wasPinned = pinned;
  if (wasPinned) await unpinReaderNetwork();
  try {
    return await task();
  } finally {
    if (wasPinned) await pinToReaderNetwork();
  }
}
