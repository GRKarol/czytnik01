import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";

interface BeforeInstallPromptEvent extends Event {
  prompt(): Promise<void>;
  userChoice: Promise<{ outcome: "accepted" | "dismissed" }>;
}

const DISMISS_KEY = "czytnik01.installPrompt.dismissedAt";
const DISMISS_TTL_MS = 7 * 24 * 60 * 60 * 1000;

@customElement("czytnik-install-prompt")
export class InstallPrompt extends LitElement {
  @state() private deferred: BeforeInstallPromptEvent | null = null;
  @state() private visible = false;

  connectedCallback(): void {
    super.connectedCallback();
    window.addEventListener("beforeinstallprompt", this.onPrompt);
    window.addEventListener("appinstalled", this.onInstalled);
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    window.removeEventListener("beforeinstallprompt", this.onPrompt);
    window.removeEventListener("appinstalled", this.onInstalled);
  }

  private onPrompt = (e: Event) => {
    e.preventDefault();
    const ts = Number(localStorage.getItem(DISMISS_KEY) ?? 0);
    if (ts && Date.now() - ts < DISMISS_TTL_MS) return;
    this.deferred = e as BeforeInstallPromptEvent;
    this.visible = true;
  };

  private onInstalled = () => {
    this.deferred = null;
    this.visible = false;
  };

  private install = async () => {
    if (!this.deferred) return;
    await this.deferred.prompt();
    await this.deferred.userChoice;
    this.deferred = null;
    this.visible = false;
  };

  private dismiss = () => {
    localStorage.setItem(DISMISS_KEY, String(Date.now()));
    this.visible = false;
  };

  render() {
    if (!this.visible) return null;
    return html`
      <div class="bar">
        <span>Dodaj aplikację do ekranu głównego.</span>
        <div class="actions">
          <button @click=${this.dismiss}>Później</button>
          <button class="primary" @click=${this.install}>Zainstaluj</button>
        </div>
      </div>
    `;
  }

  static styles = css`
    .bar {
      position: sticky;
      top: 0;
      z-index: 10;
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      align-items: center;
      justify-content: space-between;
      padding: 10px 14px;
      background: #ff7a45;
      color: #1a0d05;
      font: 600 0.88rem/1.2 inherit;
    }
    .actions {
      display: flex;
      gap: 8px;
    }
    button {
      padding: 6px 12px;
      border: 0;
      border-radius: 999px;
      background: rgba(0, 0, 0, 0.15);
      color: inherit;
      font: inherit;
      cursor: pointer;
    }
    button.primary {
      background: #1a0d05;
      color: #ff7a45;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "czytnik-install-prompt": InstallPrompt;
  }
}
