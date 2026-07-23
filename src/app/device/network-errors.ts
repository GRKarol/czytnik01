/**
 * Tłumaczy surowe błędy przeglądarki (fetch/XHR) na czytelne komunikaty
 * po polsku. `fetch()` rzuca zawsze tym samym, angielskim
 * `TypeError: Failed to fetch` niezależnie od prawdziwej przyczyny
 * (brak sieci, urządzenie nieosiągalne, zablokowane przez CORS...) —
 * appka pokazywała ten surowy tekst użytkownikowi wprost, co jest
 * nieczytelne dla kogoś kto nie zna się na fetch API.
 */
export function describeNetworkError(err: unknown, context?: "device" | "internet"): string {
  if (err instanceof DOMException && (err.name === "AbortError" || err.name === "TimeoutError")) {
    return context === "device"
      ? "Urządzenie nie odpowiedziało na czas. Sprawdź czy jest w pobliżu i wciąż w trybie Sync."
      : "Upłynął czas oczekiwania na odpowiedź serwera. Spróbuj ponownie.";
  }
  if (err instanceof TypeError && /fetch/i.test(err.message)) {
    return context === "device"
      ? "Nie udało się połączyć z czytnikiem. Sprawdź czy telefon jest wciąż podłączony do sieci Flower-… i spróbuj ponownie."
      : "Brak połączenia z internetem albo serwer jest nieosiągalny. Sprawdź WiFi i spróbuj ponownie.";
  }
  if (err instanceof Error) return err.message;
  return String(err);
}
