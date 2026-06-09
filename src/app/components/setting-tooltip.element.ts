import { LitElement, css, html } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { t } from "../i18n/index";

/**
 * Kontekstowy tooltip wyjaśniający przeznaczenie ustawienia w panelu.
 *
 * Renderuje ikonę "?" — po kliknięciu wyświetla popup z opisem, efektem
 * zmiany oraz wartością domyślną. Tylko 1 tooltip widoczny na raz
 * (singleton pattern via document event bus).
 *
 * Treść pobierana z i18n:
 *   tooltip.{settingKey}.desc
 *   tooltip.{settingKey}.effect
 *   tooltip.{settingKey}.default
 */
@customElement("setting-tooltip")
export class SettingTooltip extends LitElement {
  @property() settingKey: string = "";
  @state() private visible: boolean = false;

  private boundOnDocumentClick: ((e: MouseEvent) => void) | null = null;
  private boundOnTooltipOpen: ((e: Event) => void) | null = null;

  connectedCallback(): void {
    super.connectedCallback();
    this.boundOnTooltipOpen = this.handleTooltipOpen.bind(this);
    document.addEventListener("tooltip-open", this.boundOnTooltipOpen);
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    if (this.boundOnTooltipOpen) {
      document.removeEventListener("tooltip-open", this.boundOnTooltipOpen);
    }
    this.removeDocumentClickListener();
  }

  render() {
    return html`
      <button
        class="trigger"
        @click=${this.handleTriggerClick}
        aria-label="Help for ${this.settingKey}"
        aria-expanded=${this.visible}
      >
        ?
      </button>
      ${this.visible ? this.renderPopup() : null}
    `;
  }

  private renderPopup() {
    const content = this.buildContent();
    return html`
      <div class="popup" role="tooltip" aria-live="polite">
        <span class="popup-text">${content}</span>
      </div>
    `;
  }

  /** Build combined tooltip text (desc + effect + default), truncated to 200 chars */
  private buildContent(): string {
    const desc = t(`tooltip.${this.settingKey}.desc`);
    const effect = t(`tooltip.${this.settingKey}.effect`);
    const defaultVal = t(`tooltip.${this.settingKey}.default`);

    let combined = `${desc} ${effect} Default: ${defaultVal}`;

    if (combined.length > 200) {
      combined = combined.slice(0, 197) + "…";
    }

    return combined;
  }

  private handleTriggerClick(e: MouseEvent) {
    e.stopPropagation();

    if (this.visible) {
      this.hide();
      return;
    }

    this.show();
  }

  private show() {
    // Dispatch singleton event so other tooltips close
    document.dispatchEvent(new CustomEvent("tooltip-open", { detail: { key: this.settingKey } }));

    this.visible = true;
    this.addDocumentClickListener();
  }

  private hide() {
    this.visible = false;
    this.removeDocumentClickListener();
  }

  private handleTooltipOpen(e: Event) {
    const detail = (e as CustomEvent).detail;
    if (detail && detail.key !== this.settingKey && this.visible) {
      this.hide();
    }
  }

  private addDocumentClickListener() {
    // Defer to next microtask to avoid the same click closing immediately
    requestAnimationFrame(() => {
      this.boundOnDocumentClick = this.handleDocumentClick.bind(this);
      document.addEventListener("click", this.boundOnDocumentClick, { capture: true });
    });
  }

  private removeDocumentClickListener() {
    if (this.boundOnDocumentClick) {
      document.removeEventListener("click", this.boundOnDocumentClick, { capture: true });
      this.boundOnDocumentClick = null;
    }
  }

  private handleDocumentClick(e: MouseEvent) {
    const path = e.composedPath();
    if (!path.includes(this)) {
      this.hide();
    }
  }

  static styles = css`
    :host {
      display: inline-block;
      position: relative;
    }

    .trigger {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 44px;
      min-height: 44px;
      width: 28px;
      height: 28px;
      padding: 0;
      border: 1.5px solid var(--line, #d9e6f6);
      border-radius: 50%;
      background: var(--paper-tint, #f8fbff);
      color: var(--accent, #2e8eff);
      font:
        700 0.85rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      transition:
        background 0.12s,
        border-color 0.12s;
      -webkit-tap-highlight-color: transparent;
      touch-action: manipulation;
    }

    .trigger:hover,
    .trigger:focus-visible {
      background: rgba(46, 142, 255, 0.08);
      border-color: var(--accent, #2e8eff);
      outline: none;
    }

    .popup {
      position: absolute;
      bottom: calc(100% + 8px);
      left: 50%;
      transform: translateX(-50%);
      width: max-content;
      max-width: 260px;
      padding: 12px 14px;
      border: 1px solid var(--line, #d9e6f6);
      border-radius: 12px;
      background: var(--paper-tint, #ffffff);
      color: var(--ink-soft, #3d5278);
      font:
        0.88rem/1.5 ui-sans-serif,
        system-ui,
        sans-serif;
      box-shadow: 0 8px 24px rgba(0, 0, 0, 0.12);
      z-index: 200;
      animation: tooltip-show 0.15s ease-out;
    }

    .popup-text {
      display: block;
      font-size: 16px;
      line-height: 1.45;
      word-break: break-word;
    }

    @keyframes tooltip-show {
      from {
        opacity: 0;
        transform: translateX(-50%) translateY(4px);
      }
      to {
        opacity: 1;
        transform: translateX(-50%) translateY(0);
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "setting-tooltip": SettingTooltip;
  }
}
