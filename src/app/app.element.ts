import { LitElement, css, html, svg } from "lit";
import { customElement, state } from "lit/decorators.js";
import { APP_NAME, APP_VERSION } from "../shared/config";
import { SerialLink } from "./device/serial-link";
import "./components/install-prompt.element";

const iconHome = () => svg`
  <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 24 24"
       fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <path d="M3 11l9-8 9 8v10a2 2 0 0 1-2 2h-4v-7H10v7H5a2 2 0 0 1-2-2z"/>
  </svg>
`;
const iconDevice = (size = 22) => svg`
  <svg xmlns="http://www.w3.org/2000/svg" width=${size} height=${size} viewBox="0 0 24 24"
       fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <rect x="3" y="5" width="18" height="14" rx="3"/>
    <line x1="7" y1="10" x2="17" y2="10"/>
    <line x1="7" y1="14" x2="13" y2="14"/>
  </svg>
`;
const iconSettings = () => svg`
  <svg xmlns="http://www.w3.org/2000/svg" width="22" height="22" viewBox="0 0 24 24"
       fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
    <circle cx="12" cy="12" r="3"/>
    <path d="M19.4 15a1.7 1.7 0 0 0 .34 1.87l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.7 1.7 0 0 0-1.87-.34 1.7 1.7 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09a1.7 1.7 0 0 0-1.11-1.51 1.7 1.7 0 0 0-1.87.34l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.7 1.7 0 0 0 .34-1.87 1.7 1.7 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09A1.7 1.7 0 0 0 4.6 9a1.7 1.7 0 0 0-.34-1.87l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06A1.7 1.7 0 0 0 9 4.6a1.7 1.7 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09c0 .67.39 1.28 1 1.51a1.7 1.7 0 0 0 1.87-.34l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.7 1.7 0 0 0-.34 1.87c.23.61.84 1 1.51 1H21a2 2 0 1 1 0 4h-.09c-.67 0-1.28.39-1.51 1z"/>
  </svg>
`;

type View = "home" | "connect" | "device" | "settings";

@customElement("czytnik-app")
export class CzytnikApp extends LitElement {
  @state() private view: View = "home";
  @state() private connecting = false;
  @state() private connected = false;
  @state() private error: string | null = null;

  private link = new SerialLink();

  render() {
    return html`
      <czytnik-install-prompt></czytnik-install-prompt>

      <header>
        <h1>${APP_NAME}</h1>
        <span class="version">v${APP_VERSION}</span>
      </header>

      <main>${this.renderView()}</main>

      <nav>
        <button
          class=${this.view === "home" ? "active" : ""}
          @click=${() => (this.view = "home")}
        >
          <span class="ico">${iconHome()}</span>
          Start
        </button>
        <button
          class=${this.view === "device" ? "active" : ""}
          @click=${() => (this.view = "device")}
          ?disabled=${!this.connected}
        >
          <span class="ico">${iconDevice()}</span>
          Urządzenie
        </button>
        <button
          class=${this.view === "settings" ? "active" : ""}
          @click=${() => (this.view = "settings")}
        >
          <span class="ico">${iconSettings()}</span>
          Ustawienia
        </button>
      </nav>
    `;
  }

  private renderView() {
    switch (this.view) {
      case "home":
        return this.renderHome();
      case "device":
        return html`
          <div class="card">
            <h3>Dashboard</h3>
            <p class="placeholder">
              Tu pojawi się sterowanie, telemetria i funkcje urządzenia.
            </p>
          </div>
        `;
      case "settings":
        return html`
          <div class="card">
            <h3>Ustawienia</h3>
            <p class="placeholder">Placeholder — uzupełnimy razem z firmware.</p>
          </div>
        `;
      default:
        return html`<p>?</p>`;
    }
  }

  private renderHome() {
    const supported = SerialLink.isSupported();

    return html`
      <section class="hero">
        <div class="device-art">${iconDevice(48)}</div>
        <div class=${`status ${this.connected ? "connected" : ""}`}>
          <span class="dot"></span>
          ${this.connected ? "Połączono" : "Nie połączono"}
        </div>
        <h2>${APP_NAME}</h2>
        <p>
          ${this.connected
            ? "Urządzenie jest gotowe do pracy."
            : "Podłącz urządzenie kablem USB i naciśnij przycisk poniżej."}
        </p>
      </section>

      ${supported
        ? html`
            ${this.connected
              ? html`<button class="cta ghost" @click=${this.disconnect}>Rozłącz</button>`
              : html`<button
                  class="cta"
                  ?disabled=${this.connecting}
                  @click=${this.connect}
                >
                  ${this.connecting ? "Łączenie…" : "Połącz urządzenie"}
                </button>`}
          `
        : html`
            <div class="card">
              <h3>Brak wsparcia Web Serial</h3>
              <p class="placeholder">
                Otwórz aplikację w Chrome lub Edge na desktopie albo w Chrome na Androidzie.
              </p>
            </div>
          `}
      ${this.error ? html`<p class="error">${this.error}</p>` : ""}
    `;
  }

  private connect = async () => {
    this.error = null;
    this.connecting = true;
    try {
      await this.link.connect();
      this.connected = true;
      this.view = "device";
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    } finally {
      this.connecting = false;
    }
  };

  private disconnect = async () => {
    await this.link.disconnect();
    this.connected = false;
    this.view = "home";
  };

  static styles = css`
    :host {
      --ink: #e8e6e3;
      --muted: #9aa0a6;
      --bg: #0b0d10;
      --panel: #14181d;
      --panel-2: #1b2026;
      --accent: #ff7a45;
      --accent-deep: #c64f1f;
      --ok: #4ade80;
      --err: #f87171;
      --line: #2a2f36;
      display: flex;
      flex-direction: column;
      min-height: 100vh;
      min-height: 100dvh;
      color: var(--ink);
      background: var(--bg);
      font-family:
        ui-sans-serif,
        system-ui,
        -apple-system,
        "Segoe UI",
        Roboto,
        sans-serif;
    }

    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 14px 20px;
      border-bottom: 1px solid var(--line);
      padding-top: calc(14px + env(safe-area-inset-top));
      flex: 0 0 auto;
    }

    header h1 {
      margin: 0;
      font-size: 1.1rem;
      letter-spacing: -0.01em;
    }

    .version {
      color: var(--muted);
      font-size: 0.75rem;
    }

    main {
      flex: 1 1 auto;
      display: flex;
      flex-direction: column;
      gap: 16px;
      padding: 20px;
      overflow-y: auto;
      -webkit-overflow-scrolling: touch;
    }

    .hero {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 14px;
      padding: 28px 20px 22px;
      border: 1px solid var(--line);
      border-radius: 24px;
      background:
        radial-gradient(circle at 50% 0%, rgba(255, 122, 69, 0.18), transparent 60%),
        var(--panel);
      text-align: center;
    }

    .device-art {
      width: 96px;
      height: 96px;
      border-radius: 24px;
      background: linear-gradient(135deg, #232931 0%, #14181d 100%);
      border: 1px solid var(--line);
      display: grid;
      place-items: center;
      box-shadow: 0 12px 30px rgba(0, 0, 0, 0.4);
    }

    .device-art svg {
      width: 56px;
      height: 56px;
      color: var(--accent);
    }

    .status {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 6px 12px;
      border-radius: 999px;
      font-size: 0.82rem;
      font-weight: 600;
      background: rgba(154, 160, 166, 0.12);
      color: var(--muted);
    }

    .status.connected {
      background: rgba(74, 222, 128, 0.12);
      color: var(--ok);
    }

    .status .dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: currentColor;
      box-shadow: 0 0 8px currentColor;
    }

    .hero h2 {
      margin: 0;
      font-size: 1.6rem;
      letter-spacing: -0.02em;
    }

    .hero p {
      margin: 0;
      color: var(--muted);
      line-height: 1.5;
      max-width: 30ch;
    }

    .card {
      padding: 20px;
      border: 1px solid var(--line);
      border-radius: 20px;
      background: var(--panel);
      display: flex;
      flex-direction: column;
      gap: 12px;
    }

    .card h3 {
      margin: 0;
      font-size: 0.95rem;
      letter-spacing: 0.02em;
      text-transform: uppercase;
      color: var(--muted);
    }

    .placeholder {
      color: var(--muted);
      line-height: 1.5;
    }

    .cta {
      width: 100%;
      padding: 16px 22px;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: var(--accent);
      font: 800 1rem/1 inherit;
      cursor: pointer;
      box-shadow: 0 12px 26px rgba(255, 122, 69, 0.28);
    }

    .cta:hover {
      background: var(--accent-deep);
    }

    .cta.ghost {
      background: transparent;
      color: var(--accent);
      border: 1px solid var(--accent);
      box-shadow: none;
    }

    .cta:disabled {
      opacity: 0.6;
      cursor: progress;
    }

    .error {
      color: var(--err);
      font-size: 0.9rem;
    }

    nav {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      border-top: 1px solid var(--line);
      background: var(--panel);
      padding-bottom: env(safe-area-inset-bottom);
      flex: 0 0 auto;
    }

    nav button {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      padding: 12px 0 10px;
      background: transparent;
      border: 0;
      color: var(--muted);
      font: 600 0.72rem/1.1 inherit;
      cursor: pointer;
      letter-spacing: 0.03em;
    }

    nav button .ico {
      width: 22px;
      height: 22px;
      display: grid;
      place-items: center;
    }

    nav button.active {
      color: var(--accent);
    }

    nav button:disabled {
      opacity: 0.35;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "czytnik-app": CzytnikApp;
  }
}
