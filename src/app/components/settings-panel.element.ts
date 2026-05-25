import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import {
  deviceApi,
  type DeviceSettings,
  type Theme,
  type Language,
  type ReaderHand,
  type ReaderMode,
  type PauseBehaviour,
} from "../device/api";

const THEME_LABEL: Record<Theme, string> = {
  light: "Jasny",
  dark: "Ciemny",
  night: "Nocny",
};
const LANG_LABEL: Record<Language, string> = {
  pl: "Polski",
  en: "English",
  de: "Deutsch",
  es: "Español",
  fr: "Français",
  it: "Italiano",
};
const HAND_LABEL: Record<ReaderHand, string> = { right: "Prawa", left: "Lewa" };
const MODE_LABEL: Record<ReaderMode, string> = { rsvp: "RSVP", scroll: "Przewijanie" };
const PAUSE_LABEL: Record<PauseBehaviour, string> = {
  tap: "Tap",
  "long-press": "Przytrzymanie",
  auto: "Auto",
};

@customElement("settings-panel")
export class SettingsPanel extends LitElement {
  @state() private settings: DeviceSettings | null = null;
  @state() private saving = false;
  @state() private error = "";
  @state() private tapCount = 0;
  @state() private justUnlocked = false;
  private tapResetTimer: number | null = null;

  connectedCallback(): void {
    super.connectedCallback();
    void this.load();
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    if (this.tapResetTimer) window.clearTimeout(this.tapResetTimer);
  }

  render() {
    if (!this.settings) {
      return html`<p class="muted">Wczytuję ustawienia z urządzenia…</p>`;
    }
    const s = this.settings;
    return html`
      <div class="brand" @click=${this.onBrandTap}>
        <strong>Flower</strong>
        <span>Ustawienia urządzenia</span>
        ${this.tapCount > 0 && this.tapCount < 10 && !s.devMode
          ? html`<small class="tap-hint">${10 - this.tapCount} aby odblokować…</small>`
          : ""}
        ${this.justUnlocked
          ? html`<small class="tap-hint ok">Tryb developera włączony</small>`
          : ""}
      </div>

      ${this.error ? html`<p class="error">${this.error}</p>` : ""}

      <fieldset class="group">
        <legend>Wyświetlanie</legend>
        ${this.segmented("theme", s.theme, ["light", "dark", "night"], THEME_LABEL)}
        ${this.slider("brightness", "Jasność", s.brightness, 10, 100, 5, "%")}
        ${this.segmented("readerHand", s.readerHand, ["right", "left"], HAND_LABEL, "Dłoń")}
      </fieldset>

      <fieldset class="group">
        <legend>Czytanie</legend>
        ${this.segmented("readerMode", s.readerMode, ["rsvp", "scroll"], MODE_LABEL, "Tryb")}
        ${this.segmented(
          "pauseBehaviour",
          s.pauseBehaviour,
          ["tap", "long-press", "auto"],
          PAUSE_LABEL,
          "Pauza",
        )}
        ${this.slider("baseWpm", "Tempo", s.baseWpm, 50, 1000, 25, "WPM")}
        ${this.slider("longWordDelayMs", "Długie słowa", s.longWordDelayMs, 0, 600, 50, "ms")}
        ${this.slider("complexWordDelayMs", "Złożone słowa", s.complexWordDelayMs, 0, 600, 50, "ms")}
        ${this.slider("punctuationDelayMs", "Interpunkcja", s.punctuationDelayMs, 0, 600, 50, "ms")}
      </fieldset>

      <fieldset class="group">
        <legend>HUD podczas czytania</legend>
        ${this.toggle("showBatteryWhileReading", "Bateria", s.showBatteryWhileReading)}
        ${this.toggle("showChapterWhileReading", "Rozdział", s.showChapterWhileReading)}
        ${this.toggle("showPercentWhileReading", "Procent", s.showPercentWhileReading)}
      </fieldset>

      <fieldset class="group">
        <legend>Język</legend>
        <label class="select">
          <span>Język interfejsu</span>
          <select @change=${(e: Event) => this.put({ language: (e.target as HTMLSelectElement).value as Language })}>
            ${(Object.keys(LANG_LABEL) as Language[]).map(
              (l) => html`<option value=${l} ?selected=${l === s.language}>${LANG_LABEL[l]}</option>`,
            )}
          </select>
        </label>
      </fieldset>

      ${s.devMode
        ? html`
            <fieldset class="group dev">
              <legend>Developer</legend>
              <p class="muted small">
                Te opcje są ukryte przed klientem. Włączasz je tylko z aplikacji
                — na samym urządzeniu też nic nie widzi, dopóki tu jest „On".
              </p>
              ${this.toggle("devMode", "Tryb developera", s.devMode)}
              <p class="muted small">
                Po wyłączeniu trybu developera advanced ustawienia (OTA owner,
                Auto OTA, RSS feed editor, etc.) znikają zarówno z urządzenia
                jak i z tej aplikacji.
              </p>
            </fieldset>
          `
        : ""}

      ${this.saving ? html`<p class="muted small">Zapisuję…</p>` : ""}
    `;
  }

  // ─── helpers UI ───────────────────────────────────────────────────────────

  private toggle(key: keyof DeviceSettings, label: string, value: boolean) {
    return html`
      <label class="toggle">
        <span>${label}</span>
        <input
          type="checkbox"
          ?checked=${value}
          @change=${(e: Event) => this.put({ [key]: (e.target as HTMLInputElement).checked } as Partial<DeviceSettings>)}
        />
      </label>
    `;
  }

  private slider(
    key: keyof DeviceSettings,
    label: string,
    value: number,
    min: number,
    max: number,
    step: number,
    unit: string,
  ) {
    return html`
      <label class="slider">
        <span>${label}<small>${value} ${unit}</small></span>
        <input
          type="range"
          min=${min}
          max=${max}
          step=${step}
          .value=${String(value)}
          @input=${(e: Event) =>
            this.put({ [key]: Number((e.target as HTMLInputElement).value) } as Partial<DeviceSettings>)}
        />
      </label>
    `;
  }

  private segmented<K extends keyof DeviceSettings>(
    key: K,
    current: DeviceSettings[K],
    options: ReadonlyArray<DeviceSettings[K]>,
    labels: Record<string, string>,
    title?: string,
  ) {
    return html`
      <label class="seg">
        ${title ? html`<span>${title}</span>` : ""}
        <div class="seg-buttons">
          ${options.map(
            (opt) => html`
              <button
                class=${opt === current ? "active" : ""}
                @click=${() => this.put({ [key]: opt } as Partial<DeviceSettings>)}
              >
                ${labels[opt as string]}
              </button>
            `,
          )}
        </div>
      </label>
    `;
  }

  // ─── network ──────────────────────────────────────────────────────────────

  private async load() {
    try {
      this.settings = await deviceApi.getSettings();
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    }
  }

  private async put(patch: Partial<DeviceSettings>) {
    if (!this.settings) return;
    // Optimistic update — natychmiast aktualizuj UI, w razie czego cofnij.
    const previous = this.settings;
    this.settings = { ...previous, ...patch };
    this.saving = true;
    this.error = "";
    try {
      this.settings = await deviceApi.putSettings(patch);
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
      this.settings = previous;
    } finally {
      this.saving = false;
    }
  }

  // ─── 10-tap unlock ────────────────────────────────────────────────────────

  private onBrandTap = () => {
    if (!this.settings) return;
    if (this.settings.devMode) return; // już odblokowane

    this.tapCount += 1;
    if (this.tapResetTimer) window.clearTimeout(this.tapResetTimer);
    this.tapResetTimer = window.setTimeout(() => {
      this.tapCount = 0;
    }, 1500);

    if (this.tapCount >= 10) {
      this.tapCount = 0;
      void this.put({ devMode: true });
      this.justUnlocked = true;
      window.setTimeout(() => (this.justUnlocked = false), 3000);
    }
  };

  static styles = css`
    :host {
      display: block;
      display: flex;
      flex-direction: column;
      gap: 14px;
    }
    .muted {
      color: var(--muted);
      margin: 0;
      font: 0.92rem/1.5 ui-sans-serif, system-ui, sans-serif;
    }
    .small {
      font-size: 0.8rem;
    }
    .error {
      color: var(--err);
      font: 0.9rem ui-sans-serif, system-ui, sans-serif;
      margin: 0;
    }
    .brand {
      display: flex;
      flex-direction: column;
      gap: 2px;
      padding: 14px;
      border: 1px solid var(--line);
      border-radius: 16px;
      background: var(--paper-tint);
      cursor: pointer;
      user-select: none;
      -webkit-user-select: none;
    }
    .brand strong {
      font-family: "Iowan Old Style", Georgia, ui-serif, serif;
      font-size: 1.4rem;
      color: var(--accent);
    }
    .brand span {
      font: 0.85rem ui-sans-serif, system-ui, sans-serif;
      color: var(--muted);
    }
    .tap-hint {
      margin-top: 4px;
      font: 600 0.75rem ui-sans-serif, system-ui, sans-serif;
      color: var(--accent);
    }
    .tap-hint.ok {
      color: var(--ok);
    }
    fieldset.group {
      margin: 0;
      padding: 14px;
      border: 1px solid var(--line);
      border-radius: 16px;
      background: var(--paper-tint);
      display: flex;
      flex-direction: column;
      gap: 10px;
    }
    fieldset.group.dev {
      border-color: var(--accent);
      background: rgba(46, 142, 255, 0.05);
    }
    legend {
      padding: 0 6px;
      font: 700 0.78rem ui-sans-serif, system-ui, sans-serif;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      color: var(--muted);
    }
    .toggle {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      font: 0.95rem ui-sans-serif, system-ui, sans-serif;
    }
    .toggle input {
      width: 44px;
      height: 26px;
      appearance: none;
      border-radius: 999px;
      background: var(--line);
      position: relative;
      cursor: pointer;
      transition: background 0.15s;
    }
    .toggle input:checked {
      background: var(--accent);
    }
    .toggle input::before {
      content: "";
      position: absolute;
      top: 3px;
      left: 3px;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #fff;
      transition: transform 0.15s;
    }
    .toggle input:checked::before {
      transform: translateX(18px);
    }
    .slider {
      display: flex;
      flex-direction: column;
      gap: 6px;
      font: 0.95rem ui-sans-serif, system-ui, sans-serif;
    }
    .slider span {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
    }
    .slider small {
      color: var(--muted);
      font: 0.82rem ui-sans-serif, system-ui, sans-serif;
    }
    .slider input[type="range"] {
      width: 100%;
      accent-color: var(--accent);
    }
    .seg {
      display: flex;
      flex-direction: column;
      gap: 6px;
      font: 0.95rem ui-sans-serif, system-ui, sans-serif;
    }
    .seg-buttons {
      display: grid;
      grid-auto-flow: column;
      grid-auto-columns: 1fr;
      gap: 4px;
      padding: 3px;
      border-radius: 999px;
      background: var(--line);
    }
    .seg-buttons button {
      padding: 8px 10px;
      border: 0;
      border-radius: 999px;
      background: transparent;
      color: var(--ink-soft);
      font: 600 0.85rem ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
    }
    .seg-buttons button.active {
      background: #fff;
      color: var(--accent);
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.08);
    }
    .select {
      display: flex;
      flex-direction: column;
      gap: 6px;
      font: 0.95rem ui-sans-serif, system-ui, sans-serif;
    }
    .select select {
      padding: 10px 12px;
      border: 1px solid var(--line);
      border-radius: 12px;
      background: #fff;
      font: 0.95rem ui-sans-serif, system-ui, sans-serif;
      color: var(--ink);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "settings-panel": SettingsPanel;
  }
}
