import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { deviceApi, onDeviceApiChange, type Book } from "../device/api";

@customElement("library-panel")
export class LibraryPanel extends LitElement {
  @state() private books: Book[] = [];
  @state() private loading = true;
  @state() private error = "";
  @state() private filter: "all" | "book" | "article" = "all";
  private unsubApi: (() => void) | null = null;

  connectedCallback(): void {
    super.connectedCallback();
    void this.refresh();
    this.unsubApi = onDeviceApiChange(() => void this.refresh());
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this.unsubApi?.();
  }

  render() {
    if (this.loading) return html`<p class="muted">Wczytuję bibliotekę…</p>`;
    if (this.error) return html`<p class="error">${this.error}</p>`;

    const list = this.filtered();
    return html`
      <div class="actions">
        <input
          id="upload"
          type="file"
          accept=".rsvp,.txt,.epub"
          hidden
          @change=${this.onUpload}
        />
        <label for="upload" class="btn">Wyślij plik na urządzenie</label>
        <button class="btn ghost" @click=${this.refresh}>Odśwież</button>
      </div>

      <div class="tabs">
        ${this.tabButton("all", "Wszystko", this.books.length)}
        ${this.tabButton("book", "Książki", this.books.filter((b) => b.category !== "article").length)}
        ${this.tabButton("article", "Artykuły", this.books.filter((b) => b.category === "article").length)}
      </div>

      ${list.length === 0
        ? html`<p class="muted">
            Pusto. Wyślij coś z telefonu albo przekonwertuj plik w zakładce <strong>Konwerter</strong>.
          </p>`
        : html`<ul class="list">${list.map((b) => this.row(b))}</ul>`}

      <p class="hint muted">
        Lista jest na razie symulowana w pamięci telefonu — kiedy firmware
        zacznie odpowiadać przez WiFi, ta sama logika pójdzie na realne API.
      </p>
    `;
  }

  private tabButton(key: typeof this.filter, label: string, count: number) {
    return html`
      <button
        class=${this.filter === key ? "tab active" : "tab"}
        @click=${() => (this.filter = key)}
      >
        ${label} <span>${count}</span>
      </button>
    `;
  }

  private row(b: Book) {
    const title = b.title || b.name.replace(/^.*\//, "");
    return html`
      <li>
        <div class="meta">
          <strong>${title}</strong>
          <span>
            ${b.author ? `${b.author} · ` : ""}${formatBytes(b.bytes)}
            ${b.progressPercent != null ? html` · ${b.progressPercent}% przeczytane` : ""}
          </span>
        </div>
        <button class="del" @click=${() => this.onDelete(b)} aria-label="Usuń">✕</button>
      </li>
    `;
  }

  private filtered(): Book[] {
    if (this.filter === "all") return this.books;
    return this.books.filter((b) =>
      this.filter === "book" ? b.category !== "article" : b.category === "article",
    );
  }

  private refresh = async () => {
    this.loading = true;
    this.error = "";
    try {
      this.books = await deviceApi.listBooks();
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    } finally {
      this.loading = false;
    }
  };

  private onUpload = async (e: Event) => {
    const input = e.target as HTMLInputElement;
    const file = input.files?.[0];
    input.value = "";
    if (!file) return;
    try {
      await deviceApi.uploadBook(file, file.name);
      await this.refresh();
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    }
  };

  private onDelete = async (b: Book) => {
    if (!confirm(`Usunąć „${b.title || b.name}"?`)) return;
    try {
      await deviceApi.deleteBook(b.name);
      await this.refresh();
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    }
  };

  static styles = css`
    :host {
      display: block;
      display: flex;
      flex-direction: column;
      gap: 12px;
    }
    .muted {
      color: var(--muted);
      font: 0.92rem/1.45 ui-sans-serif, system-ui, sans-serif;
      margin: 0;
    }
    .error {
      color: var(--err);
      font: 0.92rem ui-sans-serif, system-ui, sans-serif;
      margin: 0;
    }
    .actions {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
    }
    .btn {
      flex: 1 1 auto;
      padding: 11px 16px;
      text-align: center;
      border: 0;
      border-radius: 999px;
      color: #fff;
      background: var(--accent);
      font: 700 0.9rem ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
    }
    .btn.ghost {
      flex: 0 0 auto;
      background: transparent;
      color: var(--accent);
      border: 1.5px solid var(--accent);
    }
    .tabs {
      display: flex;
      gap: 6px;
    }
    .tab {
      flex: 1 1 auto;
      padding: 8px 10px;
      border: 1px solid var(--line);
      border-radius: 999px;
      background: var(--paper-tint);
      color: var(--ink-soft);
      font: 600 0.78rem ui-sans-serif, system-ui, sans-serif;
      cursor: pointer;
    }
    .tab.active {
      background: var(--accent);
      border-color: var(--accent);
      color: #fff;
    }
    .tab span {
      opacity: 0.7;
      font-weight: 500;
    }
    .list {
      list-style: none;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .list li {
      display: flex;
      align-items: center;
      gap: 10px;
      padding: 10px 12px;
      border: 1px solid var(--line);
      border-radius: 12px;
      background: var(--paper-tint);
    }
    .meta {
      flex: 1 1 auto;
      display: flex;
      flex-direction: column;
      gap: 1px;
      min-width: 0;
    }
    .meta strong {
      font-family: "Iowan Old Style", Georgia, ui-serif, serif;
      font-size: 0.98rem;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .meta span {
      font: 0.78rem ui-sans-serif, system-ui, sans-serif;
      color: var(--muted);
    }
    .del {
      width: 32px;
      height: 32px;
      border: 0;
      border-radius: 50%;
      background: rgba(228, 77, 101, 0.1);
      color: var(--err);
      font-size: 0.85rem;
      cursor: pointer;
      flex: 0 0 auto;
    }
    .hint {
      font-size: 0.78rem;
      font-style: italic;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "library-panel": LibraryPanel;
  }
}

function formatBytes(n: number): string {
  if (n < 1024) return `${n} B`;
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} kB`;
  return `${(n / 1024 / 1024).toFixed(2)} MB`;
}
