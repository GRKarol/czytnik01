import { LitElement, css, html, svg, type TemplateResult } from "lit";
import { customElement, state } from "lit/decorators.js";
import { t } from "../i18n/index";
import { setTutorialStatus } from "../onboarding/onboarding-store";

interface TutorialScreen {
  titleKey: string;
  descKey: string;
  visual: () => TemplateResult;
}

/**
 * Tutorial wizard — 4 educational screens introducing key features
 * of the Flower reader. Shown once after first device connection.
 *
 * Screens: (1) RSVP mode, (2) WPM tempo, (3) Pause modes, (4) HUD elements.
 * Navigation via buttons or swipe gestures (handled in task 3.2).
 */
@customElement("tutorial-wizard")
export class TutorialWizard extends LitElement {
  @state() private step = 0;

  private readonly screens: TutorialScreen[] = [
    {
      titleKey: "tutorial.rsvp.title",
      descKey: "tutorial.rsvp.desc",
      visual: () => this.renderRsvpVisual(),
    },
    {
      titleKey: "tutorial.wpm.title",
      descKey: "tutorial.wpm.desc",
      visual: () => this.renderWpmVisual(),
    },
    {
      titleKey: "tutorial.pause.title",
      descKey: "tutorial.pause.desc",
      visual: () => this.renderPauseVisual(),
    },
    {
      titleKey: "tutorial.hud.title",
      descKey: "tutorial.hud.desc",
      visual: () => this.renderHudVisual(),
    },
  ];

  private get totalSteps(): number {
    return this.screens.length;
  }

  private get isFirstStep(): boolean {
    return this.step === 0;
  }

  private get isLastStep(): boolean {
    return this.step === this.totalSteps - 1;
  }

  render() {
    const screen = this.screens[this.step];
    return html`
      <div class="overlay">
        <div class="card">
          <div class="progress" aria-live="polite">
            ${this.formatProgress(this.step + 1, this.totalSteps)}
          </div>

          <div class="stage">
            <div class="visual">${screen.visual()}</div>
            <h2>${t(screen.titleKey)}</h2>
            <p>${t(screen.descKey)}</p>
          </div>

          <div class="footer">
            ${this.isFirstStep
              ? html``
              : html`<button class="btn-back" @click=${this.goBack}>
                  ${t("tutorial.btn.back")}
                </button>`}

            <button class="btn-skip" @click=${this.skip}>${t("tutorial.btn.skip")}</button>

            ${this.isLastStep
              ? html`<button class="btn-next" @click=${this.finish}>
                  ${t("tutorial.btn.finish")}
                </button>`
              : html`<button class="btn-next" @click=${this.goNext}>
                  ${t("tutorial.btn.next")}
                </button>`}
          </div>
        </div>
      </div>
    `;
  }

  // ─── Swipe gesture handling ────────────────────────────────────────────

  private touchStartX = 0;
  private touchStartY = 0;

  private handleTouchStart = (e: TouchEvent) => {
    this.touchStartX = e.touches[0].clientX;
    this.touchStartY = e.touches[0].clientY;
  };

  private handleTouchEnd = (e: TouchEvent) => {
    const deltaX = e.changedTouches[0].clientX - this.touchStartX;
    const deltaY = e.changedTouches[0].clientY - this.touchStartY;

    // Ignore vertical scrolls — vertical delta takes priority
    if (Math.abs(deltaY) > Math.abs(deltaX)) {
      return;
    }

    const SWIPE_THRESHOLD = 50;

    if (deltaX < -SWIPE_THRESHOLD) {
      // Swipe left → next screen
      this.goNext();
    } else if (deltaX > SWIPE_THRESHOLD && this.step > 0) {
      // Swipe right → previous screen (blocked on first screen)
      this.goBack();
    }
  };

  connectedCallback(): void {
    super.connectedCallback();
    this.addEventListener("touchstart", this.handleTouchStart, { passive: true });
    this.addEventListener("touchend", this.handleTouchEnd, { passive: true });
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this.removeEventListener("touchstart", this.handleTouchStart);
    this.removeEventListener("touchend", this.handleTouchEnd);
  }

  // ─── Navigation ──────────────────────────────────────────────────────

  private goNext = () => {
    if (this.step < this.totalSteps - 1) {
      this.step += 1;
    }
  };

  private goBack = () => {
    if (this.step > 0) {
      this.step -= 1;
    }
  };

  private skip = () => {
    setTutorialStatus("skipped");
    this.dispatchEvent(new CustomEvent("tutorial-close", { bubbles: true, composed: true }));
  };

  private finish = () => {
    setTutorialStatus("completed");
    this.dispatchEvent(new CustomEvent("tutorial-close", { bubbles: true, composed: true }));
  };

  // ─── Progress formatting ─────────────────────────────────────────────

  /**
   * Format progress string with placeholder replacement.
   * The t() function doesn't support placeholders natively, so we
   * replace {current} and {total} manually.
   */
  private formatProgress(current: number, total: number): string {
    const template = t("tutorial.progress");
    return template.replace("{current}", String(current)).replace("{total}", String(total));
  }

  // ─── Screen visuals ──────────────────────────────────────────────────

  private renderRsvpVisual(): TemplateResult {
    // Word with highlighted ORP letter
    return html`
      <div class="rsvp-demo" aria-hidden="true">
        <span class="phantom">na</span>
        <span class="word">
          <span>c</span><span>z</span><span class="orp">y</span><span>t</span><span>a</span
          ><span>n</span><span>i</span><span>e</span>
        </span>
        <span class="phantom">to</span>
      </div>
    `;
  }

  private renderWpmVisual(): TemplateResult {
    // Speed gauge representation
    return svg`
      <svg class="visual-svg" width="100" height="60" viewBox="0 0 100 60" aria-hidden="true">
        <text x="10" y="40" font-size="14" fill="var(--muted, #6b7c97)">50</text>
        <rect x="28" y="28" width="44" height="8" rx="4" fill="var(--sky-2, #d1e9fb)"/>
        <rect x="28" y="28" width="26" height="8" rx="4" fill="var(--accent, #2e8eff)"/>
        <text x="76" y="40" font-size="14" fill="var(--muted, #6b7c97)">1000</text>
        <text x="36" y="18" font-size="12" font-weight="bold" fill="var(--ink, #0c2340)">300 WPM</text>
      </svg>
    `;
  }

  private renderPauseVisual(): TemplateResult {
    // Three pause mode icons
    return svg`
      <svg class="visual-svg" width="120" height="60" viewBox="0 0 120 60" aria-hidden="true">
        <!-- Tap icon -->
        <circle cx="20" cy="25" r="12" fill="none" stroke="var(--accent, #2e8eff)" stroke-width="2"/>
        <circle cx="20" cy="25" r="3" fill="var(--accent, #2e8eff)"/>
        <text x="10" y="52" font-size="9" fill="var(--muted, #6b7c97)">Tap</text>

        <!-- Hold icon -->
        <circle cx="60" cy="25" r="12" fill="none" stroke="var(--accent, #2e8eff)" stroke-width="2"/>
        <line x1="60" y1="18" x2="60" y2="32" stroke="var(--accent, #2e8eff)" stroke-width="2" stroke-linecap="round"/>
        <text x="47" y="52" font-size="9" fill="var(--muted, #6b7c97)">Hold</text>

        <!-- Auto icon -->
        <circle cx="100" cy="25" r="12" fill="none" stroke="var(--accent, #2e8eff)" stroke-width="2"/>
        <rect x="95" y="20" width="4" height="10" fill="var(--accent, #2e8eff)" rx="1"/>
        <rect x="101" y="20" width="4" height="10" fill="var(--accent, #2e8eff)" rx="1"/>
        <text x="89" y="52" font-size="9" fill="var(--muted, #6b7c97)">Auto</text>
      </svg>
    `;
  }

  private renderHudVisual(): TemplateResult {
    // HUD elements: battery, chapter, percent
    return svg`
      <svg class="visual-svg" width="120" height="60" viewBox="0 0 120 60" aria-hidden="true">
        <!-- Battery icon -->
        <rect x="8" y="18" width="22" height="12" rx="3" fill="none" stroke="var(--accent, #2e8eff)" stroke-width="1.5"/>
        <rect x="30" y="22" width="3" height="4" rx="1" fill="var(--accent, #2e8eff)"/>
        <rect x="11" y="21" width="14" height="6" rx="1.5" fill="var(--accent, #2e8eff)"/>
        <text x="7" y="46" font-size="8" fill="var(--muted, #6b7c97)">Battery</text>

        <!-- Chapter icon -->
        <rect x="46" y="16" width="16" height="16" rx="2" fill="none" stroke="var(--accent, #2e8eff)" stroke-width="1.5"/>
        <line x1="50" y1="21" x2="58" y2="21" stroke="var(--accent, #2e8eff)" stroke-width="1"/>
        <line x1="50" y1="24" x2="58" y2="24" stroke="var(--accent, #2e8eff)" stroke-width="1"/>
        <line x1="50" y1="27" x2="55" y2="27" stroke="var(--accent, #2e8eff)" stroke-width="1"/>
        <text x="42" y="46" font-size="8" fill="var(--muted, #6b7c97)">Chapter</text>

        <!-- Percent icon -->
        <circle cx="95" cy="24" r="10" fill="none" stroke="var(--sky-2, #d1e9fb)" stroke-width="3"/>
        <circle cx="95" cy="24" r="10" fill="none" stroke="var(--accent, #2e8eff)" stroke-width="3"
                stroke-dasharray="47" stroke-dashoffset="16" stroke-linecap="round"/>
        <text x="89" y="28" font-size="8" font-weight="bold" fill="var(--ink, #0c2340)">%</text>
        <text x="83" y="46" font-size="8" fill="var(--muted, #6b7c97)">Progress</text>
      </svg>
    `;
  }

  // ─── Styles ──────────────────────────────────────────────────────────

  static styles = css`
    :host {
      position: fixed;
      inset: 0;
      z-index: 110;
      pointer-events: none;
    }

    .overlay {
      position: absolute;
      inset: 0;
      display: grid;
      place-items: center;
      padding: 16px;
      background: rgba(12, 35, 64, 0.6);
      backdrop-filter: blur(6px);
      pointer-events: auto;
    }

    .card {
      width: 100%;
      max-width: 420px;
      padding: 28px 24px;
      background: var(--paper, #ffffff);
      border-radius: 20px;
      box-shadow: 0 16px 40px rgba(0, 0, 0, 0.18);
      color: var(--ink, #0c2340);
      display: flex;
      flex-direction: column;
      gap: 18px;
      font-family: ui-sans-serif, system-ui, sans-serif;
    }

    .progress {
      text-align: center;
      font-size: 0.85rem;
      font-weight: 600;
      color: var(--muted, #6b7c97);
      letter-spacing: 0.02em;
    }

    .stage {
      display: flex;
      flex-direction: column;
      align-items: center;
      text-align: center;
      gap: 12px;
    }

    .visual {
      display: flex;
      align-items: center;
      justify-content: center;
      min-height: 68px;
      padding: 12px;
      border-radius: 14px;
      background: var(--sky-1, #e8f4fd);
    }

    .visual-svg {
      display: block;
    }

    h2 {
      margin: 0;
      font-size: 1.4rem;
      font-weight: 700;
      color: var(--ink, #0c2340);
      letter-spacing: -0.01em;
    }

    p {
      margin: 0;
      font-size: 1rem;
      line-height: 1.55;
      color: var(--ink-soft, #3d5278);
      max-width: 36ch;
    }

    .footer {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      padding-top: 6px;
    }

    .btn-back {
      background: transparent;
      border: 0;
      color: var(--muted, #6b7c97);
      font:
        600 0.9rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      padding: 10px 8px;
      min-height: 44px;
      min-width: 44px;
    }

    .btn-skip {
      background: transparent;
      border: 0;
      color: var(--muted, #6b7c97);
      font:
        600 0.85rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      padding: 10px 8px;
      min-height: 44px;
      text-decoration: underline;
      text-underline-offset: 2px;
    }

    .btn-next {
      padding: 12px 22px;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: var(--accent, #2e8eff);
      font:
        700 0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      box-shadow: 0 8px 18px rgba(46, 142, 255, 0.25);
      min-height: 44px;
      transition: background 0.15s;
    }

    .btn-next:hover {
      background: var(--accent-deep, #1f6fd4);
    }

    /* RSVP demo visual */
    .rsvp-demo {
      display: flex;
      align-items: baseline;
      gap: 8px;
      font-family: ui-monospace, SFMono-Regular, monospace;
      font-size: 1.3rem;
    }

    .phantom {
      color: var(--muted, #6b7c97);
      opacity: 0.4;
      font-size: 1rem;
    }

    .word {
      font-weight: 700;
      color: var(--ink, #0c2340);
      letter-spacing: 0.06em;
    }

    .orp {
      color: var(--accent, #2e8eff);
      font-weight: 900;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "tutorial-wizard": TutorialWizard;
  }
}
