import { LitElement, css, html, svg } from "lit";
import { customElement, property } from "lit/decorators.js";

/**
 * Motyw dmuchawca w rogu tła — jeden cienki, monochromatyczny rysunek
 * kreskowy, tak jak na flower.theworkpc.com (nie kolorowe kwiatki
 * porozrzucane po ekranie). Czysto wizualne, pointer-events: none.
 */
@customElement("flower-decor")
export class FlowerDecor extends LitElement {
  @property({ type: String }) density: "low" | "medium" | "high" = "medium";

  private dandelion(cx: number, cy: number, r: number, seeds: number) {
    const stems = Array.from({ length: seeds }, (_, i) => (360 / seeds) * i + (cx + i * 7));
    return svg`
      <g transform="translate(${cx} ${cy})">
        ${stems.map((a) => {
          const len = r * (0.62 + ((a * 13) % 30) / 100);
          return svg`
            <g transform="rotate(${a})">
              <line x1="0" y1="0" x2="0" y2="${-len}" stroke="currentColor" stroke-width="0.35"/>
              <circle cx="0" cy="${-len}" r="0.9" fill="currentColor"/>
            </g>
          `;
        })}
        <circle r="1.6" fill="currentColor"/>
      </g>
    `;
  }

  render() {
    const corners: Array<{ x: number; y: number; r: number; seeds: number }> =
      this.density === "low"
        ? [{ x: 6, y: 96, r: 26, seeds: 18 }]
        : this.density === "high"
          ? [
              { x: 6, y: 96, r: 30, seeds: 22 },
              { x: 96, y: 6, r: 16, seeds: 14 },
              { x: 92, y: 70, r: 12, seeds: 10 },
            ]
          : [
              { x: 6, y: 98, r: 28, seeds: 20 },
              { x: 97, y: 4, r: 14, seeds: 12 },
            ];

    return html`
      <svg viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
        ${corners.map((c) => this.dandelion(c.x, c.y, c.r, c.seeds))}
      </svg>
    `;
  }

  static styles = css`
    :host {
      position: absolute;
      inset: 0;
      pointer-events: none;
      overflow: hidden;
      z-index: 0;
      color: var(--ink-soft, #6b665d);
      opacity: 0.16;
    }
    svg {
      width: 100%;
      height: 100%;
      display: block;
    }
  `;
}

declare global {
  interface HTMLElementTagNameMap {
    "flower-decor": FlowerDecor;
  }
}
