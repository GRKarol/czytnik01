import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { APP_URL, FIRMWARE_MANIFEST_URL } from "../shared/config";

@customElement("czytnik-flasher")
export class CzytnikFlasher extends LitElement {
  @state() private serialSupported = "serial" in navigator;
  @state() private isSecure = window.isSecureContext;

  render() {
    return html`
      <main>
        <section class="hero">
          <p class="eyebrow">Instalator firmware</p>
          <h1>Czytnik01</h1>
          <p class="lead">
            Podłącz urządzenie kablem USB i zainstaluj firmware z poziomu przeglądarki —
            bez VS Code, bez PlatformIO, bez terminala.
          </p>
        </section>

        <section class="install">
          <h2>Krok 1 — Zainstaluj firmware</h2>
          <p>Podłącz urządzenie i naciśnij przycisk poniżej.</p>

          <esp-web-install-button manifest=${FIRMWARE_MANIFEST_URL}>
            <button slot="activate" class="cta">Zainstaluj firmware</button>
            <span slot="unsupported">
              Twoja przeglądarka nie obsługuje Web Serial. Użyj Chrome lub Edge na desktopie.
            </span>
            <span slot="not-allowed">
              Strona musi być otwarta przez HTTPS lub localhost.
            </span>
          </esp-web-install-button>

          ${!this.serialSupported
            ? html`<p class="warning">
                Twoja przeglądarka nie obsługuje Web Serial. Otwórz tę stronę w Chrome lub Edge
                na komputerze.
              </p>`
            : ""}
          ${!this.isSecure
            ? html`<p class="warning">
                Strona nie jest otwarta po HTTPS. Flashowanie zadziała tylko z HTTPS lub
                localhost.
              </p>`
            : ""}
        </section>

        <section class="install">
          <h2>Krok 2 — Otwórz aplikację</h2>
          <p>
            Po zainstalowaniu firmware otwórz aplikację Czytnik01 i połącz się z urządzeniem.
            Aplikację możesz dodać do ekranu głównego telefonu (PWA).
          </p>
          <a class="cta cta--ghost" href=${APP_URL}>Przejdź do aplikacji</a>
        </section>

        <section class="steps">
          <h2>Zanim zaczniesz</h2>
          <ol>
            <li>Użyj Chrome lub Edge na macOS, Windows lub Linux.</li>
            <li>Podłącz urządzenie kablem USB z transmisją danych.</li>
            <li>
              Jeśli instalator nie może się połączyć, przytrzymaj <code>BOOT</code>, naciśnij
              reset i spróbuj ponownie.
            </li>
          </ol>
        </section>

        <footer>Czytnik01 &middot; web flasher</footer>
      </main>
    `;
  }

  static styles = css`
    :host {
      --ink: #e8e6e3;
      --muted: #9aa0a6;
      --bg: #0b0d10;
      --panel: #14181d;
      --accent: #ff7a45;
      --accent-deep: #c64f1f;
      --line: #2a2f36;
      display: block;
      min-height: 100vh;
      color: var(--ink);
      background:
        radial-gradient(circle at 14% 16%, rgba(255, 122, 69, 0.16), transparent 32rem),
        radial-gradient(circle at 88% 10%, rgba(198, 79, 31, 0.1), transparent 24rem),
        var(--bg);
      font-family:
        ui-sans-serif,
        system-ui,
        -apple-system,
        "Segoe UI",
        Roboto,
        sans-serif;
    }

    main {
      width: min(1040px, calc(100% - 32px));
      margin: 0 auto;
      padding: 52px 0 44px;
      display: grid;
      gap: 24px;
    }

    .hero {
      padding: clamp(28px, 6vw, 58px);
      border: 1px solid var(--line);
      border-radius: 28px;
      background: var(--panel);
    }

    .eyebrow {
      margin: 0 0 16px;
      color: var(--accent);
      font-weight: 700;
      font-size: 0.76rem;
      letter-spacing: 0.18em;
      text-transform: uppercase;
    }

    h1 {
      margin: 0;
      font-size: clamp(3rem, 9vw, 6.4rem);
      line-height: 0.9;
      letter-spacing: -0.04em;
    }

    .lead {
      max-width: 56ch;
      margin: 28px 0 0;
      color: var(--muted);
      font-size: clamp(1.05rem, 2vw, 1.32rem);
      line-height: 1.55;
    }

    .install,
    .steps {
      padding: 28px;
      border: 1px solid var(--line);
      border-radius: 24px;
      background: var(--panel);
    }

    .install h2,
    .steps h2 {
      margin: 0 0 10px;
      font-size: 1.2rem;
    }

    .install p,
    .steps p,
    .steps li {
      color: var(--muted);
      line-height: 1.5;
    }

    .cta {
      display: inline-block;
      margin-top: 16px;
      padding: 13px 22px;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: var(--accent);
      font: 800 0.95rem/1 inherit;
      text-decoration: none;
      cursor: pointer;
      box-shadow: 0 12px 26px rgba(255, 122, 69, 0.28);
    }

    .cta:hover {
      background: var(--accent-deep);
    }

    .cta--ghost {
      background: transparent;
      color: var(--accent);
      border: 1px solid var(--accent);
      box-shadow: none;
    }

    .warning {
      margin-top: 14px;
      padding: 12px 14px;
      border-left: 3px solid var(--accent);
      border-radius: 12px;
      background: rgba(255, 122, 69, 0.08);
      color: var(--accent);
      font-size: 0.92rem;
    }

    ol {
      padding-left: 1.35rem;
    }

    code {
      padding: 0.12em 0.35em;
      border-radius: 0.4em;
      background: rgba(255, 122, 69, 0.12);
      color: var(--accent);
    }

    footer {
      text-align: center;
      color: var(--muted);
      font-size: 0.9rem;
    }

    @media (max-width: 760px) {
      main {
        padding-top: 18px;
      }
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "czytnik-flasher": CzytnikFlasher;
  }
}
