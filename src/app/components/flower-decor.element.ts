import { LitElement, css, html, svg } from "lit";
import { customElement, property } from "lit/decorators.js";

/**
 * Dekoracyjne kwiatki rozsiane po tle — proste, geometryczne, w kolorach
 * Flower (niebieski / żółty / róż). Czysto wizualne, pointer-events: none.
 */
@customElement("flower-decor")
export class FlowerDecor extends LitElement {
  @property({ type: String }) density: "low" | "medium" | "high" = "medium";

  render() {
    const blossoms: Array<{ x: number; y: number; r: number; rot: number; tone: string; o: number }> =
      [
        { x: 8, y: 12, r: 18, rot: 14, tone: "var(--bloom-yellow)", o: 0.55 },
        { x: 86, y: 8, r: 14, rot: -22, tone: "var(--bloom-pink)", o: 0.55 },
        { x: 18, y: 78, r: 22, rot: 38, tone: "var(--bloom-pink)", o: 0.45 },
        { x: 78, y: 64, r: 16, rot: -8, tone: "var(--bloom-yellow)", o: 0.5 },
        { x: 50, y: 92, r: 12, rot: 0, tone: "var(--bloom-blue)", o: 0.4 },
      ];

    const picks =
      this.density === "low"
        ? blossoms.slice(0, 2)
        : this.density === "high"
          ? blossoms
          : blossoms.slice(0, 4);

    return html`
      <svg viewBox="0 0 100 100" preserveAspectRatio="none" aria-hidden="true">
        ${picks.map(
          (b) => svg`
          <g transform="translate(${b.x} ${b.y}) rotate(${b.rot})" opacity=${b.o}>
            ${[0, 72, 144, 216, 288].map(
              (a) => svg`
              <ellipse cx="0" cy="${-b.r * 0.36}" rx="${b.r * 0.22}" ry="${b.r * 0.36}"
                       fill=${b.tone} transform="rotate(${a})"/>
            `,
            )}
            <circle r="${b.r * 0.16}" fill="var(--bloom-center)"/>
          </g>
        `,
        )}
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
      --bloom-yellow: #ffd66e;
      --bloom-pink: #ffb6c8;
      --bloom-blue: #7cc4ff;
      --bloom-center: #ffd66e;
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
