import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { t, onLangChange } from "../i18n/index";
import { resetTutorial } from "../onboarding/onboarding-store";
import { HELP_CATEGORIES } from "../onboarding/help-data";

/**
 * Full help/guide page accessible from settings.
 *
 * Sections:
 * 1. "Szybki start" (Quick Start) — connect device, send book, start reading
 * 2. Categorized accordion list — Wyświetlanie, Czytanie, HUD, Język, Połączenie
 * 3. "Uruchom tutorial ponownie" button
 *
 * Accordion behavior: expanding one item collapses previous in the same
 * category (max 1 expanded per category).
 *
 * All text is served via i18n t() keys — works fully offline with static
 * bundle data, no fetch calls.
 */
@customElement("help-panel")
export class HelpPanel extends LitElement {
  /**
   * Track which item is expanded per category.
   * Key = category index, Value = item index (or -1 if none expanded).
   */
  @state() private expandedItems: Record<number, number> = {};

  private unsubLang: (() => void) | null = null;

  connectedCallback(): void {
    super.connectedCallback();
    this.unsubLang = onLangChange(() => this.requestUpdate());
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this.unsubLang?.();
  }

  render() {
    return html`
      <button class="back-btn" @click=${this.handleBack} aria-label="${t("help.btn.back")}">
        <svg
          width="20"
          height="20"
          viewBox="0 0 20 20"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
          aria-hidden="true"
        >
          <polyline points="12,4 6,10 12,16" />
        </svg>
        <span>${t("help.btn.back")}</span>
      </button>

      <section class="quick-start">
        <h2>${t("help.quickstart.title")}</h2>
        <ol class="steps">
          <li>${t("help.quickstart.connect")}</li>
          <li>${t("help.quickstart.sendBook")}</li>
          <li>${t("help.quickstart.startReading")}</li>
        </ol>
      </section>

      ${HELP_CATEGORIES.map(
        (category, catIdx) => html`
          <section class="category">
            <h3 class="category-title">${t(category.titleKey)}</h3>
            <div class="accordion">
              ${category.items.map((item, itemIdx) => {
                const isExpanded = this.expandedItems[catIdx] === itemIdx;
                return html`
                  <div class="accordion-item ${isExpanded ? "expanded" : ""}">
                    <button
                      class="accordion-header"
                      @click=${() => this.toggleItem(catIdx, itemIdx)}
                      aria-expanded=${isExpanded}
                    >
                      <span class="item-name">${t(item.nameKey)}</span>
                      <svg
                        class="chevron"
                        width="16"
                        height="16"
                        viewBox="0 0 16 16"
                        fill="none"
                        stroke="currentColor"
                        stroke-width="2"
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        aria-hidden="true"
                      >
                        <polyline points="4,6 8,10 12,6" />
                      </svg>
                    </button>
                    ${isExpanded
                      ? html`
                          <div class="accordion-body">
                            <p class="item-desc">${t(item.descKey)}</p>
                            <p class="item-values">${t(item.valuesKey)}</p>
                          </div>
                        `
                      : ""}
                  </div>
                `;
              })}
            </div>
          </section>
        `,
      )}

      <button class="restart-btn" @click=${this.handleRestartTutorial}>
        ${t("help.btn.restartTutorial")}
      </button>
    `;
  }

  /**
   * Toggle accordion item. Expanding one collapses the previous in the same category.
   */
  toggleItem(catIdx: number, itemIdx: number): void {
    const current = this.expandedItems[catIdx];
    this.expandedItems = {
      ...this.expandedItems,
      [catIdx]: current === itemIdx ? -1 : itemIdx,
    };
  }

  private handleBack(): void {
    this.dispatchEvent(new CustomEvent("help-close", { bubbles: true, composed: true }));
  }

  private handleRestartTutorial(): void {
    resetTutorial();
    this.dispatchEvent(new CustomEvent("restart-tutorial", { bubbles: true, composed: true }));
  }

  static styles = css`
    :host {
      display: flex;
      flex-direction: column;
      gap: 16px;
      font-family: ui-sans-serif, system-ui, sans-serif;
      font-size: 16px;
      color: var(--ink, #0c2340);
    }

    .back-btn {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 8px 12px;
      border: 0;
      border-radius: 12px;
      background: var(--paper-tint, #f5f8fc);
      color: var(--ink, #0c2340);
      font:
        600 0.9rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      align-self: flex-start;
      transition: background 0.15s;
    }
    .back-btn:hover {
      background: var(--line, #e2e8f0);
    }

    .quick-start {
      padding: 16px;
      border: 1px solid var(--line, #e2e8f0);
      border-radius: 16px;
      background: var(--paper-tint, #f5f8fc);
    }
    .quick-start h2 {
      margin: 0 0 12px 0;
      font-size: 1.1rem;
      font-weight: 700;
      color: var(--ink, #0c2340);
    }
    .steps {
      margin: 0;
      padding-left: 1.4rem;
      display: flex;
      flex-direction: column;
      gap: 8px;
      font-size: 0.95rem;
      line-height: 1.5;
      color: var(--ink-soft, #3d5278);
    }

    .category {
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .category-title {
      margin: 0;
      padding: 0 6px;
      font:
        700 0.78rem ui-sans-serif,
        system-ui,
        sans-serif;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      color: var(--muted, #6b7c97);
    }

    .accordion {
      border: 1px solid var(--line, #e2e8f0);
      border-radius: 16px;
      background: var(--paper-tint, #f5f8fc);
      overflow: hidden;
    }
    .accordion-item + .accordion-item {
      border-top: 1px solid var(--line, #e2e8f0);
    }

    .accordion-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      width: 100%;
      padding: 14px 16px;
      border: 0;
      background: transparent;
      color: var(--ink, #0c2340);
      font:
        0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      text-align: left;
    }
    .accordion-header:hover {
      background: rgba(0, 0, 0, 0.02);
    }
    .item-name {
      font-weight: 500;
    }
    .chevron {
      flex-shrink: 0;
      transition: transform 0.15s;
    }
    .accordion-item.expanded .chevron {
      transform: rotate(180deg);
    }

    .accordion-body {
      padding: 0 16px 14px 16px;
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .item-desc {
      margin: 0;
      font-size: 0.9rem;
      line-height: 1.5;
      color: var(--ink-soft, #3d5278);
    }
    .item-values {
      margin: 0;
      font-size: 0.82rem;
      color: var(--muted, #6b7c97);
      font-style: italic;
    }

    .restart-btn {
      padding: 14px 20px;
      border: 1px solid var(--accent, #2e8eff);
      border-radius: 16px;
      background: transparent;
      color: var(--accent, #2e8eff);
      font:
        600 0.95rem ui-sans-serif,
        system-ui,
        sans-serif;
      cursor: pointer;
      text-align: center;
      transition: background 0.15s;
    }
    .restart-btn:hover {
      background: rgba(46, 142, 255, 0.06);
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "help-panel": HelpPanel;
  }
}
