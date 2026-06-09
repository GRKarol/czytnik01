/**
 * Onboarding state management backed by localStorage.
 *
 * All keys use the `flower.onboarding.` prefix — existing app keys
 * (e.g. `flower.onboarded.v1`) are never read or written by this module.
 *
 * Error handling strategy:
 * - All reads/writes are wrapped in try/catch
 * - Corrupted or unrecognized values → treated as "not_seen"
 * - localStorage unavailable → tutorial shows (not_seen), hints DON'T show (seen)
 */

const PREFIX = "flower.onboarding.";

const KEYS = {
  tutorial: `${PREFIX}tutorial`,
  tutorialCompletedAt: `${PREFIX}tutorial.completedAt`,
  hint: (screenKey: string) => `${PREFIX}hint.${screenKey}`,
} as const;

export type TutorialStatus = "not_seen" | "completed" | "skipped";

/**
 * Check whether localStorage is available and writable.
 */
export function isLocalStorageAvailable(): boolean {
  try {
    const testKey = `${PREFIX}__test__`;
    localStorage.setItem(testKey, "1");
    localStorage.removeItem(testKey);
    return true;
  } catch {
    return false;
  }
}

/**
 * Get the current tutorial status from localStorage.
 *
 * Returns "completed" or "skipped" only if the stored value matches exactly.
 * Any other value (missing, corrupted, unexpected string) → "not_seen".
 */
export function getTutorialStatus(): TutorialStatus {
  try {
    const value = localStorage.getItem(KEYS.tutorial);
    if (value === "completed") return "completed";
    if (value === "skipped") return "skipped";
    return "not_seen";
  } catch {
    // localStorage unavailable or read error → show tutorial
    return "not_seen";
  }
}

/**
 * Persist the tutorial status to localStorage.
 *
 * Also records a `completedAt` ISO timestamp when status is "completed".
 * Write failures are silently ignored — tutorial may re-appear next time.
 */
export function setTutorialStatus(status: "completed" | "skipped"): void {
  try {
    localStorage.setItem(KEYS.tutorial, status);
    if (status === "completed") {
      localStorage.setItem(KEYS.tutorialCompletedAt, new Date().toISOString());
    }
  } catch {
    // Silently fail — tutorial may show again next session
  }
}

/**
 * Reset tutorial state so it can be shown again.
 *
 * Removes both the status key and the completedAt timestamp.
 */
export function resetTutorial(): void {
  try {
    localStorage.removeItem(KEYS.tutorial);
    localStorage.removeItem(KEYS.tutorialCompletedAt);
  } catch {
    // Silently fail
  }
}

/**
 * Check whether a first-use hint for a specific screen has been seen.
 *
 * If localStorage is unavailable → returns true (hint treated as "seen",
 * so it won't show and won't block access to functionality).
 *
 * If localStorage is available but the key is missing or corrupted →
 * returns false (hint should show).
 */
export function isHintSeen(screenKey: string): boolean {
  if (!isLocalStorageAvailable()) {
    // No localStorage → don't show hint, don't block functionality
    return true;
  }
  try {
    return localStorage.getItem(KEYS.hint(screenKey)) === "1";
  } catch {
    // Read error → treat as seen (don't block)
    return true;
  }
}

/**
 * Mark a first-use hint as seen for a specific screen.
 *
 * Write failures are silently ignored — hint may re-appear next time.
 */
export function markHintSeen(screenKey: string): void {
  try {
    localStorage.setItem(KEYS.hint(screenKey), "1");
  } catch {
    // Silently fail — hint may show again next session
  }
}
