import { LitElement, css, html, svg } from "lit";
import { customElement, state } from "lit/decorators.js";
import { BRAND_NAME, DEVICE_LABEL, APP_VERSION } from "../shared/config";
import type { DeviceLink } from "./device/device-link";
import { WifiLink } from "./device/wifi-link";
import { BluetoothLink } from "./device/bluetooth-link";
import { SerialLink } from "./device/serial-link";
import "./components/install-prompt.element";
import "./components/flower-decor.element";
import "./components/converter-panel.element";
import "./components/updates-panel.element";
import "./components/library-panel.element";
import "./components/settings-panel.element";
import "./components/onboarding.element";
import { deviceApi, onDeviceApiChange, setDeviceApi, type DeviceSettings } from "./device/api";
import { HttpDeviceApi, pingDevice } from "./device/http-api";

type View = "home" | "library" | "converter" | "plugins" | "updates" | "settings";
type Transport = "wifi" | "bluetooth" | "serial";

const iconHome = (s = 24) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
    <path d="M3 11l9-8 9 8v10a2 2 0 0 1-2 2h-4v-7H10v7H5a2 2 0 0 1-2-2z"/>
  </svg>
`;
const iconBook = (s = 24) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
    <path d="M4 4h6a4 4 0 0 1 4 4v12a3 3 0 0 0-3-3H4z"/>
    <path d="M20 4h-6a4 4 0 0 0-4 4v12a3 3 0 0 1 3-3h7z"/>
  </svg>
`;
const iconConvert = (s = 24) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
    <path d="M5 9h11l-3-3"/>
    <path d="M19 15H8l3 3"/>
  </svg>
`;
const iconPlug = (s = 24) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
    <path d="M9 7V3M15 7V3"/>
    <rect x="7" y="7" width="10" height="8" rx="2"/>
    <path d="M12 15v4"/>
  </svg>
`;
const iconUpdate = (s = 24) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
    <path d="M21 12a9 9 0 1 1-3-6.7"/>
    <path d="M21 4v5h-5"/>
  </svg>
`;
const iconGear = (s = 24) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">
    <circle cx="12" cy="12" r="3"/>
    <path d="M19 12a7 7 0 0 0-.1-1.2l2-1.6-2-3.5-2.4.9a7 7 0 0 0-2-1.2L14 3h-4l-.5 2.4a7 7 0 0 0-2 1.2L5 5.7l-2 3.5 2 1.6A7 7 0 0 0 5 12c0 .4 0 .8.1 1.2l-2 1.6 2 3.5 2.4-.9a7 7 0 0 0 2 1.2L10 21h4l.5-2.4a7 7 0 0 0 2-1.2l2.4.9 2-3.5-2-1.6c.1-.4.1-.8.1-1.2z"/>
  </svg>
`;
const iconWifi = (s = 28) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
    <path d="M2 8.5a17 17 0 0 1 20 0"/>
    <path d="M5 12a13 13 0 0 1 14 0"/>
    <path d="M8.5 15.5a8 8 0 0 1 7 0"/>
    <circle cx="12" cy="19" r="1.2" fill="currentColor"/>
  </svg>
`;
const iconBt = (s = 28) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
    <path d="M7 7l10 10-5 4V3l5 4L7 17"/>
  </svg>
`;
const iconUsb = (s = 28) => svg`
  <svg width=${s} height=${s} viewBox="0 0 24 24" fill="none"
       stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
    <circle cx="12" cy="4" r="1.5"/>
    <path d="M12 5.5V20"/>
    <path d="M12 14l-4-4h3V8"/>
    <path d="M12 12l4-2h-3V8"/>
    <rect x="9" y="20" width="6" height="2" rx="1"/>
  </svg>
`;
const iconFlower = (s = 64) => svg`
  <svg width=${s} height=${s} viewBox="0 0 100 100" aria-hidden="true">
    <g transform="translate(50 50)">
      ${[0, 60, 120, 180, 240, 300].map(
        (a) => svg`
        <ellipse cx="0" cy="-22" rx="14" ry="22" fill="currentColor" opacity="0.85"
                 transform="rotate(${a})"/>
      `,
      )}
      <circle r="10" fill="#fff2bf"/>
      <circle r="6" fill="#ffd66e"/>
    </g>
  </svg>
`;

@customElement("czytnik-app")
export class CzytnikApp extends LitElement {
  @state() private view: View = "home";
  @state() private connecting = false;
  @state() private connected = false;
  @state() private error: string | null = null;
  @state() private chosenTransport: Transport | null = null;
  @state() private showAdvanced = false;
  @state() private devMode = false;

  private link: DeviceLink | null = null;
  private unsubApi: (() => void) | null = null;

  connectedCallback(): void {
    super.connectedCallback();
    this.refreshDevMode();
    this.unsubApi = onDeviceApiChange(() => this.refreshDevMode());
    this.addEventListener("device-settings-changed", () => this.refreshDevMode());
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this.unsubApi?.();
  }

  /** Sprawdza w API czy dev mode jest włączony — odświeża badge w header. */
  private async refreshDevMode(): Promise<void> {
    try {
      const s: DeviceSettings = await deviceApi.getSettings();
      this.devMode = s.devMode;
    } catch {
      this.devMode = false;
    }
  }

  render() {
    return html`
      <czytnik-install-prompt></czytnik-install-prompt>
      <onboarding-wizard></onboarding-wizard>

      <div class="sky">
        <flower-decor density="medium"></flower-decor>
      </div>

      <header>
        <div class="brand">
          <span class="flower">${iconFlower(28)}</span>
          <span>
            <strong>${BRAND_NAME}</strong>
            <small>v${APP_VERSION}</small>
          </span>
        </div>
        <div class="badges">
          ${this.devMode ? html`<span class="badge dev">DEV</span>` : ""}
          <div class=${`pill ${this.connected ? "ok" : ""}`}>
            <span class="dot"></span>
            ${this.connected ? `Połączono · ${this.link?.transport.label}` : "Brak połączenia"}
          </div>
        </div>
      </header>

      <main>${this.renderView()}</main>

      <nav>
        ${this.navButton("home", "Start", iconHome())}
        ${this.navButton("library", "Książki", iconBook())}
        ${this.navButton("converter", "Konwerter", iconConvert())}
        ${this.navButton("plugins", "Pluginy", iconPlug())}
        ${this.navButton("updates", "Aktualizacje", iconUpdate())}
        ${this.navButton("settings", "Więcej", iconGear())}
      </nav>
    `;
  }

  private navButton(v: View, label: string, ico: unknown, disabled = false) {
    return html`
      <button
        class=${this.view === v ? "active" : ""}
        ?disabled=${disabled}
        @click=${() => (this.view = v)}
      >
        <span class="ico">${ico}</span>
        ${label}
      </button>
    `;
  }

  private renderView() {
    switch (this.view) {
      case "home":
        return this.renderHome();
      case "library":
        return this.renderLibrary();
      case "converter":
        return this.renderConverter();
      case "plugins":
        return this.renderPlugins();
      case "updates":
        return this.renderUpdates();
      case "settings":
        return this.renderSettings();
    }
  }

  // ─── Home ──────────────────────────────────────────────────────────────────

  private renderHome() {
    return html`
      <section class="hero">
        <div class="hero-flower">${iconFlower(96)}</div>
        <h2>Cześć!</h2>
        <p>
          To aplikacja Twojego ${DEVICE_LABEL.toLowerCase()}a <strong>${BRAND_NAME}</strong>.
          Wysyłaj książki, instaluj pluginy i aktualizuj urządzenie — wszystko bezprzewodowo.
        </p>
      </section>

      ${!this.connected ? this.renderConnectChoice() : this.renderConnectedActions()}
      ${this.error ? html`<p class="error">${this.error}</p>` : ""}
    `;
  }

  private renderConnectChoice() {
    if (this.chosenTransport) return this.renderConnecting();

    const btSupported = BluetoothLink.isSupported();
    const serialSupported = SerialLink.isSupported();

    return html`
      <section class="card">
        <h3>Połącz urządzenie</h3>
        <p class="muted">Wybierz sposób połączenia z ${DEVICE_LABEL.toLowerCase()}em.</p>

        <button class="choice" @click=${() => this.pickTransport("wifi")}>
          <span class="choice-ico">${iconWifi()}</span>
          <span class="choice-body">
            <strong>WiFi</strong>
            <span>Polecane. Działa na iPhonie i Androidzie. Szybki transfer książek.</span>
          </span>
        </button>

        <button
          class="choice"
          ?disabled=${!btSupported}
          @click=${() => this.pickTransport("bluetooth")}
        >
          <span class="choice-ico">${iconBt()}</span>
          <span class="choice-body">
            <strong>Bluetooth</strong>
            <span>
              ${btSupported
                ? "Bonus dla Androida. Idealny do drobnych komend."
                : "Niewspierany w tej przeglądarce (iOS nie obsługuje Web Bluetooth)."}
            </span>
          </span>
        </button>

        <button class="link-button" @click=${() => (this.showAdvanced = !this.showAdvanced)}>
          ${this.showAdvanced ? "Schowaj tryb zaawansowany" : "Tryb zaawansowany"}
        </button>

        ${this.showAdvanced
          ? html`
              <button
                class="choice subtle"
                ?disabled=${!serialSupported}
                @click=${() => this.pickTransport("serial")}
              >
                <span class="choice-ico">${iconUsb()}</span>
                <span class="choice-body">
                  <strong>USB</strong>
                  <span>
                    ${serialSupported
                      ? "Diagnostyka / serwis. Wymaga kabla USB-C i Chrome/Edge na desktopie."
                      : "Web Serial niewspierany — użyj Chrome lub Edge na desktopie."}
                  </span>
                </span>
              </button>
            `
          : ""}
      </section>
    `;
  }

  private renderConnecting() {
    const label =
      this.chosenTransport === "wifi"
        ? "WiFi"
        : this.chosenTransport === "bluetooth"
          ? "Bluetooth"
          : "USB";
    return html`
      <section class="card">
        <h3>Łączenie przez ${label}…</h3>
        ${this.chosenTransport === "wifi"
          ? html`
              <ol class="steps">
                <li>Włącz urządzenie i poczekaj, aż wyświetli kod sieci.</li>
                <li>
                  Wejdź w ustawienia WiFi telefonu i wybierz
                  <code>Flower-…</code>.
                </li>
                <li>Wróć tutaj i naciśnij „Sprawdź połączenie".</li>
              </ol>
            `
          : this.chosenTransport === "bluetooth"
            ? html`<p class="muted">Wybierz urządzenie w okienku przeglądarki.</p>`
            : html`<p class="muted">Wybierz port USB w okienku przeglądarki.</p>`}

        <div class="row">
          <button class="cta" ?disabled=${this.connecting} @click=${this.connect}>
            ${this.connecting ? "Łączenie…" : "Sprawdź połączenie"}
          </button>
          <button class="cta ghost" @click=${this.cancelChoice}>Wróć</button>
        </div>
      </section>
    `;
  }

  private renderConnectedActions() {
    return html`
      <section class="card">
        <h3>Co robimy?</h3>
        <div class="grid">
          <button class="tile" @click=${() => (this.view = "library")}>
            <span class="tile-ico">${iconBook(28)}</span>
            <strong>Książki</strong>
            <span>Wyślij nowe, zarządzaj biblioteką.</span>
          </button>
          <button class="tile" @click=${() => (this.view = "converter")}>
            <span class="tile-ico">${iconConvert(28)}</span>
            <strong>Konwerter</strong>
            <span>EPUB · PDF · MOBI · TXT → .rsvp</span>
          </button>
          <button class="tile" @click=${() => (this.view = "plugins")}>
            <span class="tile-ico">${iconPlug(28)}</span>
            <strong>Pluginy</strong>
            <span>Klepsydra, dyktafon i więcej.</span>
          </button>
          <button class="tile" @click=${() => (this.view = "updates")}>
            <span class="tile-ico">${iconUpdate(28)}</span>
            <strong>Aktualizacje</strong>
            <span>Sprawdź nowe wersje firmware.</span>
          </button>
        </div>
        <button class="cta ghost" @click=${this.disconnect}>Rozłącz</button>
      </section>
    `;
  }

  // ─── Pozostałe ekrany — szkielety, logika idzie w kolejnych rundach ─────

  private renderLibrary() {
    return html`
      <section class="card">
        <h3>${iconBook(22)} Książki</h3>
        <library-panel></library-panel>
      </section>
    `;
  }

  private renderConverter() {
    return html`
      <section class="card">
        <h3>${iconConvert(22)} Konwerter</h3>
        <p class="muted">
          Wybierz plik z telefonu — przekonwertujemy go na format
          <code>.rsvp</code> w przeglądarce, bez wysyłania nigdzie.
        </p>
        <converter-panel></converter-panel>
      </section>
    `;
  }

  private renderPlugins() {
    return html`
      <section class="card">
        <h3>${iconPlug(22)} Pluginy</h3>
        <p class="muted">
          Dodatkowe funkcje, które możesz wgrać na urządzenie — klepsydra, dyktafon,
          odtwarzacz muzyki, itd. Nowe pluginy pojawiają się tu sukcesywnie.
        </p>
        <div class="plugin-list">
          ${this.pluginCard("Klepsydra", "Sesja czytania z timerem 25/5.", "Gotowy")}
          ${this.pluginCard("Dyktafon", "Notatki głosowe podczas czytania.", "Wkrótce")}
          ${this.pluginCard("Odtwarzacz muzyki", "Cicha muzyka tła z SD.", "Wkrótce")}
        </div>
      </section>
    `;
  }

  private pluginCard(name: string, tagline: string, badge: string) {
    return html`
      <div class="plugin">
        <span class="plugin-ico">${iconFlower(36)}</span>
        <div class="plugin-body">
          <strong>${name}</strong>
          <span>${tagline}</span>
        </div>
        <span class=${`badge ${badge === "Gotowy" ? "ok" : ""}`}>${badge}</span>
      </div>
    `;
  }

  private renderUpdates() {
    return html`
      <section class="card">
        <h3>${iconUpdate(22)} Aktualizacje</h3>
        <updates-panel></updates-panel>
      </section>
    `;
  }

  private renderSettings() {
    return html`
      <section class="card">
        <h3>${iconGear(22)} Więcej</h3>
        <settings-panel></settings-panel>
        <ul class="settings-list">
          <li><strong>Wersja aplikacji</strong><span>${APP_VERSION}</span></li>
          <li><strong>Marka</strong><span>${BRAND_NAME}</span></li>
          <li>
            <strong>Połączenie</strong>
            <span>${this.link?.transport.label ?? "—"}</span>
          </li>
        </ul>
      </section>
    `;
  }

  // ─── Logika połączenia ─────────────────────────────────────────────────────

  private pickTransport(t: Transport) {
    this.chosenTransport = t;
    this.error = null;
  }

  private cancelChoice = () => {
    this.chosenTransport = null;
    this.error = null;
  };

  private connect = async () => {
    if (!this.chosenTransport) return;
    this.error = null;
    this.connecting = true;
    try {
      this.link =
        this.chosenTransport === "wifi"
          ? new WifiLink()
          : this.chosenTransport === "bluetooth"
            ? new BluetoothLink()
            : new SerialLink();
      await this.link.connect();
      this.connected = true;
      // Przełącz API komponentów na real HTTP — biblioteka i ustawienia
      // od teraz gadają z urządzeniem zamiast z mockiem.
      // (BLE i USB jeszcze nie mają back-end API, więc dopiero WiFi to robi.)
      if (this.chosenTransport === "wifi") {
        const reachable = await pingDevice();
        if (reachable) setDeviceApi(new HttpDeviceApi());
      }
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
      this.link = null;
    } finally {
      this.connecting = false;
    }
  };

  private disconnect = async () => {
    await this.link?.disconnect();
    this.link = null;
    this.connected = false;
    this.chosenTransport = null;
    this.view = "home";
    // Wróć do mocka — szybkie testy bez podłączonego urządzenia dalej działają.
    const { MockDeviceApi } = await import("./device/api");
    setDeviceApi(new MockDeviceApi());
  };

  // ─── Style ─────────────────────────────────────────────────────────────────

  static styles = css`
    :host {
      --ink: #0c2340;
      --ink-soft: #3d5278;
      --muted: #6b7c97;
      --sky-1: #e8f4fd;
      --sky-2: #d1e9fb;
      --sky-3: #b8dcff;
      --paper: #ffffff;
      --paper-tint: #fbfcff;
      --accent: #2e8eff;
      --accent-deep: #1f6fd4;
      --bloom-yellow: #ffd66e;
      --bloom-pink: #ffb6c8;
      --ok: #2dbe75;
      --err: #e44d65;
      --line: #d9e6f6;
      --shadow: 0 12px 28px rgba(46, 142, 255, 0.14);
      display: flex;
      flex-direction: column;
      position: relative;
      min-height: 100vh;
      min-height: 100dvh;
      color: var(--ink);
      font-family:
        "Iowan Old Style", "Hoefler Text", "Georgia", "Times New Roman", ui-serif, serif;
      background:
        radial-gradient(circle at 18% 4%, rgba(255, 214, 110, 0.35), transparent 36rem),
        radial-gradient(circle at 90% 12%, rgba(255, 182, 200, 0.3), transparent 30rem),
        linear-gradient(160deg, var(--sky-1) 0%, var(--sky-2) 55%, var(--sky-3) 100%);
    }

    .sky {
      position: fixed;
      inset: 0;
      z-index: 0;
      pointer-events: none;
    }

    header,
    main,
    nav {
      position: relative;
      z-index: 1;
    }

    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
      padding: 16px 18px;
      padding-top: calc(16px + env(safe-area-inset-top));
      border-bottom: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.65);
      backdrop-filter: blur(12px);
      flex: 0 0 auto;
    }

    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .brand .flower {
      color: var(--accent);
      display: grid;
      place-items: center;
    }

    .brand strong {
      font-family: "Iowan Old Style", "Hoefler Text", Georgia, ui-serif, serif;
      font-size: 1.25rem;
      letter-spacing: -0.01em;
      line-height: 1;
      display: block;
    }

    .brand small {
      font-family: ui-sans-serif, system-ui, sans-serif;
      font-size: 0.7rem;
      color: var(--muted);
      letter-spacing: 0.06em;
    }

    .badges {
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .badge {
      padding: 4px 8px;
      border-radius: 6px;
      font: 800 0.7rem/1 ui-sans-serif, system-ui, sans-serif;
      letter-spacing: 0.1em;
    }
    .badge.dev {
      background: linear-gradient(135deg, #ff7a45, #ff9b73);
      color: #fff;
      box-shadow: 0 4px 10px rgba(255, 122, 69, 0.3);
    }
    .pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 5px 10px;
      border-radius: 999px;
      background: rgba(107, 124, 151, 0.12);
      color: var(--muted);
      font: 600 0.74rem/1 ui-sans-serif, system-ui, sans-serif;
      letter-spacing: 0.02em;
    }
    .pill.ok {
      background: rgba(45, 190, 117, 0.14);
      color: var(--ok);
    }
    .pill .dot {
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: currentColor;
      box-shadow: 0 0 6px currentColor;
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
      text-align: center;
      gap: 10px;
      padding: 16px 20px 4px;
    }

    .hero-flower {
      color: var(--accent);
      filter: drop-shadow(0 8px 20px rgba(46, 142, 255, 0.25));
    }

    .hero h2 {
      margin: 0;
      font-family: "Iowan Old Style", "Hoefler Text", Georgia, ui-serif, serif;
      font-size: 2rem;
      letter-spacing: -0.02em;
    }

    .hero p {
      margin: 0;
      color: var(--ink-soft);
      max-width: 36ch;
      line-height: 1.5;
      font-size: 1rem;
    }

    .card {
      display: flex;
      flex-direction: column;
      gap: 12px;
      padding: 20px;
      border: 1px solid var(--line);
      border-radius: 22px;
      background: var(--paper);
      box-shadow: var(--shadow);
    }

    .card h3 {
      display: flex;
      align-items: center;
      gap: 8px;
      margin: 0;
      font-family: "Iowan Old Style", "Hoefler Text", Georgia, ui-serif, serif;
      font-size: 1.2rem;
      color: var(--ink);
    }

    .muted {
      color: var(--muted);
      line-height: 1.5;
      margin: 0;
      font-family: ui-sans-serif, system-ui, sans-serif;
    }

    code {
      font-family: ui-monospace, SFMono-Regular, monospace;
      padding: 0.1em 0.4em;
      border-radius: 0.4em;
      background: var(--sky-2);
      color: var(--accent-deep);
      font-size: 0.92em;
    }

    .choice {
      display: flex;
      align-items: center;
      gap: 14px;
      padding: 14px;
      border: 1px solid var(--line);
      border-radius: 16px;
      background: var(--paper-tint);
      color: var(--ink);
      font: inherit;
      cursor: pointer;
      text-align: left;
      transition: border-color 0.15s ease;
    }
    .choice:hover:not(:disabled) {
      border-color: var(--accent);
    }
    .choice:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .choice.subtle {
      background: transparent;
    }
    .choice-ico {
      flex: 0 0 auto;
      width: 44px;
      height: 44px;
      display: grid;
      place-items: center;
      border-radius: 12px;
      background: var(--sky-2);
      color: var(--accent);
    }
    .choice-body {
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    .choice-body strong {
      font-family: "Iowan Old Style", "Hoefler Text", Georgia, ui-serif, serif;
      font-size: 1.05rem;
    }
    .choice-body span {
      font-family: ui-sans-serif, system-ui, sans-serif;
      font-size: 0.85rem;
      color: var(--muted);
      line-height: 1.4;
    }

    .link-button {
      align-self: flex-start;
      background: transparent;
      border: 0;
      color: var(--accent);
      padding: 4px 0;
      cursor: pointer;
      font: 600 0.88rem/1 ui-sans-serif, system-ui, sans-serif;
    }

    .steps {
      margin: 0;
      padding-left: 1.2rem;
      color: var(--ink-soft);
      font-family: ui-sans-serif, system-ui, sans-serif;
      line-height: 1.5;
    }

    .row {
      display: flex;
      gap: 10px;
    }
    .row .cta {
      flex: 1 1 0;
    }

    .cta {
      padding: 14px 20px;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: var(--accent);
      font: 700 0.98rem/1 ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
      box-shadow: 0 10px 22px rgba(46, 142, 255, 0.28);
    }
    .cta:hover {
      background: var(--accent-deep);
    }
    .cta:disabled {
      opacity: 0.55;
      cursor: not-allowed;
      box-shadow: none;
    }
    .cta.ghost {
      background: transparent;
      color: var(--accent);
      border: 1.5px solid var(--accent);
      box-shadow: none;
    }

    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }

    .tile {
      display: flex;
      flex-direction: column;
      align-items: flex-start;
      gap: 4px;
      padding: 14px;
      border: 1px solid var(--line);
      border-radius: 16px;
      background: var(--paper-tint);
      color: var(--ink);
      cursor: pointer;
      text-align: left;
      font: inherit;
      min-height: 100px;
    }
    .tile:hover {
      border-color: var(--accent);
    }
    .tile-ico {
      width: 36px;
      height: 36px;
      display: grid;
      place-items: center;
      border-radius: 10px;
      background: var(--sky-2);
      color: var(--accent);
      margin-bottom: 4px;
    }
    .tile strong {
      font-family: "Iowan Old Style", Georgia, ui-serif, serif;
      font-size: 1rem;
    }
    .tile span {
      font-family: ui-sans-serif, system-ui, sans-serif;
      font-size: 0.78rem;
      color: var(--muted);
      line-height: 1.35;
    }

    .plugin-list {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .plugin {
      display: flex;
      align-items: center;
      gap: 12px;
      padding: 12px;
      border: 1px solid var(--line);
      border-radius: 14px;
      background: var(--paper-tint);
    }
    .plugin-ico {
      color: var(--bloom-pink);
      flex: 0 0 auto;
    }
    .plugin-body {
      flex: 1 1 auto;
      display: flex;
      flex-direction: column;
      gap: 1px;
    }
    .plugin-body strong {
      font-family: "Iowan Old Style", Georgia, ui-serif, serif;
      font-size: 1rem;
    }
    .plugin-body span {
      font-family: ui-sans-serif, system-ui, sans-serif;
      font-size: 0.82rem;
      color: var(--muted);
    }
    .badge {
      padding: 3px 9px;
      border-radius: 999px;
      background: var(--sky-2);
      color: var(--ink-soft);
      font: 600 0.7rem/1 ui-sans-serif, system-ui, sans-serif;
    }
    .badge.ok {
      background: rgba(45, 190, 117, 0.18);
      color: var(--ok);
    }

    .settings-list {
      list-style: none;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 0;
    }
    .settings-list li {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 12px 0;
      border-bottom: 1px solid var(--line);
      font-family: ui-sans-serif, system-ui, sans-serif;
    }
    .settings-list li:last-child {
      border-bottom: 0;
    }
    .settings-list strong {
      font-weight: 600;
    }
    .settings-list span {
      color: var(--muted);
    }

    .error {
      color: var(--err);
      font-size: 0.9rem;
      font-family: ui-sans-serif, system-ui, sans-serif;
    }

    nav {
      display: grid;
      grid-template-columns: repeat(6, 1fr);
      border-top: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.85);
      backdrop-filter: blur(12px);
      padding-bottom: env(safe-area-inset-bottom);
      flex: 0 0 auto;
    }
    nav button {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 3px;
      padding: 9px 0 8px;
      background: transparent;
      border: 0;
      color: var(--muted);
      font: 600 0.65rem/1.1 ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
      letter-spacing: 0.02em;
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
      opacity: 0.3;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "czytnik-app": CzytnikApp;
  }
}
