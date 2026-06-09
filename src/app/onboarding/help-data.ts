export interface HelpItem {
  nameKey: string; // i18n key for item name
  descKey: string; // i18n key for item description
  valuesKey: string; // i18n key for available values/range
}

export interface HelpCategory {
  titleKey: string; // i18n key for category title
  items: HelpItem[];
}

export const HELP_CATEGORIES: HelpCategory[] = [
  {
    titleKey: "help.category.display",
    items: [
      {
        nameKey: "help.display.theme.name",
        descKey: "help.display.theme.desc",
        valuesKey: "help.display.theme.values",
      },
      {
        nameKey: "help.display.brightness.name",
        descKey: "help.display.brightness.desc",
        valuesKey: "help.display.brightness.values",
      },
      {
        nameKey: "help.display.readerHand.name",
        descKey: "help.display.readerHand.desc",
        valuesKey: "help.display.readerHand.values",
      },
      {
        nameKey: "help.display.footerLabel.name",
        descKey: "help.display.footerLabel.desc",
        valuesKey: "help.display.footerLabel.values",
      },
      {
        nameKey: "help.display.batteryLabel.name",
        descKey: "help.display.batteryLabel.desc",
        valuesKey: "help.display.batteryLabel.values",
      },
      {
        nameKey: "help.display.screensaver.name",
        descKey: "help.display.screensaver.desc",
        valuesKey: "help.display.screensaver.values",
      },
      {
        nameKey: "help.display.readingBattery.name",
        descKey: "help.display.readingBattery.desc",
        valuesKey: "help.display.readingBattery.values",
      },
      {
        nameKey: "help.display.readingChapter.name",
        descKey: "help.display.readingChapter.desc",
        valuesKey: "help.display.readingChapter.values",
      },
      {
        nameKey: "help.display.readingPercent.name",
        descKey: "help.display.readingPercent.desc",
        valuesKey: "help.display.readingPercent.values",
      },
      {
        nameKey: "help.display.focusColor.name",
        descKey: "help.display.focusColor.desc",
        valuesKey: "help.display.focusColor.values",
      },
      {
        nameKey: "help.display.saveBtn.name",
        descKey: "help.display.saveBtn.desc",
        valuesKey: "help.display.saveBtn.values",
      },
    ],
  },
  {
    titleKey: "help.category.reading",
    items: [
      {
        nameKey: "help.reading.readingMode.name",
        descKey: "help.reading.readingMode.desc",
        valuesKey: "help.reading.readingMode.values",
      },
      {
        nameKey: "help.reading.pauseBehaviour.name",
        descKey: "help.reading.pauseBehaviour.desc",
        valuesKey: "help.reading.pauseBehaviour.values",
      },
      {
        nameKey: "help.reading.baseWpm.name",
        descKey: "help.reading.baseWpm.desc",
        valuesKey: "help.reading.baseWpm.values",
      },
      {
        nameKey: "help.reading.longWordDelay.name",
        descKey: "help.reading.longWordDelay.desc",
        valuesKey: "help.reading.longWordDelay.values",
      },
      {
        nameKey: "help.reading.complexWordDelay.name",
        descKey: "help.reading.complexWordDelay.desc",
        valuesKey: "help.reading.complexWordDelay.values",
      },
      {
        nameKey: "help.reading.punctuationDelay.name",
        descKey: "help.reading.punctuationDelay.desc",
        valuesKey: "help.reading.punctuationDelay.values",
      },
      {
        nameKey: "help.reading.fontSize.name",
        descKey: "help.reading.fontSize.desc",
        valuesKey: "help.reading.fontSize.values",
      },
      {
        nameKey: "help.reading.typeface.name",
        descKey: "help.reading.typeface.desc",
        valuesKey: "help.reading.typeface.values",
      },
      {
        nameKey: "help.reading.phantomWords.name",
        descKey: "help.reading.phantomWords.desc",
        valuesKey: "help.reading.phantomWords.values",
      },
      {
        nameKey: "help.reading.focusHighlight.name",
        descKey: "help.reading.focusHighlight.desc",
        valuesKey: "help.reading.focusHighlight.values",
      },
      {
        nameKey: "help.reading.tracking.name",
        descKey: "help.reading.tracking.desc",
        valuesKey: "help.reading.tracking.values",
      },
      {
        nameKey: "help.reading.anchor.name",
        descKey: "help.reading.anchor.desc",
        valuesKey: "help.reading.anchor.values",
      },
      {
        nameKey: "help.reading.guideWidth.name",
        descKey: "help.reading.guideWidth.desc",
        valuesKey: "help.reading.guideWidth.values",
      },
      {
        nameKey: "help.reading.guideGap.name",
        descKey: "help.reading.guideGap.desc",
        valuesKey: "help.reading.guideGap.values",
      },
    ],
  },
  {
    titleKey: "help.category.hud",
    items: [
      {
        nameKey: "help.hud.readingBattery.name",
        descKey: "help.hud.readingBattery.desc",
        valuesKey: "help.hud.readingBattery.values",
      },
      {
        nameKey: "help.hud.readingChapter.name",
        descKey: "help.hud.readingChapter.desc",
        valuesKey: "help.hud.readingChapter.values",
      },
      {
        nameKey: "help.hud.readingPercent.name",
        descKey: "help.hud.readingPercent.desc",
        valuesKey: "help.hud.readingPercent.values",
      },
    ],
  },
  {
    titleKey: "help.category.language",
    items: [
      {
        nameKey: "help.language.uiLang.name",
        descKey: "help.language.uiLang.desc",
        valuesKey: "help.language.uiLang.values",
      },
    ],
  },
  {
    titleKey: "help.category.connection",
    items: [
      {
        nameKey: "help.connection.bluetooth.name",
        descKey: "help.connection.bluetooth.desc",
        valuesKey: "help.connection.bluetooth.values",
      },
      {
        nameKey: "help.connection.wifi.name",
        descKey: "help.connection.wifi.desc",
        valuesKey: "help.connection.wifi.values",
      },
    ],
  },
];
