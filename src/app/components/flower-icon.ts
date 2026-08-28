import { html, type TemplateResult } from "lit";
import dandelionMarkUrl from "../assets/dandelion-mark.png";

// Prawdziwy rysunek dmuchawca (nadesłany przez Karola), nie odwzorowanie SVG.
// Renderowany jako CSS mask, żeby dziedziczyć `color` z kontenera (currentColor)
// dokładnie tak samo jak poprzednia wersja rysowana ręcznie w SVG.
export function dandelionIcon(s = 64): TemplateResult {
  return html`
    <span
      style=${`display:inline-block;width:${s}px;height:${s}px;background-color:currentColor;
        -webkit-mask-image:url(${dandelionMarkUrl});mask-image:url(${dandelionMarkUrl});
        -webkit-mask-size:contain;mask-size:contain;
        -webkit-mask-repeat:no-repeat;mask-repeat:no-repeat;
        -webkit-mask-position:center;mask-position:center;`}
      aria-hidden="true"
    ></span>
  `;
}
