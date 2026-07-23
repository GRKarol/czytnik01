import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import {
  fetchLatestRelease,
  pickFirmwareAsset,
  downloadAsset,
  isNewer,
  type ReleaseInfo,
} from "../updates/releases";
import { deviceApi, onDeviceApiChange, isDeviceConnected } from "../device/api";
import { getFirmwareVersion, onFirmwareVersionChange } from "../device/device-info";
import { pinToDeviceNetwork, unpinFromDeviceNetwork } from "../device/network-pin";

type Stage =
  | "idle"
  | "checking"
  | "found"
  | "none"
  | "error"
  | "downloading"
  | "downloaded"
  | "installing"
  | "installed";

@customElement("updates-panel")
export class UpdatesPanel extends LitElement {
  @state() private stage: Stage = "idle";
  @state() private release: ReleaseInfo | null = null;
  @state() private error = "";
  @state() private progress = 0;
  /** Aktualna wersja firmware na urządzeniu — z /api/hello (device-info.ts). */
  @state() private currentFw: string | null = getFirmwareVersion();
  /** Czy appka jest właśnie realnie połączona z czytnikiem (nie mock). */
  @state() private connected = isDeviceConnected();
  private downloaded: Blob | null = null;
  private unsubFw: (() => void) | null = null;
  private unsubApi: (() => void) | null = null;

  connectedCallback(): void {
    super.connectedCallback();
    this.unsubFw = onFirmwareVersionChange((v) => (this.currentFw = v));
    this.unsubApi = onDeviceApiChange(() => {
      this.connected = isDeviceConnected();
    });
    void this.check();
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this.unsubFw?.();
    this.unsubApi?.();
  }

  render() {
    return html`
      <div class="head">
        <strong>Aktualizacje firmware</strong>
      </div>

      ${this.renderStage()}
    `;
  }

  private renderStage() {
    switch (this.stage) {
      case "idle":
      case "checking":
        return html`<p class="muted">Sprawdzam dostępność aktualizacji…</p>`;
      case "none":
        return html`
          <p class="muted">Brak dostępnych aktualizacji.</p>
          <button class="cta ghost" @click=${this.check}>Sprawdź ponownie</button>
        `;
      case "error":
        return html`
          <p class="error">${this.error}</p>
          <button class="cta ghost" @click=${this.check}>Spróbuj ponownie</button>
        `;
      case "found":
      case "downloading":
      case "downloaded":
      case "installing":
      case "installed":
        return this.renderRelease();
    }
  }

  private renderRelease() {
    const r = this.release!;
    const asset = pickFirmwareAsset(r);
    const tag = r.tag.replace(/^v/, "");
    const newer = this.currentFw ? isNewer(tag, this.currentFw) : true;
    const date = new Date(r.publishedAt).toLocaleDateString("pl-PL");
    const changelog = trimChangelog(r.body);

    return html`
      <article class="release">
        <header>
          <div>
            <h4>${r.name}</h4>
            <small>${tag} · ${date}</small>
          </div>
          ${newer
            ? html`<span class="badge ok">Dostępna</span>`
            : html`<span class="badge">Aktualna</span>`}
        </header>

        ${changelog
          ? html`<pre class="changelog">${changelog}</pre>`
          : html`<p class="muted">Brak opisu wersji.</p>`}

        ${asset
          ? html`
              <div class="asset">
                <span><strong>${asset.name}</strong> · ${formatBytes(asset.size)}</span>
                ${this.stage === "downloading" || this.stage === "installing"
                  ? html`<progress max="100" value=${this.progress}></progress>`
                  : ""}
                ${this.stage === "downloaded"
                  ? html`<span class="ok">Pobrano</span>`
                  : ""}
                ${this.stage === "installed"
                  ? html`<span class="ok">Zainstalowano · urządzenie się restartuje</span>`
                  : ""}
              </div>
              <div class="row">
                ${this.stage === "found"
                  ? html`<button class="cta" @click=${() => this.download(asset)}>
                      Pobierz na telefon
                    </button>`
                  : ""}
                ${this.stage === "downloaded"
                  ? html`
                      ${this.connected
                        ? html`<button class="cta" @click=${this.install}>
                            Wgraj na czytnik
                          </button>`
                        : html`<p class="hint">
                            Połącz telefon z czytnikiem (WiFi, tryb Sync), żeby wgrać —
                            czytnika nie trzeba łączyć z internetem domowym.
                          </p>`}
                      <button class="cta ghost" @click=${this.savePhone}>
                        Zapisz plik na telefonie
                      </button>
                    `
                  : ""}
                ${this.stage === "installing"
                  ? html`<span class="muted">Wysyłam ${this.progress}%…</span>`
                  : ""}
              </div>
            `
          : html`<p class="muted">Brak pliku do wgrania w tej wersji.</p>`}
      </article>
    `;
  }

  // ─── actions ──────────────────────────────────────────────────────────────

  private check = async () => {
    this.error = "";
    this.stage = "checking";
    // Gdy telefon jest połączony z czytnikiem, NetworkPin (Android) kieruje
    // CAŁY ruch appki przez sieć czytnika — a ta nie ma internetu, więc
    // GitHub API nigdy by nie odpowiedziało ("Failed to fetch"). Na czas
    // tego zapytania (i tylko na ten czas) odpinamy się od sieci czytnika.
    await unpinFromDeviceNetwork();
    try {
      const r = await fetchLatestRelease();
      if (!r) {
        this.stage = "none";
        return;
      }
      this.release = r;
      this.stage = "found";
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
      this.stage = "error";
    } finally {
      if (this.connected) void pinToDeviceNetwork();
    }
  };

  private download = async (asset: ReturnType<typeof pickFirmwareAsset>) => {
    if (!asset) return;
    this.stage = "downloading";
    this.progress = 0;
    // Ten sam powód co w check() — pobieranie assetu leci do GitHuba, nie
    // do czytnika, więc musi iść przez zwykły internet telefonu.
    await unpinFromDeviceNetwork();
    try {
      this.downloaded = await downloadAsset(asset, (loaded, total) => {
        this.progress = total ? Math.round((loaded / total) * 100) : 0;
      });
      this.stage = "downloaded";
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
      this.stage = "error";
    } finally {
      if (this.connected) void pinToDeviceNetwork();
    }
  };

  private savePhone = () => {
    if (!this.downloaded || !this.release) return;
    const asset = pickFirmwareAsset(this.release);
    if (!asset) return;
    const url = URL.createObjectURL(this.downloaded);
    const a = document.createElement("a");
    a.href = url;
    a.download = asset.name;
    a.click();
    URL.revokeObjectURL(url);
  };

  private install = async () => {
    if (!this.downloaded) return;
    this.stage = "installing";
    this.progress = 0;
    this.error = "";
    try {
      await deviceApi.installOta(this.downloaded, (loaded, total) => {
        this.progress = total ? Math.round((loaded / total) * 100) : 0;
      });
      this.stage = "installed";
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
      this.stage = "error";
    }
  };

  static styles = css`
    :host {
      display: block;
      display: flex;
      flex-direction: column;
      gap: 12px;
    }
    .head {
      display: flex;
      flex-direction: column;
      gap: 2px;
    }
    .head strong {
      font-family: "Iowan Old Style", Georgia, ui-serif, serif;
      font-size: 1rem;
    }
    .muted {
      color: var(--muted);
      font: 0.9rem/1.5 ui-sans-serif, system-ui, sans-serif;
      margin: 0;
    }
    .error {
      color: var(--err);
      font: 0.9rem ui-sans-serif, system-ui, sans-serif;
      margin: 0;
    }
    .hint {
      color: var(--muted);
      font: 0.85rem/1.4 ui-sans-serif, system-ui, sans-serif;
      margin: 0;
      flex: 1 1 100%;
    }
    .cta {
      padding: 12px 18px;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: var(--accent);
      font: 700 0.92rem ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
    }
    .cta:hover {
      background: var(--accent-deep);
    }
    .cta.ghost {
      background: transparent;
      color: var(--accent);
      border: 1.5px solid var(--accent);
    }
    .cta:disabled {
      opacity: 0.55;
      cursor: not-allowed;
    }
    .release {
      display: flex;
      flex-direction: column;
      gap: 10px;
      padding: 14px;
      border: 1px solid var(--line);
      border-radius: 16px;
      background: var(--paper-tint);
    }
    .release header {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      gap: 12px;
    }
    .release h4 {
      margin: 0;
      font-family: "Iowan Old Style", Georgia, ui-serif, serif;
      font-size: 1.05rem;
    }
    .release small {
      color: var(--muted);
      font: 0.78rem ui-sans-serif, system-ui, sans-serif;
    }
    .badge {
      padding: 3px 9px;
      border-radius: 999px;
      background: var(--sky-2);
      color: var(--ink-soft);
      font: 600 0.72rem ui-sans-serif, system-ui, sans-serif;
    }
    .badge.ok {
      background: rgba(45, 190, 117, 0.18);
      color: var(--ok);
    }
    .changelog {
      margin: 0;
      padding: 10px 12px;
      background: #fff;
      border: 1px solid var(--line);
      border-radius: 10px;
      font: 0.82rem/1.55 ui-monospace, SFMono-Regular, monospace;
      color: var(--ink-soft);
      white-space: pre-wrap;
      max-height: 200px;
      overflow-y: auto;
    }
    .asset {
      display: flex;
      flex-direction: column;
      gap: 6px;
      font: 0.88rem ui-sans-serif, system-ui, sans-serif;
      color: var(--muted);
    }
    .asset strong {
      color: var(--ink);
    }
    .asset progress {
      width: 100%;
      height: 6px;
    }
    .row {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
    }
    .row .cta {
      flex: 1 1 auto;
    }
    .ok {
      color: var(--ok);
      font: 600 0.85rem ui-sans-serif, system-ui, sans-serif;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "updates-panel": UpdatesPanel;
  }
}

function formatBytes(n: number): string {
  if (n < 1024) return `${n} B`;
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} kB`;
  return `${(n / 1024 / 1024).toFixed(2)} MB`;
}

function trimChangelog(text: string): string {
  // GitHub auto-dopisuje "**Full Changelog**: https://github.com/...compare/..."
  // na końcu opisu release'a — czysto techniczny link, nie ma po co go
  // pokazywać użytkownikowi appki (i wprost mówi "github.com").
  const withoutFullChangelog = text.replace(/\n*\*\*Full Changelog\*\*:.*$/s, "").trim();
  const lines = withoutFullChangelog.split("\n");
  return lines.slice(0, 12).join("\n") + (lines.length > 12 ? "\n…" : "");
}
