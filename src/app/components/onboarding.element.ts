import { LitElement, css, html, svg } from "lit";
import { customElement, state } from "lit/decorators.js";
import { BRAND_NAME, DEVICE_LABEL } from "../../shared/config";

const STORAGE_KEY = "flower.onboarded.v1";

/**
 * Pełnoekranowy pierwszorazowy wizard. Pokazuje się raz po otwarciu
 * PWA. Trzy ekrany: powitanie / dodaj do ekranu głównego / połącz
 * urządzenie. Na iOS pokazujemy ilustrowaną instrukcję Share Sheet bo
 * Safari nie wspiera `beforeinstallprompt`.
 *
 * Po przejściu wszystkich ekranów zapisuje flagę w localStorage —
 * następnym razem wizard się nie pokaże.
 */
@customElement("onboarding-wizard")
export class OnboardingWizard extends LitElement {
  @state() private step = 0;
  @state() private dismissed = false;

  connectedCallback(): void {
    super.connectedCallback();
    this.dismissed = !!localStorage.getItem(STORAGE_KEY);
  }

  render() {
    if (this.dismissed) return null;
    return html`
      <div class="overlay">
        <div class="card">
          <div class="dots">
            ${[0, 1, 2].map(
              (i) => html`<span class=${i === this.step ? "dot active" : "dot"}></span>`,
            )}
          </div>

          <div class="stage">${this.renderStep()}</div>

          <div class="footer">
            ${this.step > 0
              ? html`<button class="link" @click=${() => (this.step -= 1)}>Wróć</button>`
              : html`<button class="link" @click=${this.skip}>Pomiń</button>`}
            <button class="cta" @click=${this.next}>
              ${this.step < 2 ? "Dalej" : "Zaczynamy"}
            </button>
          </div>
        </div>
      </div>
    `;
  }

  private renderStep() {
    switch (this.step) {
      case 0:
        return html`
          <div class="hero">${this.flower(120)}</div>
          <h2>Cześć, tu ${BRAND_NAME}.</h2>
          <p>
            Aplikacja Twojego ${DEVICE_LABEL.toLowerCase()}a. Stąd wysyłasz książki,
            zarządzasz ustawieniami i pobierasz pluginy. Bezprzewodowo, bez kabli.
          </p>
        `;
      case 1: {
        const isIos = /iPad|iPhone|iPod/.test(navigator.userAgent);
        return html`
          <div class="hero soft">${this.iconAdd()}</div>
          <h2>Dodaj do ekranu głównego</h2>
          ${isIos
            ? html`
                <p>
                  Otwórz menu udostępniania na dole Safari (ikona kwadratu ze strzałką)
                  i wybierz <strong>„Do ekranu początkowego"</strong>. Aplikacja zachowa się
                  jak natywna — bez paska adresu i bez tabów.
                </p>
              `
            : html`
                <p>
                  Twoja przeglądarka zapyta o instalację za chwilę. Możesz też zrobić
                  to ręcznie: menu (⋮) → <strong>„Zainstaluj aplikację"</strong>.
                </p>
              `}
        `;
      }
      case 2:
        return html`
          <div class="hero soft">${this.iconWifi()}</div>
          <h2>Połącz urządzenie</h2>
          <p>
            Na ekranie startowym wybierz <strong>WiFi</strong>. Telefon przełączy się
            na chwilę do sieci urządzenia (<code>${BRAND_NAME}-XXXX</code>) i zaczniecie
            się komunikować. iPhone i Android działają tak samo.
          </p>
          <p class="hint">
            Na razie aplikacja działa też bez urządzenia — pełen interfejs z mockowanymi
            danymi, żeby było co testować.
          </p>
        `;
    }
  }

  private next = () => {
    if (this.step < 2) {
      this.step += 1;
      return;
    }
    this.finish();
  };

  private skip = () => this.finish();

  private finish() {
    localStorage.setItem(STORAGE_KEY, new Date().toISOString());
    this.dismissed = true;
  }

  // ─── ikony ────────────────────────────────────────────────────────────────

  private flower(s: number) {
    return svg`
      <svg width=${s} height=${s} viewBox="0 0 100 100" aria-hidden="true">
        <g transform="translate(50 50)">
          ${[0, 60, 120, 180, 240, 300].map(
            (a) => svg`
            <ellipse cx="0" cy="-22" rx="14" ry="22" fill="currentColor" opacity="0.9"
                     transform="rotate(${a})"/>
          `,
          )}
          <circle r="10" fill="#fff2bf"/>
          <circle r="6" fill="#ffd66e"/>
        </g>
      </svg>
    `;
  }

  private iconAdd() {
    return svg`
      <svg width="90" height="90" viewBox="0 0 100 100" fill="none"
           stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
        <rect x="22" y="22" width="56" height="56" rx="12"/>
        <line x1="50" y1="38" x2="50" y2="62"/>
        <line x1="38" y1="50" x2="62" y2="50"/>
      </svg>
    `;
  }

  private iconWifi() {
    return svg`
      <svg width="100" height="100" viewBox="0 0 100 100" fill="none"
           stroke="currentColor" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">
        <path d="M14 38a48 48 0 0 1 72 0"/>
        <path d="M26 52a32 32 0 0 1 48 0"/>
        <path d="M38 66a16 16 0 0 1 24 0"/>
        <circle cx="50" cy="80" r="4" fill="currentColor"/>
      </svg>
    `;
  }

  static styles = css`
    :host {
      position: fixed;
      inset: 0;
      z-index: 100;
      pointer-events: none;
    }
    .overlay {
      position: absolute;
      inset: 0;
      display: grid;
      place-items: end center;
      padding: 12px;
      padding-bottom: calc(12px + env(safe-area-inset-bottom));
      background: rgba(12, 35, 64, 0.55);
      backdrop-filter: blur(8px);
      pointer-events: auto;
    }
    .card {
      width: 100%;
      max-width: 460px;
      padding: 24px 22px;
      padding-bottom: calc(20px + env(safe-area-inset-bottom));
      background: #ffffff;
      border-radius: 26px 26px 18px 18px;
      box-shadow: 0 -16px 40px rgba(0, 0, 0, 0.18);
      color: #0c2340;
      display: flex;
      flex-direction: column;
      gap: 14px;
      font-family:
        "Iowan Old Style", Georgia, ui-serif, serif;
    }
    .dots {
      display: flex;
      gap: 6px;
      justify-content: center;
      margin-bottom: 2px;
    }
    .dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: #d9e6f6;
      transition: width 0.2s;
    }
    .dot.active {
      width: 22px;
      border-radius: 999px;
      background: #2e8eff;
    }
    .stage {
      display: flex;
      flex-direction: column;
      align-items: center;
      text-align: center;
      gap: 12px;
      padding: 6px 0;
    }
    .hero {
      color: #2e8eff;
      filter: drop-shadow(0 12px 24px rgba(46, 142, 255, 0.28));
    }
    .hero.soft {
      color: #2e8eff;
      padding: 12px;
      border-radius: 24px;
      background: linear-gradient(135deg, #e8f4fd, #d1e9fb);
    }
    h2 {
      margin: 0;
      font-size: 1.6rem;
      letter-spacing: -0.02em;
    }
    p {
      margin: 0;
      max-width: 38ch;
      color: #3d5278;
      font: 1rem/1.55 ui-sans-serif, system-ui, sans-serif;
    }
    .hint {
      font-style: italic;
      font-size: 0.9rem;
      color: #6b7c97;
    }
    .footer {
      display: flex;
      gap: 10px;
      align-items: center;
      justify-content: space-between;
      padding-top: 4px;
    }
    .link {
      background: transparent;
      border: 0;
      color: #6b7c97;
      font: 600 0.9rem ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
      padding: 8px 4px;
    }
    .cta {
      padding: 12px 22px;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: #2e8eff;
      font: 700 0.95rem ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
      box-shadow: 0 10px 22px rgba(46, 142, 255, 0.28);
    }
    .cta:hover {
      background: #1f6fd4;
    }
    code {
      padding: 0.12em 0.38em;
      border-radius: 0.4em;
      background: #d1e9fb;
      color: #1f6fd4;
      font-family: ui-monospace, monospace;
      font-size: 0.9em;
    }
    strong {
      color: #0c2340;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "onboarding-wizard": OnboardingWizard;
  }
}
