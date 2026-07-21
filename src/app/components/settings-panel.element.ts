import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import {
  deviceApi,
  onDeviceApiChange,
  type DeviceSettings,
  type Theme,
  type Language,
  type ReaderHand,
  type ReaderMode,
  type PauseBehaviour,
  type Typeface,
  type FooterMetric,
  type BatteryLabel,
  type NavMode,
  type ScreensaverMode,
} from "../device/api";
import { setLang } from "../i18n/index";
import { deviceLangToSupported } from "../i18n/lang-map";
import "./help-panel.element";
import "./setting-tooltip.element";

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
  ro: "Română",
};
const HAND_LABEL: Record<ReaderHand, string> = { right: "Prawa", left: "Lewa" };
const MODE_LABEL: Record<ReaderMode, string> = { rsvp: "RSVP", scroll: "Przewijanie" };
const PAUSE_LABEL: Record<PauseBehaviour, string> = {
  tap: "Tap",
  "long-press": "Przytrzymanie",
  auto: "Auto",
};

const FONT_SIZE_LABEL: Record<number, string> = {
  0: "0",
  1: "1",
  2: "2",
  3: "3",
  4: "4",
  5: "5",
  6: "6",
  7: "7",
  8: "8",
};

const LINE_SPACING_LABEL: Record<number, string> = {
  0: "Compact",
  1: "Normal",
  2: "Relaxed",
};

const MARGIN_LABEL: Record<number, string> = {
  0: "Narrow",
  1: "Normal",
  2: "Wide",
};

const TYPEFACE_LABEL: Record<Typeface, string> = {
  standard: "Standard",
  open_dyslexic: "OpenDyslexic",
  atkinson: "Atkinson",
};

const RSVP_FONT_SIZE_LABEL: Record<number, string> = {
  0: "S",
  1: "M",
  2: "L",
};

const FOOTER_METRIC_LABEL: Record<FooterMetric, string> = {
  percentage: "Procent",
  chapter_time: "Czas rozdziału",
  book_time: "Czas książki",
};

const BATTERY_LABEL_LABEL: Record<BatteryLabel, string> = {
  percent: "Procent",
  time_remaining: "Czas",
  voltage: "Napięcie",
};

// Dopisane po audycie parytetu firmware<->appka (2026-07-21) —
// docs/roadmap.md, Faza 6.
const NAV_MODE_LABEL: Record<NavMode, string> = { swipe: "Gesty", dpad: "D-Pad" };
const FOCUS_COLOR_LABEL: Record<number, string> = {
  0: "Czerwony",
  1: "Niebieski",
  2: "Zielony",
  3: "Żółty",
  4: "Pomarańczowy",
  5: "Fioletowy",
};
const SCREENSAVER_MODE_LABEL: Record<ScreensaverMode, string> = {
  life: "Life",
  maze: "Labirynt",
  voronoi: "Voronoi",
  stars: "Gwiazdy",
  matrix: "Matrix",
  screen_off: "Wyłącz ekran",
};
// Wartości minut zgodne z kScreensaverTimeoutMinutes/kScreensaverAutoOffMinutes/
// kScreensaverSleepGuardMinutes w firmware/src/app/App.cpp.
const SCREENSAVER_TIMEOUT_LABEL: Record<number, string> = {
  0: "1 min",
  1: "2 min",
  2: "3 min",
  3: "5 min",
  4: "10 min",
  5: "15 min",
  6: "20 min",
  7: "30 min",
};
const SCREENSAVER_AUTOOFF_LABEL: Record<number, string> = {
  0: "Nigdy",
  1: "5 min",
  2: "10 min",
  3: "15 min",
  4: "20 min",
  5: "30 min",
  6: "45 min",
  7: "60 min",
};
const SCREENSAVER_SLEEPGUARD_LABEL: Record<number, string> = {
  0: "Wyłączone",
  1: "5 min",
  2: "10 min",
  3: "15 min",
  4: "20 min",
  5: "30 min",
  6: "45 min",
  7: "60 min",
};

type SettingsSubView = "settings" | "help";

@customElement("settings-panel")
export class SettingsPanel extends LitElement {
  @state() private settings: DeviceSettings | null = null;
  @state() private saving = false;
  @state() private error = "";
  @state() private tapCount = 0;
  @state() private justUnlocked = false;
  @state() private subView: SettingsSubView = "settings";
  private tapResetTimer: number | null = null;
  private unsubApi: (() => void) | null = null;

  private get effectiveMode(): ReaderMode {
    const m = this.settings?.readerMode;
    return m === "scroll" ? "scroll" : "rsvp";
  }

  connectedCallback(): void {
    super.connectedCallback();
    void this.load();
    this.unsubApi = onDeviceApiChange(() => void this.load());
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    if (this.tapResetTimer) window.clearTimeout(this.tapResetTimer);
    this.unsubApi?.();
  }

  render() {
    if (this.subView === "help") {
      return html`
        <help-panel
          @help-close=${this.handleHelpClose}
          @restart-tutorial=${this.handleRestartTutorial}
        ></help-panel>
      `;
    }

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
        <legend>Tryb czytania</legend>
        ${this.segmented(
          "readerMode",
          s.readerMode,
          ["rsvp", "scroll"],
          MODE_LABEL,
          "Tryb",
          "readingMode",
        )}
      </fieldset>

      ${this.effectiveMode === "rsvp"
        ? html`
            <fieldset class="group">
              <legend>Ustawienia RSVP</legend>
              ${this.segmented(
                "pauseBehaviour",
                s.pauseBehaviour,
                ["tap", "long-press", "auto"],
                PAUSE_LABEL,
                "Pauza",
                "pauseBehaviour",
              )}
              ${this.slider("baseWpm", "Tempo", s.baseWpm, 50, 1000, 25, "WPM", "baseWpm")}
              ${this.slider(
                "longWordDelayMs",
                "Długie słowa",
                s.longWordDelayMs,
                0,
                600,
                50,
                "ms",
                "longWordDelay",
              )}
              ${this.slider(
                "complexWordDelayMs",
                "Złożone słowa",
                s.complexWordDelayMs,
                0,
                600,
                50,
                "ms",
                "complexWordDelay",
              )}
              ${this.slider(
                "punctuationDelayMs",
                "Interpunkcja",
                s.punctuationDelayMs,
                0,
                600,
                50,
                "ms",
                "punctuationDelay",
              )}
              ${this.toggle("phantomWords", "Słowa widma", s.phantomWords)}
            </fieldset>

            <fieldset class="group">
              <legend>Typografia RSVP</legend>
              ${this.segmented(
                "fontSizeIndex",
                s.fontSizeIndex,
                [0, 1, 2],
                RSVP_FONT_SIZE_LABEL,
                "Rozmiar czcionki",
              )}
              ${this.segmented(
                "typeface",
                s.typeface,
                ["standard", "open_dyslexic", "atkinson"],
                TYPEFACE_LABEL,
                "Krój czcionki",
              )}
              ${this.toggle("focusHighlight", "Podświetlenie fokusowe", s.focusHighlight)}
              ${this.segmented(
                "focusColorIndex",
                s.focusColorIndex,
                [0, 1, 2, 3, 4, 5],
                FOCUS_COLOR_LABEL,
                "Kolor podświetlenia",
              )}
              ${this.slider("tracking", "Tracking (odstępy)", s.tracking, -2, 3, 1, "")}
              ${this.slider("anchorPercent", "Pozycja kotwicy", s.anchorPercent, 30, 40, 1, "%")}
              ${this.slider("guideWidth", "Szerokość prowadnicy", s.guideWidth, 12, 30, 1, "px")}
              ${this.slider("guideGap", "Przerwa prowadnicy", s.guideGap, 2, 8, 1, "px")}
            </fieldset>
          `
        : html`
            <fieldset class="group">
              <legend>Ustawienia Scroll</legend>
              ${this.segmented(
                "scrollFontSize",
                s.scrollFontSize,
                [0, 1, 2, 3, 4, 5, 6, 7, 8],
                FONT_SIZE_LABEL,
                "Rozmiar czcionki",
              )}
              ${this.segmented(
                "scrollLineSpacing",
                s.scrollLineSpacing,
                [0, 1, 2],
                LINE_SPACING_LABEL,
                "Interlinia",
              )}
              ${this.segmented(
                "scrollMargin",
                s.scrollMargin,
                [0, 1, 2],
                MARGIN_LABEL,
                "Marginesy",
              )}
            </fieldset>
          `}

      <fieldset class="group">
        <legend>Wyświetlanie</legend>
        ${this.segmented(
          "theme",
          s.theme,
          ["light", "dark", "night"],
          THEME_LABEL,
          undefined,
          "theme",
        )}
        ${this.slider("brightness", "Jasność", s.brightness, 10, 100, 5, "%", "brightness")}
        ${this.segmented(
          "readerHand",
          s.readerHand,
          ["right", "left"],
          HAND_LABEL,
          "Dłoń",
          "readerHand",
        )}
      </fieldset>

      <fieldset class="group">
        <legend>HUD podczas czytania</legend>
        ${this.toggle(
          "showBatteryWhileReading",
          "Bateria",
          s.showBatteryWhileReading,
          "readingBattery",
        )}
        ${this.toggle(
          "showChapterWhileReading",
          "Rozdział",
          s.showChapterWhileReading,
          "readingChapter",
        )}
        ${this.toggle(
          "showPercentWhileReading",
          "Procent",
          s.showPercentWhileReading,
          "readingPercent",
        )}
        ${this.segmented(
          "footerMetric",
          s.footerMetric,
          ["percentage", "chapter_time", "book_time"],
          FOOTER_METRIC_LABEL,
          "Metryka stopki",
        )}
        ${this.segmented(
          "batteryLabel",
          s.batteryLabel,
          ["percent", "time_remaining", "voltage"],
          BATTERY_LABEL_LABEL,
          "Etykieta baterii",
        )}
        ${this.toggle(
          "accurateTimeEstimate",
          "Dokładny szacowany czas",
          s.accurateTimeEstimate,
        )}
        ${this.toggle("savePointButtonVisible", "Przycisk zakładki", s.savePointButtonVisible)}
        ${this.toggle("showHelpHints", "Podpowiedzi na urządzeniu", s.showHelpHints)}
      </fieldset>

      <fieldset class="group">
        <legend>Sterowanie</legend>
        ${this.segmented(
          "navMode",
          s.navMode,
          ["swipe", "dpad"],
          NAV_MODE_LABEL,
          "Nawigacja w menu",
        )}
      </fieldset>

      <fieldset class="group">
        <legend>Wygaszacz ekranu</legend>
        ${this.segmented(
          "screensaverMode",
          s.screensaverMode,
          ["life", "maze", "voronoi", "stars", "matrix", "screen_off"],
          SCREENSAVER_MODE_LABEL,
          "Animacja",
        )}
        ${this.segmented(
          "screensaverTimeoutIndex",
          s.screensaverTimeoutIndex,
          [0, 1, 2, 3, 4, 5, 6, 7],
          SCREENSAVER_TIMEOUT_LABEL,
          "Czas do wygaszacza",
        )}
        ${this.segmented(
          "screensaverAutoOffIndex",
          s.screensaverAutoOffIndex,
          [0, 1, 2, 3, 4, 5, 6, 7],
          SCREENSAVER_AUTOOFF_LABEL,
          "Auto-wyłączenie",
        )}
        ${this.segmented(
          "screensaverSleepGuardIndex",
          s.screensaverSleepGuardIndex,
          [0, 1, 2, 3, 4, 5, 6, 7],
          SCREENSAVER_SLEEPGUARD_LABEL,
          "Ochrona przed uśpieniem",
        )}
      </fieldset>

      <fieldset class="group">
        <legend>Język</legend>
        <label class="select">
          <span>Język interfejsu</span>
          <select
            @change=${(e: Event) =>
              this.put({ language: (e.target as HTMLSelectElement).value as Language })}
          >
            ${(Object.keys(LANG_LABEL) as Language[]).map(
              (l) =>
                html`<option value=${l} ?selected=${l === s.language}>${LANG_LABEL[l]}</option>`,
            )}
          </select>
        </label>
      </fieldset>

      ${s.devMode
        ? html`
            <fieldset class="group dev">
              <legend>Developer</legend>
              <p class="muted small">
                Te opcje są ukryte przed klientem. Włączasz je tylko z aplikacji — na samym
                urządzeniu też nic nie widzi, dopóki tu jest „On".
              </p>
              ${this.toggle("devMode", "Tryb developera", s.devMode)}
              ${this.toggle("otaAutoCheck", "Auto-sprawdzanie aktualizacji", s.otaAutoCheck)}
              ${this.toggle("bleEnabled", "Bluetooth (peryferium)", s.bleEnabled)}
              <p class="muted small">
                Po wyłączeniu trybu developera advanced ustawienia (OTA owner, Auto OTA, RSS feed
                editor, etc.) znikają zarówno z urządzenia jak i z tej aplikacji.
              </p>
            </fieldset>
          `
        : ""}

      <button class="help-link" @click=${this.openHelp}>
        <svg
          width="20"
          height="20"
          viewBox="0 0 20 20"
          fill="none"
          stroke="currentColor"
          stroke-width="1.8"
          stroke-linecap="round"
          stroke-linejoin="round"
          aria-hidden="true"
        >
          <circle cx="10" cy="10" r="8" />
          <path d="M7.5 7.5a2.5 2.5 0 0 1 4.5 1.5c0 1.5-2 2-2 3" />
          <circle cx="10" cy="14.5" r="0.5" fill="currentColor" />
        </svg>
        <span>Pomoc / Przewodnik</span>
      </button>

      ${this.saving ? html`<p class="muted small">Zapisuję…</p>` : ""}
    `;
  }

  // ─── help-panel navigation ────────────────────────────────────────────────

  private openHelp(): void {
    this.subView = "help";
  }

  private handleHelpClose(): void {
    this.subView = "settings";
  }

  private handleRestartTutorial(): void {
    this.subView = "settings";
    this.dispatchEvent(new CustomEvent("restart-tutorial", { bubbles: true, composed: true }));
  }

  // ─── helpers UI ───────────────────────────────────────────────────────────

  private toggle(key: keyof DeviceSettings, label: string, value: boolean, tooltipKey?: string) {
    return html`
      <label class="toggle">
        <span class="label-with-tooltip"
          >${label}${tooltipKey
            ? html`<setting-tooltip settingKey=${tooltipKey}></setting-tooltip>`
            : ""}</span
        >
        <input
          type="checkbox"
          ?checked=${value}
          @change=${(e: Event) =>
            this.put({ [key]: (e.target as HTMLInputElement).checked } as Partial<DeviceSettings>)}
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
    tooltipKey?: string,
  ) {
    return html`
      <label class="slider">
        <span
          >${label}${tooltipKey
            ? html`<setting-tooltip settingKey=${tooltipKey}></setting-tooltip>`
            : ""}<small>${value} ${unit}</small></span
        >
        <input
          type="range"
          min=${min}
          max=${max}
          step=${step}
          .value=${String(value)}
          @input=${(e: Event) =>
            this.put({
              [key]: Number((e.target as HTMLInputElement).value),
            } as Partial<DeviceSettings>)}
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
    tooltipKey?: string,
  ) {
    const showHeader = title || tooltipKey;
    return html`
      <label class="seg">
        ${showHeader
          ? html`<span class="label-with-tooltip"
              >${title ?? ""}${tooltipKey
                ? html`<setting-tooltip settingKey=${tooltipKey}></setting-tooltip>`
                : ""}</span
            >`
          : ""}
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
      // Sync i18n module with device language on initial load
      if (this.settings) {
        setLang(deviceLangToSupported(this.settings.language));
      }
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

    // Sync i18n when language changes
    if ("language" in patch && patch.language) {
      setLang(deviceLangToSupported(patch.language));
    }

    try {
      this.settings = await deviceApi.putSettings(patch);
      // Powiedz rodzicowi (app.element.ts) żeby odświeżył DEV badge w header.
      if ("devMode" in patch && previous.devMode !== this.settings.devMode) {
        this.dispatchEvent(
          new CustomEvent("device-settings-changed", {
            bubbles: true,
            composed: true,
            detail: this.settings,
          }),
        );
      }
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
      this.settings = previous;
      // Revert i18n on failure
      if ("language" in patch) {
        setLang(deviceLangToSupported(previous.language));
      }
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
      font:
        0.92rem/1.5 ui-sans-serif,
        system-ui,
        sans-serif;
    }
    .small {
      font-size: 0.8rem;
    }
    .error {
      color: var(--err);
      font:
        0.9rem ui-sans-serif,
        system-ui,
        sans-serif;
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
      font:
        0.85rem ui-sans-serif,
        system-ui,
        sans-serif;
      color: var(--muted);
    }
    .tap-hint {
      margin-top: 4px;
      font:
        600 0.75rem ui-sans-serif,
        system-ui,
        sans-serif;
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
      /* Fieldsety mają domyślnie min-width: min-content w przeglądarkach —
         bez tego rząd segmentów z długimi etykietami (np. nazwy kolorów)
         rozpycha całą sekcję szerzej niż ekran zamiast się zawinąć. To był
         powód "Typografia RSVP zbyt szeroka, dziwnie się przesuwa". */
      min-width: 0;
    }
    fieldset.group.dev {
      border-color: var(--accent);
      background: rgba(46, 142, 255, 0.05);
    }
    legend {
      padding: 0 6px;
      font:
        700 0.78rem ui-sans-serif,
        system-ui,
        sans-serif;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      color: var(--muted);
    }
    .toggle {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      font:
        0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
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
      font:
        0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
    }
    .slider span {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
    }
    .slider small {
      color: var(--muted);
      font:
        0.82rem ui-sans-serif,
        system-ui,
        sans-serif;
    }
    .slider input[type="range"] {
      width: 100%;
      accent-color: var(--accent);
    }
    .seg {
      display: flex;
      flex-direction: column;
      gap: 6px;
      font:
        0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
    }
    .seg-buttons {
      display: flex;
      flex-wrap: wrap;
      gap: 4px;
      padding: 3px;
      border-radius: 14px;
      background: var(--line);
    }
    .seg-buttons button {
      flex: 1 1 auto;
      min-width: max-content;
      padding: 8px 10px;
      border: 0;
      border-radius: 999px;
      background: transparent;
      color: var(--ink-soft);
      font:
        600 0.85rem ui-sans-serif,
        system-ui,
        sans-serif;
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
      font:
        0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
    }
    .select select {
      padding: 10px 12px;
      border: 1px solid var(--line);
      border-radius: 12px;
      background: #fff;
      font:
        0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
      color: var(--ink);
    }
    .help-link {
      display: flex;
      align-items: center;
      gap: 10px;
      padding: 14px;
      border: 1px solid var(--line);
      border-radius: 16px;
      background: var(--paper-tint);
      color: var(--ink);
      font:
        600 0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      transition: border-color 0.15s;
    }
    .help-link:hover {
      border-color: var(--accent);
    }
    .help-link svg {
      flex-shrink: 0;
      color: var(--accent);
    }
    .label-with-tooltip {
      display: inline-flex;
      align-items: center;
      gap: 6px;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "settings-panel": SettingsPanel;
  }
}
