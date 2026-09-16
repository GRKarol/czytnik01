#pragma once

#include <stdint.h>

#include "generated/TranslationsData.h"

enum class UiLanguage : uint8_t {
  English = 0,
  Spanish,
  French,
  German,
  Romanian,
  Polish,
  Count,
};

enum class UiText : uint8_t {
  Resume,
  Chapters,
  Library,
  Settings,
  UsbTransfer,
  PowerOff,
  Read,
  SavePoints,
  Plugins,
  Back,
  Display,
  TypographyTune,
  WordPacing,
  Theme,
  Brightness,
  Language,
  ReadingMode,
  LongWords,
  Complexity,
  Punctuation,
  ResetPacing,
  Night,
  Dark,
  Light,
  On,
  Off,
  FontSize,
  Typeface,
  PhantomWords,
  RedHighlight,
  Tracking,
  Anchor,
  GuideWidth,
  GuideGap,
  Reset,
  Typography,
  TapToExit,
  TapToReset,
  TapChangeSample,
  TapExitSample,
  TapToggleSample,
  TapCycleSample,
  CurrentBook,
  Start,
  StartOfBook,
  RestartBook,
  AreYouSure,
  NoKeepPlace,
  YesRestart,
  ResetTypographyQuestion,
  NoKeepSettings,
  YesReset,
  NoSamples,
  Large,
  Medium,
  Small,
  Standard,
  RsvpMode,
  ScrollMode,
  TimeEstimate,
  TimeEstimateAccurate,
  TimeEstimateFast,
  ScrollLineSpacing,
  ScrollMargins,
  SwipeMoreSettings,
};

namespace Localization {

inline UiLanguage sanitizeLanguage(uint8_t value) {
  if (value >= static_cast<uint8_t>(UiLanguage::Count)) {
    return UiLanguage::English;
  }
  return static_cast<UiLanguage>(value);
}

inline UiLanguage nextLanguage(UiLanguage current) {
  uint8_t value = static_cast<uint8_t>(current);
  value = static_cast<uint8_t>((value + 1) % static_cast<uint8_t>(UiLanguage::Count));
  return static_cast<UiLanguage>(value);
}

inline const char *languageName(UiLanguage language) {
  switch (language) {
    case UiLanguage::Spanish:
      return "Espanol";
    case UiLanguage::French:
      return "Francais";
    case UiLanguage::German:
      return "Deutsch";
    case UiLanguage::Romanian:
      return "Romana";
    case UiLanguage::Polish:
      return "Polski";
    case UiLanguage::English:
    default:
      return "English";
  }
}

inline const char *text(UiLanguage language, UiText key) {
  return TranslationsData::uiTextLookup(static_cast<uint8_t>(key), static_cast<uint8_t>(language));
}

}  // namespace Localization
