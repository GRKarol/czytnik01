/**
 * Tooltip data registry — maps all settings to their i18n keys
 * for contextual tooltip display in the settings panel.
 */

export interface TooltipData {
  settingKey: string;
  descriptionKey: string;
  effectKey: string;
  defaultValueKey: string;
}

/**
 * Setting categories for grouping tooltips in the UI.
 */
export type TooltipCategory = "czytanie" | "typografia" | "wyswietlanie";

export const TOOLTIP_CATEGORIES: Record<TooltipCategory, string[]> = {
  czytanie: [
    "readingMode",
    "pauseBehaviour",
    "baseWpm",
    "longWordDelay",
    "complexWordDelay",
    "punctuationDelay",
  ],
  typografia: [
    "fontSize",
    "typeface",
    "phantomWords",
    "focusHighlight",
    "tracking",
    "anchor",
    "guideWidth",
    "guideGap",
  ],
  wyswietlanie: [
    "theme",
    "brightness",
    "readerHand",
    "footerLabel",
    "batteryLabel",
    "screensaver",
    "readingBattery",
    "readingChapter",
    "readingPercent",
    "focusColor",
    "saveBtn",
  ],
};

/**
 * Full tooltip registry — one entry per configurable setting.
 * Keys follow the pattern: tooltip.{settingKey}.desc / .effect / .default
 */
export const TOOLTIP_REGISTRY: TooltipData[] = [
  // ─── Czytanie (Reading) ────────────────────────────────────────────
  {
    settingKey: "readingMode",
    descriptionKey: "tooltip.readingMode.desc",
    effectKey: "tooltip.readingMode.effect",
    defaultValueKey: "tooltip.readingMode.default",
  },
  {
    settingKey: "pauseBehaviour",
    descriptionKey: "tooltip.pauseBehaviour.desc",
    effectKey: "tooltip.pauseBehaviour.effect",
    defaultValueKey: "tooltip.pauseBehaviour.default",
  },
  {
    settingKey: "baseWpm",
    descriptionKey: "tooltip.baseWpm.desc",
    effectKey: "tooltip.baseWpm.effect",
    defaultValueKey: "tooltip.baseWpm.default",
  },
  {
    settingKey: "longWordDelay",
    descriptionKey: "tooltip.longWordDelay.desc",
    effectKey: "tooltip.longWordDelay.effect",
    defaultValueKey: "tooltip.longWordDelay.default",
  },
  {
    settingKey: "complexWordDelay",
    descriptionKey: "tooltip.complexWordDelay.desc",
    effectKey: "tooltip.complexWordDelay.effect",
    defaultValueKey: "tooltip.complexWordDelay.default",
  },
  {
    settingKey: "punctuationDelay",
    descriptionKey: "tooltip.punctuationDelay.desc",
    effectKey: "tooltip.punctuationDelay.effect",
    defaultValueKey: "tooltip.punctuationDelay.default",
  },

  // ─── Typografia (Typography) ───────────────────────────────────────
  {
    settingKey: "fontSize",
    descriptionKey: "tooltip.fontSize.desc",
    effectKey: "tooltip.fontSize.effect",
    defaultValueKey: "tooltip.fontSize.default",
  },
  {
    settingKey: "typeface",
    descriptionKey: "tooltip.typeface.desc",
    effectKey: "tooltip.typeface.effect",
    defaultValueKey: "tooltip.typeface.default",
  },
  {
    settingKey: "phantomWords",
    descriptionKey: "tooltip.phantomWords.desc",
    effectKey: "tooltip.phantomWords.effect",
    defaultValueKey: "tooltip.phantomWords.default",
  },
  {
    settingKey: "focusHighlight",
    descriptionKey: "tooltip.focusHighlight.desc",
    effectKey: "tooltip.focusHighlight.effect",
    defaultValueKey: "tooltip.focusHighlight.default",
  },
  {
    settingKey: "tracking",
    descriptionKey: "tooltip.tracking.desc",
    effectKey: "tooltip.tracking.effect",
    defaultValueKey: "tooltip.tracking.default",
  },
  {
    settingKey: "anchor",
    descriptionKey: "tooltip.anchor.desc",
    effectKey: "tooltip.anchor.effect",
    defaultValueKey: "tooltip.anchor.default",
  },
  {
    settingKey: "guideWidth",
    descriptionKey: "tooltip.guideWidth.desc",
    effectKey: "tooltip.guideWidth.effect",
    defaultValueKey: "tooltip.guideWidth.default",
  },
  {
    settingKey: "guideGap",
    descriptionKey: "tooltip.guideGap.desc",
    effectKey: "tooltip.guideGap.effect",
    defaultValueKey: "tooltip.guideGap.default",
  },

  // ─── Wyświetlanie (Display) ────────────────────────────────────────
  {
    settingKey: "theme",
    descriptionKey: "tooltip.theme.desc",
    effectKey: "tooltip.theme.effect",
    defaultValueKey: "tooltip.theme.default",
  },
  {
    settingKey: "brightness",
    descriptionKey: "tooltip.brightness.desc",
    effectKey: "tooltip.brightness.effect",
    defaultValueKey: "tooltip.brightness.default",
  },
  {
    settingKey: "readerHand",
    descriptionKey: "tooltip.readerHand.desc",
    effectKey: "tooltip.readerHand.effect",
    defaultValueKey: "tooltip.readerHand.default",
  },
  {
    settingKey: "footerLabel",
    descriptionKey: "tooltip.footerLabel.desc",
    effectKey: "tooltip.footerLabel.effect",
    defaultValueKey: "tooltip.footerLabel.default",
  },
  {
    settingKey: "batteryLabel",
    descriptionKey: "tooltip.batteryLabel.desc",
    effectKey: "tooltip.batteryLabel.effect",
    defaultValueKey: "tooltip.batteryLabel.default",
  },
  {
    settingKey: "screensaver",
    descriptionKey: "tooltip.screensaver.desc",
    effectKey: "tooltip.screensaver.effect",
    defaultValueKey: "tooltip.screensaver.default",
  },
  {
    settingKey: "readingBattery",
    descriptionKey: "tooltip.readingBattery.desc",
    effectKey: "tooltip.readingBattery.effect",
    defaultValueKey: "tooltip.readingBattery.default",
  },
  {
    settingKey: "readingChapter",
    descriptionKey: "tooltip.readingChapter.desc",
    effectKey: "tooltip.readingChapter.effect",
    defaultValueKey: "tooltip.readingChapter.default",
  },
  {
    settingKey: "readingPercent",
    descriptionKey: "tooltip.readingPercent.desc",
    effectKey: "tooltip.readingPercent.effect",
    defaultValueKey: "tooltip.readingPercent.default",
  },
  {
    settingKey: "focusColor",
    descriptionKey: "tooltip.focusColor.desc",
    effectKey: "tooltip.focusColor.effect",
    defaultValueKey: "tooltip.focusColor.default",
  },
  {
    settingKey: "saveBtn",
    descriptionKey: "tooltip.saveBtn.desc",
    effectKey: "tooltip.saveBtn.effect",
    defaultValueKey: "tooltip.saveBtn.default",
  },
];

/**
 * Look up tooltip data for a given setting key.
 * Returns undefined if no tooltip is registered for the key.
 */
export function getTooltipData(settingKey: string): TooltipData | undefined {
  return TOOLTIP_REGISTRY.find((entry) => entry.settingKey === settingKey);
}

/**
 * Get all tooltip entries for a specific category.
 */
export function getTooltipsByCategory(category: TooltipCategory): TooltipData[] {
  const keys = TOOLTIP_CATEGORIES[category];
  if (!keys) return [];
  return keys
    .map((key) => getTooltipData(key))
    .filter((entry): entry is TooltipData => entry !== undefined);
}

/**
 * Get all registered setting keys.
 */
export function getAllSettingKeys(): string[] {
  return TOOLTIP_REGISTRY.map((entry) => entry.settingKey);
}
