import { LitElement, css, html, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { deviceApi, type BookParagraph, type ChapterMark } from "../device/api";

type Filter = "all" | "chapters" | "suggested";

const FIRST_PAGE = 200;
const PAGE = 400;
const WORDS = 16;
const MAX_TITLE = 60;
const ROWS_STEP = 200;

// Short paragraphs that read like headings: "Rozdział 3", "CZĘŚĆ DRUGA",
// "XII", "7.", "Prolog"...
const HEADING_WORDS =
  /^(rozdzia[łl]|cz[eę][sś][cć]|ksi[eę]ga|tom|prolog|epilog|wst[eę]p|pos[łl]owie|przedmowa|zako[nń]czenie|chapter|part|book|prologue|epilogue|introduction|kapitel|teil|cap[ií]tulo|parte|chapitre|partie)(?=$|[\s.:,)\-–—])/i;
// (not \b: "ł" is not a word character in JS regexes, so "Rozdział 1"
// would never match)
const NUMBER_ONLY = /^([ivxlcdm]+|\d+)[.)]?$/i;

function looksLikeHeading(p: BookParagraph): boolean {
  if (p.n === 0 || p.n > 12) return false;
  const text = p.t.trim();
  if (HEADING_WORDS.test(text) || NUMBER_ONLY.test(text)) return true;
  const letters = text.replace(/[^\p{L}]/gu, "");
  return p.n <= 8 && letters.length >= 3 && letters === letters.toLocaleUpperCase("pl");
}

function defaultTitle(p: BookParagraph): string {
  const text = p.t.replace(/\s+/g, " ").trim();
  return text.length > MAX_TITLE ? `${text.slice(0, MAX_TITLE - 1).trimEnd()}…` : text;
}

/**
 * Where chapters start in a book on the reader. The reader sends its own
 * paragraphs (with the word each starts at), the user marks the ones that
 * open a chapter, the list goes back as word numbers — nothing depends on
 * the app splitting the text the same way the reader does.
 *
 * Events: `close`, `saved`.
 */
@customElement("chapter-editor")
export class ChapterEditor extends LitElement {
  @property() bookName = "";
  @property() bookTitle = "";

  @state() private paragraphs: BookParagraph[] = [];
  @state() private paragraphCount = 0;
  @state() private wordCount = 0;
  @state() private chapters = new Map<number, string>();
  @state() private custom = false;
  @state() private loading = true;
  @state() private loadingMore = false;
  @state() private filter: Filter = "all";
  @state() private query = "";
  @state() private rows = 120;
  @state() private busy = "";
  @state() private error = "";
  @state() private notice = "";
  private original = "";
  private cancelled = false;

  connectedCallback(): void {
    super.connectedCallback();
    this.cancelled = false;
    void this.load();
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    this.cancelled = true;
  }

  private async load(): Promise<void> {
    this.loading = true;
    this.error = "";
    this.paragraphs = [];
    try {
      const first = await deviceApi.getBookText(this.bookName, 0, FIRST_PAGE, WORDS);
      this.paragraphCount = first.paragraphCount;
      this.wordCount = first.wordCount;
      this.custom = first.custom;
      this.chapters = new Map(first.chapters.map((c) => [c.w, c.t]));
      this.original = this.serialize();
      this.paragraphs = first.paragraphs;
      this.loading = false;
      // The rest in the background, so search and suggestions see the
      // whole book.
      this.loadingMore = true;
      let from = first.paragraphs.length;
      while (!this.cancelled && from < this.paragraphCount) {
        const page = await deviceApi.getBookText(this.bookName, from, PAGE, WORDS);
        if (page.paragraphs.length === 0) break;
        this.paragraphs = [...this.paragraphs, ...page.paragraphs];
        from += page.paragraphs.length;
      }
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    } finally {
      this.loading = false;
      this.loadingMore = false;
    }
  }

  private serialize(): string {
    return JSON.stringify([...this.chapters.entries()].sort((a, b) => a[0] - b[0]));
  }

  private get dirty(): boolean {
    return this.serialize() !== this.original;
  }

  private get suggestions(): BookParagraph[] {
    return this.paragraphs.filter((p) => looksLikeHeading(p));
  }

  render() {
    const suggestions = this.suggestions;
    const newSuggestions = suggestions.filter((p) => !this.chapters.has(p.w));
    return html`
      <div class="sheet" role="dialog" aria-label="Rozdziały">
        <header>
          <div>
            <small>Rozdziały ${this.custom ? "· ustawione w aplikacji" : "· znalezione w tekście"}</small>
            <strong>${this.bookTitle || this.bookName}</strong>
          </div>
          <button class="icon" @click=${this.close} aria-label="Zamknij">✕</button>
        </header>

        ${this.loading
          ? html`<p class="lead">Czytnik przygotowuje tekst książki… Przy dużym EPUB-ie może to potrwać minutę.</p>`
          : this.renderEditor(suggestions, newSuggestions)}

        <footer>
          ${this.error ? html`<p class="error">${this.error}</p>` : nothing}
          ${this.notice ? html`<p class="ok">${this.notice}</p>` : nothing}
          <button class="cta ghost" ?disabled=${!!this.busy} @click=${this.resetToDetected}>
            Przywróć z tekstu
          </button>
          <button class="cta" ?disabled=${!!this.busy || !this.dirty || this.loading} @click=${this.save}>
            ${this.busy || `Zapisz (${this.chapters.size})`}
          </button>
        </footer>
      </div>
    `;
  }

  private renderEditor(suggestions: BookParagraph[], newSuggestions: BookParagraph[]) {
    const list = this.visibleParagraphs(suggestions);
    const loadedPercent = this.paragraphCount ? Math.round((this.paragraphs.length / this.paragraphCount) * 100) : 100;
    return html`
      <p class="lead">
        Stuknij akapit, od którego zaczyna się rozdział. Tytuł możesz zmienić. Na czytniku rozdziały pojawią się
        po wyjściu z ekranu Aplikacja.
      </p>

      ${this.chapters.size > 0
        ? html`<div class="chips">
            ${[...this.chapters.entries()]
              .sort((a, b) => a[0] - b[0])
              .map(
                ([w, t], i) => html`<button class="chip" @click=${() => this.jumpTo(w)}>
                  <span>${i + 1}</span>${t || "Bez tytułu"}
                </button>`,
              )}
          </div>`
        : html`<p class="hint">Ta książka nie ma jeszcze rozdziałów.</p>`}

      ${newSuggestions.length > 0
        ? html`<div class="suggest">
            <span>Wygląda na nagłówki: <strong>${newSuggestions.length}</strong></span>
            <button class="btn" @click=${() => this.addAll(newSuggestions)}>Dodaj wszystkie</button>
          </div>`
        : nothing}

      <div class="toolbar">
        <input
          type="search"
          placeholder="Szukaj w tekście, np. Rozdział"
          .value=${this.query}
          @input=${(e: Event) => {
            this.query = (e.target as HTMLInputElement).value;
            this.rows = 120;
          }}
        />
        <div class="filters">
          ${this.filterButton("all", "Cały tekst")}
          ${this.filterButton("chapters", `Rozdziały ${this.chapters.size}`)}
          ${this.filterButton("suggested", `Nagłówki ${suggestions.length}`)}
        </div>
      </div>

      ${this.loadingMore
        ? html`<div class="progress"><span style="width:${loadedPercent}%"></span></div>
            <p class="hint">Wczytuję resztę tekstu: ${loadedPercent}%</p>`
        : nothing}

      <ol class="paragraphs">
        ${list.items.map((p) => this.renderParagraph(p))}
      </ol>
      ${list.more
        ? html`<button class="btn ghost more" @click=${() => (this.rows += ROWS_STEP)}>Pokaż dalszy tekst</button>`
        : nothing}
      ${list.items.length === 0 && !this.loadingMore ? html`<p class="hint">Nic nie pasuje.</p>` : nothing}
    `;
  }

  private visibleParagraphs(suggestions: BookParagraph[]): { items: BookParagraph[]; more: boolean } {
    let items =
      this.filter === "chapters"
        ? this.paragraphs.filter((p) => this.chapters.has(p.w))
        : this.filter === "suggested"
          ? suggestions
          : this.paragraphs;
    const q = this.query.trim().toLocaleLowerCase("pl");
    if (q) items = items.filter((p) => p.t.toLocaleLowerCase("pl").includes(q));
    return { items: items.slice(0, this.rows), more: items.length > this.rows };
  }

  private filterButton(filter: Filter, label: string) {
    return html`<button
      class=${this.filter === filter ? "filter active" : "filter"}
      @click=${() => {
        this.filter = filter;
        this.rows = 120;
      }}
    >
      ${label}
    </button>`;
  }

  private renderParagraph(p: BookParagraph) {
    const title = this.chapters.get(p.w);
    const isChapter = title !== undefined;
    const position = this.wordCount ? Math.floor((p.w / this.wordCount) * 100) : 0;
    const heading = looksLikeHeading(p);
    return html`
      <li id=${`p${p.w}`} class=${isChapter ? "chapter" : heading ? "heading" : ""}>
        ${isChapter
          ? html`<div class="mark">
              <input
                type="text"
                maxlength=${MAX_TITLE}
                .value=${title}
                aria-label="Tytuł rozdziału"
                @input=${(e: Event) => this.rename(p.w, (e.target as HTMLInputElement).value)}
              />
              <button class="remove" @click=${() => this.toggle(p)} aria-label="Usuń rozdział">✕</button>
            </div>`
          : nothing}
        <button class="text" @click=${() => (isChapter ? this.jumpTo(p.w) : this.toggle(p))}>
          <span class="pos">${position}%</span>
          <span class="words">${p.t}${p.n > WORDS ? "…" : ""}</span>
          ${isChapter ? nothing : html`<span class="add">+ rozdział</span>`}
        </button>
      </li>
    `;
  }

  private toggle(p: BookParagraph): void {
    const next = new Map(this.chapters);
    if (next.has(p.w)) next.delete(p.w);
    else next.set(p.w, defaultTitle(p));
    this.chapters = next;
    this.notice = "";
  }

  private rename(w: number, title: string): void {
    const next = new Map(this.chapters);
    next.set(w, title.slice(0, MAX_TITLE));
    this.chapters = next;
  }

  private addAll(paragraphs: BookParagraph[]): void {
    const next = new Map(this.chapters);
    for (const p of paragraphs) if (!next.has(p.w)) next.set(p.w, defaultTitle(p));
    this.chapters = next;
    this.notice = "";
  }

  private async jumpTo(w: number): Promise<void> {
    this.filter = "all";
    this.query = "";
    const index = this.paragraphs.findIndex((p) => p.w === w);
    if (index >= this.rows) this.rows = index + 20;
    await this.updateComplete;
    this.renderRoot.querySelector(`#p${w}`)?.scrollIntoView({ block: "center", behavior: "smooth" });
  }

  private save = async () => {
    if (this.chapters.size === 0) {
      this.error = "Dodaj przynajmniej jeden rozdział albo użyj „Przywróć z tekstu”.";
      return;
    }
    this.error = "";
    this.busy = "Zapisuję…";
    try {
      const list: ChapterMark[] = [...this.chapters.entries()]
        .sort((a, b) => a[0] - b[0])
        .map(([w, t]) => ({ w, t: t.trim() || "Rozdział" }));
      await deviceApi.setBookChapters(this.bookName, list);
      this.original = this.serialize();
      this.custom = true;
      this.notice = "Zapisane na czytniku.";
      this.dispatchEvent(new CustomEvent("saved", { bubbles: true, composed: true }));
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    } finally {
      this.busy = "";
    }
  };

  private resetToDetected = async () => {
    if (this.custom && !confirm("Usunąć rozdziały ustawione w aplikacji i wrócić do tych z tekstu książki?")) return;
    this.error = "";
    this.busy = "Przywracam…";
    try {
      if (this.custom) await deviceApi.resetBookChapters(this.bookName);
      const first = await deviceApi.getBookText(this.bookName, 0, 1, 1);
      this.custom = first.custom;
      this.chapters = new Map(first.chapters.map((c) => [c.w, c.t]));
      this.original = this.serialize();
      this.notice = "Rozdziały jak w tekście książki.";
      this.dispatchEvent(new CustomEvent("saved", { bubbles: true, composed: true }));
    } catch (err) {
      this.error = err instanceof Error ? err.message : String(err);
    } finally {
      this.busy = "";
    }
  };

  private close = () => {
    if (this.dirty && !confirm("Zamknąć bez zapisywania zmian w rozdziałach?")) return;
    this.dispatchEvent(new CustomEvent("close", { bubbles: true, composed: true }));
  };

  static styles = css`
    :host {
      position: fixed;
      inset: 0;
      z-index: 50;
      display: flex;
      align-items: flex-end;
      justify-content: center;
      background: rgba(20, 18, 15, 0.45);
    }
    .sheet {
      width: min(640px, 100%);
      height: 100%;
      overflow-y: auto;
      background: var(--paper, #f8f4ec);
      padding: 0 16px;
      display: flex;
      flex-direction: column;
      gap: 10px;
      /* Paragraphs keep arriving at the bottom while the rest loads. */
      overflow-anchor: none;
    }
    header {
      position: sticky;
      top: 0;
      z-index: 2;
      padding: 14px 0 6px;
      background: var(--paper, #f8f4ec);
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
    }
    header div {
      display: flex;
      flex-direction: column;
      min-width: 0;
    }
    header small {
      font: 600 0.7rem var(--mn);
      text-transform: uppercase;
      letter-spacing: 0.04em;
      color: var(--muted);
    }
    header strong {
      font: 600 1.1rem var(--fr);
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .icon {
      width: 36px;
      height: 36px;
      flex: 0 0 auto;
      border: 0;
      border-radius: 50%;
      background: var(--paper-tint);
      color: var(--ink-soft);
      font-size: 1rem;
      cursor: pointer;
    }
    .lead,
    .hint,
    .ok,
    .error {
      margin: 0;
      font: 0.88rem/1.45 var(--ns);
      color: var(--ink-soft);
    }
    .hint {
      color: var(--muted);
      font-size: 0.8rem;
    }
    .ok {
      color: var(--ok);
    }
    footer .ok,
    footer .error {
      flex: 1 1 100%;
    }
    .error {
      color: var(--err);
    }
    .chips {
      display: flex;
      gap: 6px;
      overflow-x: auto;
      padding-bottom: 4px;
    }
    .chip {
      flex: 0 0 auto;
      max-width: 220px;
      display: flex;
      align-items: center;
      gap: 6px;
      padding: 6px 10px;
      border: 1px solid var(--line);
      border-radius: var(--radius-pill, 999px);
      background: var(--paper-tint);
      color: var(--ink);
      font: 0.8rem var(--ns);
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
      cursor: pointer;
    }
    .chip span {
      font: 700 0.7rem var(--mn);
      color: var(--accent);
    }
    .suggest {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
      padding: 10px 12px;
      border-radius: var(--radius, 13px);
      background: rgba(20, 136, 216, 0.08);
      font: 0.86rem var(--ns);
    }
    .toolbar {
      position: sticky;
      top: 58px;
      z-index: 1;
      display: flex;
      flex-direction: column;
      gap: 8px;
      padding: 8px 0;
      background: var(--paper, #f8f4ec);
    }
    .toolbar input {
      padding: 10px 12px;
      border: 1px solid var(--line);
      border-radius: var(--radius-sm, 9px);
      background: #fff;
      font: 0.92rem var(--ns);
    }
    .filters {
      display: flex;
      gap: 6px;
    }
    .filter {
      flex: 1;
      padding: 7px 6px;
      border: 1px solid var(--line);
      border-radius: var(--radius-sm, 9px);
      background: var(--paper-tint);
      color: var(--ink-soft);
      font: 600 0.72rem var(--mn);
      cursor: pointer;
    }
    .filter.active {
      background: var(--accent);
      border-color: var(--accent);
      color: #fff;
    }
    .progress {
      height: 4px;
      border-radius: 2px;
      background: var(--line);
      overflow: hidden;
    }
    .progress span {
      display: block;
      height: 100%;
      background: var(--accent);
      transition: width 0.3s ease;
    }
    .paragraphs {
      list-style: none;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      gap: 4px;
    }
    .paragraphs li {
      border: 1px solid transparent;
      border-radius: var(--radius-sm, 9px);
    }
    .paragraphs li.heading {
      border-color: var(--line);
    }
    .paragraphs li.chapter {
      border-color: var(--accent);
      background: rgba(20, 136, 216, 0.06);
    }
    .mark {
      display: flex;
      gap: 6px;
      padding: 8px 8px 0;
    }
    .mark input {
      flex: 1;
      min-width: 0;
      padding: 8px 10px;
      border: 1px solid var(--accent);
      border-radius: var(--radius-sm, 9px);
      background: #fff;
      font: 600 0.92rem var(--fr);
    }
    .remove {
      width: 36px;
      border: 0;
      border-radius: var(--radius-sm, 9px);
      background: rgba(228, 77, 101, 0.1);
      color: var(--err);
      cursor: pointer;
    }
    .text {
      width: 100%;
      display: grid;
      grid-template-columns: 38px 1fr auto;
      align-items: start;
      gap: 8px;
      padding: 8px;
      border: 0;
      background: transparent;
      text-align: left;
      color: var(--ink);
      cursor: pointer;
    }
    .pos {
      font: 0.68rem var(--mn);
      color: var(--muted);
      padding-top: 3px;
    }
    .words {
      font: 0.88rem/1.4 var(--ns);
      display: -webkit-box;
      -webkit-line-clamp: 2;
      -webkit-box-orient: vertical;
      overflow: hidden;
    }
    li.heading .words {
      font-weight: 600;
    }
    .add {
      font: 700 0.68rem var(--mn);
      color: var(--accent);
      white-space: nowrap;
      padding-top: 3px;
    }
    .sheet > * {
      flex-shrink: 0;
    }
    footer {
      position: sticky;
      bottom: 0;
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      padding: 10px 0 calc(12px + env(safe-area-inset-bottom));
      background: var(--paper, #f8f4ec);
      border-top: 1px solid var(--line);
      margin-top: auto;
    }
    .btn,
    .cta {
      padding: 10px 14px;
      border: 1px solid var(--accent);
      border-radius: var(--radius-sm, 9px);
      background: var(--accent);
      color: #fff;
      font: 700 0.8rem var(--mn);
      cursor: pointer;
    }
    .cta {
      flex: 1;
    }
    .btn.ghost,
    .cta.ghost {
      background: transparent;
      color: var(--accent);
    }
    .more {
      align-self: center;
      margin: 6px 0 10px;
    }
    .cta:disabled {
      opacity: 0.5;
      cursor: default;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "chapter-editor": ChapterEditor;
  }
}
