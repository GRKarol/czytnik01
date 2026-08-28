import { svg, type SVGTemplateResult } from "lit";

// Dmuchawiec-logo, wierne odwzorowanie znaku z flower.theworkpc.com:
// wiązka cienkich, zakrzywionych "piór" wachlarzem nad łodygą. Współdzielone
// przez header/hero apki, dialog instalacji PWA i onboarding — jedno źródło
// prawdy, żeby nie rozjeżdżały się przy kolejnych zmianach brandingu.
function frondPath(angleDeg: number, length: number, bend: number, width: number): string {
  const rad = (angleDeg * Math.PI) / 180;
  const dx = Math.cos(rad);
  const dy = Math.sin(rad);
  const px = -dy;
  const py = dx;
  const tipX = dx * length;
  const tipY = dy * length;
  const midX = dx * length * 0.55;
  const midY = dy * length * 0.55;
  const halfW = width / 2;
  const outX = midX + px * (bend + halfW);
  const outY = midY + py * (bend + halfW);
  const inX = midX + px * (bend - halfW);
  const inY = midY + py * (bend - halfW);
  return `M0 0 Q ${outX} ${outY} ${tipX} ${tipY} Q ${inX} ${inY} 0 0 Z`;
}

export function dandelionIcon(s = 64): SVGTemplateResult {
  const seedCount = 13;
  const lenVariance = [0, 5, -3, 7, -2, 4, 0, -4, 3, -6, 4, -2, 5];
  const fronds = Array.from({ length: seedCount }, (_, i) => {
    const angle = -172 + (164 / (seedCount - 1)) * i;
    const length = 34 + (lenVariance[i] ?? 0);
    return frondPath(angle, length, 3.5, 3);
  });
  const stem = frondPath(102, 46, -3, 5.5);
  return svg`
    <svg width=${s} height=${s} viewBox="0 0 100 100" aria-hidden="true">
      <g transform="translate(48 40)" fill="currentColor">
        <path d=${stem}/>
        ${fronds.map((d) => svg`<path d=${d}/>`)}
        <circle r="3.2"/>
      </g>
    </svg>
  `;
}
