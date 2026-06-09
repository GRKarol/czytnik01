import { LitElement, css, html, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { isHintSeen, markHintSeen } from "../onboarding/onboarding-store";
import { t } from "../i18n/index";

/**
 * Full-screen overlay hint shown on first use of a specific screen.
 *
 * Checks `onboarding-store.isHintSeen(screenKey)` on connect:
 * - Not seen + localStorage available → show blocking overlay
 * - Already seen OR localStorage unavailable → render nothing
 *
 * On close: marks hint as seen, removes overlay, unblocks in ≤ 300ms.
 * Hint text fetched via `t(`hint.${screenKey}.text`)`, max 150 chars.
 * Close button text via `t("hint.btn.close")`, min 44×44px touch area.
 */
@customElement("first-use-hint")
export class FirstUseHint extends LitElement {
  @property() screenKey: string = "";
  @state() private visible: boolean = false;

  connectedCallback(): void {
    super.connectedCallback();
    // If hint has NOT been seen (and localStorage is available), show overlay.
    // isHintSeen returns true when localStorage is unavailable (don't block).
    if (this.screenKey && !isHintSeen(this.screenKey)) {
      this.visible = true;
    }
  }

  render() {
    if (!this.visible) {
      return nothing;
    }

    const hintText = this.getHintText();
    const closeLabel = t("hint.btn.close");

    return html`
      <div class="overlay" role="dialog" aria-modal="true" aria-label=${hintText}>
        <div class="card">
          <p class="hint-text">${hintText}</p>
          <button class="close-btn" @click=${this.handleClose}>${closeLabel}</button>
        </div>
      </div>
    `;
  }

  /** Get hint text, truncated to 150 chars if needed */
  private getHintText(): string {
    const text = t(`hint.${this.screenKey}.text`);
    if (text.length > 150) {
      return text.slice(0, 147) + "…";
    }
    return text;
  }

  private handleClose() {
    markHintSeen(this.screenKey);
    this.visible = false;
  }

  static styles = css`
    :host {
      display: contents;
    }

    .overlay {
      position: fixed;
      inset: 0;
      z-index: 1000;
      display: flex;
      align-items: center;
      justify-content: center;
      background: rgba(0, 0, 0, 0.55);
      pointer-events: auto;
      animation: overlay-in 0.15s ease-out;
    }

    .card {
      max-width: 320px;
      width: calc(100% - 48px);
      padding: 24px;
      border-radius: 16px;
      background: var(--paper-tint, #ffffff);
      color: var(--ink, #1a2a3a);
      box-shadow: 0 12px 40px rgba(0, 0, 0, 0.2);
      text-align: center;
    }

    .hint-text {
      margin: 0 0 20px;
      font-size: 16px;
      line-height: 1.5;
      color: var(--ink-soft, #3d5278);
      word-break: break-word;
    }

    .close-btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 44px;
      min-height: 44px;
      padding: 12px 28px;
      border: none;
      border-radius: 10px;
      background: var(--accent, #2e8eff);
      color: #fff;
      font:
        600 1rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      -webkit-tap-highlight-color: transparent;
      touch-action: manipulation;
      transition: background 0.12s;
    }

    .close-btn:hover,
    .close-btn:focus-visible {
      background: var(--accent-hover, #1a7ae6);
      outline: none;
    }

    .close-btn:active {
      background: var(--accent-press, #1268c7);
    }

    @keyframes overlay-in {
      from {
        opacity: 0;
      }
      to {
        opacity: 1;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "first-use-hint": FirstUseHint;
  }
}
