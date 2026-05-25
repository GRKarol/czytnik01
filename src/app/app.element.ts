import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { APP_NAME, APP_VERSION } from "../shared/config";
import { SerialLink } from "./device/serial-link";
import "./components/install-prompt.element";

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
          Start
        </button>
        <button
          class=${this.view === "device" ? "active" : ""}
          @click=${() => (this.view = "device")}
          ?disabled=${!this.connected}
        >
          Urządzenie
        </button>
        <button
          class=${this.view === "settings" ? "active" : ""}
          @click=${() => (this.view = "settings")}
        >
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
        return html`<p class="placeholder">
          Tu pojawi się dashboard urządzenia — sterowanie, telemetria, funkcje.
        </p>`;
      case "settings":
        return html`<p class="placeholder">Ustawienia (placeholder).</p>`;
      default:
        return html`<p>?</p>`;
    }
  }

  private renderHome() {
    if (!SerialLink.isSupported()) {
      return html`
        <div class="card">
          <h2>Połącz urządzenie</h2>
          <p>
            Twoja przeglądarka nie wspiera Web Serial. Otwórz tę aplikację w Chrome lub Edge
            na desktopie albo w Chrome na Androidzie.
          </p>
        </div>
      `;
    }

    return html`
      <div class="card">
        <h2>Cześć!</h2>
        <p>Podłącz urządzenie i naciśnij przycisk poniżej, żeby się z nim połączyć.</p>
        ${this.connected
          ? html`<p class="ok">Połączono z urządzeniem.</p>
              <button class="cta" @click=${this.disconnect}>Rozłącz</button>`
          : html`<button class="cta" ?disabled=${this.connecting} @click=${this.connect}>
              ${this.connecting ? "Łączenie…" : "Połącz urządzenie"}
            </button>`}
        ${this.error ? html`<p class="error">${this.error}</p>` : ""}
      </div>
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
      --accent: #ff7a45;
      --accent-deep: #c64f1f;
      --ok: #4ade80;
      --err: #f87171;
      --line: #2a2f36;
      display: grid;
      grid-template-rows: auto 1fr auto;
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
      align-items: baseline;
      justify-content: space-between;
      padding: 16px 20px;
      border-bottom: 1px solid var(--line);
      padding-top: calc(16px + env(safe-area-inset-top));
    }

    header h1 {
      margin: 0;
      font-size: 1.2rem;
      letter-spacing: -0.01em;
    }

    .version {
      color: var(--muted);
      font-size: 0.78rem;
    }

    main {
      padding: 20px;
      overflow-y: auto;
    }

    .card {
      padding: 22px;
      border: 1px solid var(--line);
      border-radius: 20px;
      background: var(--panel);
      display: grid;
      gap: 12px;
    }

    .placeholder {
      color: var(--muted);
    }

    .cta {
      margin-top: 6px;
      padding: 14px 22px;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: var(--accent);
      font: 800 0.95rem/1 inherit;
      cursor: pointer;
    }

    .cta:hover {
      background: var(--accent-deep);
    }

    .cta:disabled {
      opacity: 0.6;
      cursor: progress;
    }

    .ok {
      color: var(--ok);
    }

    .error {
      color: var(--err);
    }

    nav {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      border-top: 1px solid var(--line);
      padding-bottom: env(safe-area-inset-bottom);
    }

    nav button {
      padding: 14px 0;
      background: transparent;
      border: 0;
      color: var(--muted);
      font: 600 0.85rem/1 inherit;
      cursor: pointer;
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
