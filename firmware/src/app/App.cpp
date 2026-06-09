#include "app/App.h"

#include <esp_sleep.h>
#include <esp_log.h>
#include <WiFi.h>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <iterator>
#include <utility>
#include <vector>

#include "app/Translations.h"
#include "board/BoardConfig.h"

#ifndef RSVP_USB_TRANSFER_ENABLED
#define RSVP_USB_TRANSFER_ENABLED 0
#endif

#ifndef RSVP_USB_TRANSFER_AUTO_START
#define RSVP_USB_TRANSFER_AUTO_START 0
#endif

static const char *kAppTag = "app";
constexpr uint32_t kOtaCheckTaskStackBytes = 10240;
constexpr uint32_t kBootSplashMs = 750;
constexpr uint32_t kWpmFeedbackMs = 900;
constexpr uint32_t kPowerOffHoldMs = 1600;
constexpr uint32_t kPowerOffReleaseWaitMs = 4000;
constexpr uint32_t kBatterySampleIntervalMs = 180000;
constexpr uint32_t kTouchPlayHoldMs = 420;
constexpr uint32_t kPreviewBrowseHoldMs = 240;
constexpr uint32_t kReaderDoubleTapWindowMs = 520;
constexpr uint32_t kThemeToggleHoldMs = 900;
constexpr uint32_t kScrollAnimationFrameMs = 16;
constexpr uint16_t kSwipeThresholdPx = 40;
constexpr uint16_t kAxisBiasPx = 12;
constexpr uint16_t kTapSlopPx = 26;
constexpr uint16_t kReaderDoubleTapSlopPx = 92;
constexpr uint16_t kPreviousSentenceTapWidthPx = 96;
constexpr uint16_t kPreviousSentenceTapHeightPx = 60;
constexpr uint16_t kFooterMetricTapWidthPx = 220;
constexpr uint16_t kFooterMetricTapHeightPx = 32;
constexpr uint16_t kBatteryBadgeTapWidthPx = 160;
constexpr uint16_t kBatteryBadgeTapHeightPx = 40;
constexpr uint16_t kScrubStepPx = 22;
constexpr uint16_t kBrowseNeutralZonePx = 14;
constexpr uint16_t kFocusTimerCancelHoldMaxDriftPx = 20;
constexpr int kMaxScrubStepsPerGesture = 96;
constexpr uint32_t kBrowseMinWordsPerSecondPermille = 4000;
constexpr uint32_t kBrowseMaxWordsPerSecondPermille = 72000;
constexpr uint32_t kFocusTimerCancelHoldMs = 850;
constexpr uint32_t kHelpLongPressMs = 600;
constexpr uint16_t kHelpLongPressMaxDriftPx = 20;
constexpr size_t kContextPreviewWindowWords = 288;
constexpr size_t kContextPreviewAnchorLeadWords = 112;
constexpr size_t kContextPreviewMaxParagraphSnapWords = 48;
constexpr uint32_t kProgressSaveIntervalMs = 10000;
constexpr uint32_t kUsbTransferExitHoldMs = 1200;
constexpr size_t kTimeEstimateBlockWords = 256;
constexpr size_t kTimeEstimateBlocksPerUpdate = 1;
constexpr uint32_t kTimeEstimateProgressLogMs = 5000;
constexpr uint32_t kNominalBatteryRuntimeMinutes = 330;
constexpr uint8_t kBatteryDisplayHysteresisPercent = 2;
constexpr uint8_t kBatteryRuntimeMinDropPercent = 3;
constexpr uint32_t kBatteryRuntimeMinElapsedMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kBatteryPlayingSampleIntervalMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kBatteryLowSampleIntervalMs = 60UL * 1000UL;
constexpr uint32_t kBatteryLowWarningRepeatMs = 5UL * 60UL * 1000UL;
constexpr uint32_t kBatteryWarningVisibleMs = 2500;
constexpr uint32_t kBatteryShutdownNoticeMs = 1500;
constexpr float kBatteryLowWarningVoltage = 3.50f;
constexpr float kBatteryCriticalVoltage = 3.30f;
constexpr uint8_t kBatteryLowWarningPercent = 5;
constexpr uint8_t kBatteryCriticalPercent = 1;
constexpr uint8_t kBatteryCriticalConsecutiveSamples = 2;
constexpr uint32_t kStandbyWakeGraceMs = 900;
constexpr uint32_t kStandbyFrameMs = 160;
constexpr uint16_t kStandbyLifeCellPixels = 2;
constexpr uint16_t kStandbyLifeColumns = BoardConfig::DISPLAY_WIDTH / kStandbyLifeCellPixels;
constexpr uint16_t kStandbyLifeRows = BoardConfig::DISPLAY_HEIGHT / kStandbyLifeCellPixels;
constexpr uint32_t kChapterTransitionMs = 1400;
constexpr uint8_t kBrightnessLevels[] = {40, 55, 70, 85, 100};
constexpr uint8_t kNightBrightnessLevels[] = {35, 40, 45, 50, 55};
constexpr size_t kBrightnessLevelCount = sizeof(kBrightnessLevels) / sizeof(kBrightnessLevels[0]);

namespace {

enum MenuItem : size_t {
  MenuRead,
  MenuLibrary,
  MenuSavePoints,
  MenuSettings,
  MenuPlugins,
  MenuPowerOff,
  MenuItemCount,
};

enum SettingsItem : size_t {
  SettingsBack,
  SettingsDisplay,
  SettingsTypography,
  SettingsWordPacing,
  SettingsHandedness,
  SettingsBrightness,
  SettingsTheme,
  SettingsPhantomWords,
  SettingsFontSize,
  SettingsLongWords,
  SettingsComplexWords,
  SettingsPunctuation,
  SettingsReset,
  SettingsItemCount,
};

enum TypographyTuningItem : size_t {
  TypographyTuningBack,
  TypographyTuningFontSize,
  TypographyTuningTypeface,
  TypographyTuningPhantomWords,
  TypographyTuningFocusHighlight,
  TypographyTuningTracking,
  TypographyTuningAnchor,
  TypographyTuningGuideWidth,
  TypographyTuningGuideGap,
  TypographyTuningReset,
  TypographyTuningItemCount,
};

enum RestartConfirmItem : size_t {
  RestartConfirmNo,
  RestartConfirmYes,
  RestartConfirmItemCount,
};

enum SdCardRepairConfirmItem : size_t {
  SdCardRepairConfirmNo,
  SdCardRepairConfirmYes,
  SdCardRepairConfirmItemCount,
};

enum UpdateConfirmItem : size_t {
  UpdateConfirmSkip,
  UpdateConfirmUpdate,
  UpdateConfirmItemCount,
};

constexpr size_t kRestartConfirmHeaderRows = 1;
constexpr size_t kSdCardRepairConfirmHeaderRows = 1;
constexpr size_t kUpdateConfirmHeaderRows = 2;
constexpr size_t kSettingsBackIndex = 0;
// SettingsHome — nowy układ, ułożony pod codzienne użycie.
// Indeksy 1–5 są zawsze widoczne; 6 (About) też; 7–8 tylko gdy `devModeEnabled()`.
constexpr size_t kSettingsHomeReadingIndex = 1;       // = stare Pacing
constexpr size_t kSettingsHomeDisplayIndex = 2;
constexpr size_t kSettingsHomeTypographyIndex = 3;    // always visible (moved from dev-only)
constexpr size_t kSettingsHomeConnectivityIndex = 4;
constexpr size_t kSettingsHomePresetsIndex = 5;       // Presets
constexpr size_t kSettingsHomeAboutIndex = 6;
constexpr size_t kSettingsHomeWifiIndex = 7;          // dev only
constexpr size_t kSettingsHomeUpdateIndex = 8;        // dev only

// Zachowujemy starą nazwę dla kompatybilności z pozostałymi miejscami,
// które do niej odwołują się dla cofania z głębszych ekranów.
constexpr size_t kSettingsHomePacingIndex = kSettingsHomeReadingIndex;

// SettingsConnectivity items (new layout per menu reorganization)
constexpr size_t kSettingsConnWifiIndex = 1;        // Wi-Fi (opens sub-screen)
#if FLOWER_BLE_ENABLED
constexpr size_t kSettingsConnBluetoothIndex = 2;   // Bluetooth toggle
constexpr size_t kSettingsConnSyncToggleIndex = 3;  // Synchronizacja z telefonem
constexpr size_t kSettingsConnRssIndex = 4;         // Kanaly RSS
constexpr size_t kSettingsConnUsbIndex = 5;         // Kopiuj przez USB
#else
constexpr size_t kSettingsConnBluetoothIndex = 99;  // not shown
constexpr size_t kSettingsConnSyncToggleIndex = 2;  // Synchronizacja z telefonem
constexpr size_t kSettingsConnRssIndex = 3;         // Kanaly RSS
constexpr size_t kSettingsConnUsbIndex = 4;         // Kopiuj przez USB
#endif

// SettingsAbout items
constexpr size_t kSettingsAboutVersionIndex = 1;   // tap-target dla dev mode
constexpr size_t kSettingsAboutBrandIndex = 2;
constexpr size_t kSettingsAboutSdCardIndex = 3;    // SD card check (moved from main menu)
constexpr size_t kSettingsAboutTutorialIndex = 4;  // restart tutorial
constexpr size_t kSettingsAboutDevModeIndex = 5;   // pokazywane gdy dev mode

constexpr const char *kPrefSetupDone = "setup_done";
constexpr const char *kPrefTutorialDone = "tut_done";
constexpr size_t kSettingsDisplayThemeIndex = 1;
constexpr size_t kSettingsDisplayBrightnessIndex = 2;
constexpr size_t kSettingsDisplayHandednessIndex = 3;
constexpr size_t kSettingsDisplayFooterIndex = 4;
constexpr size_t kSettingsDisplayBatteryIndex = 5;
constexpr size_t kSettingsDisplayScreensaverIndex = 6;
constexpr size_t kSettingsDisplayReaderBatteryIndex = 7;
constexpr size_t kSettingsDisplayReaderChapterIndex = 8;
constexpr size_t kSettingsDisplayReaderProgressIndex = 9;
constexpr size_t kSettingsDisplayLanguageIndex = 10;
constexpr size_t kSettingsDisplayFocusColorIndex = 11;
constexpr size_t kSettingsDisplaySavePointBtnIndex = 12;
constexpr size_t kSettingsDisplayHelpHintsIndex = 13;
constexpr size_t kSettingsPacingReadingModeIndex = 1;
constexpr size_t kSettingsPacingPauseModeIndex = 2;
constexpr size_t kSettingsPacingWpmIndex = 3;
constexpr size_t kSettingsPacingLongWordsIndex = 4;
constexpr size_t kSettingsPacingComplexityIndex = 5;
constexpr size_t kSettingsPacingPunctuationIndex = 6;
constexpr size_t kSettingsPacingResetIndex = 7;
// Scroll-specific indices (when readerMode_ == Scroll, these replace RSVP items)
constexpr size_t kSettingsPacingScrollFontSizeIndex = 2;
constexpr size_t kSettingsPacingScrollLineSpacingIndex = 3;
constexpr size_t kSettingsPacingScrollMarginIndex = 4;
constexpr size_t kSettingsPacingScrollPreviewIndex = 5;
constexpr size_t kWifiSettingsNetworkIndex = 1;
constexpr size_t kWifiSettingsChooseIndex = 2;
constexpr size_t kWifiSettingsForgetIndex = 3;
// Dev-only items start after ForgetNetwork
constexpr size_t kWifiSettingsAutoUpdateIndex = 4;
constexpr size_t kWifiSettingsOtaOwnerIndex = 5;

// ScreensaverSettings submenu indices
constexpr size_t kScreensaverSettingsBackIndex = 0;
constexpr size_t kScreensaverSettingsStyleIndex = 1;
constexpr size_t kScreensaverSettingsTimeoutIndex = 2;
constexpr size_t kScreensaverSettingsAutoOffIndex = 3;
constexpr size_t kScreensaverSettingsSleepGuardIndex = 4;
constexpr size_t kScreensaverSettingsPreviewIndex = 5;

// Screensaver timeout values in minutes: 1, 2, 3, 5, 10, 15, 20, 30
constexpr uint8_t kScreensaverTimeoutCount = 8;
constexpr uint16_t kScreensaverTimeoutMinutes[] = {1, 2, 3, 5, 10, 15, 20, 30};

// Screensaver auto-off values in minutes: 0=Never, 5, 10, 15, 20, 30, 45, 60
constexpr uint8_t kScreensaverAutoOffCount = 8;
constexpr uint16_t kScreensaverAutoOffMinutes[] = {0, 5, 10, 15, 20, 30, 45, 60};

// Sleep guard (reading protection) values in minutes: 0=Off, 5, 10, 15, 20, 30, 45, 60
constexpr uint8_t kScreensaverSleepGuardCount = 8;
constexpr uint16_t kScreensaverSleepGuardMinutes[] = {0, 5, 10, 15, 20, 30, 45, 60};

constexpr size_t kBookPickerBackIndex = 0;
constexpr size_t kChapterPickerBackIndex = 0;
constexpr size_t kChapterPickerFallbackIndex = 1;
constexpr size_t kWifiNetworksBackIndex = 0;
constexpr size_t kWifiNetworksFirstItemIndex = 1;
constexpr size_t kFocusTimerGenreBackIndex = 0;
constexpr size_t kFocusTimerGenreFirstIndex = 1;
constexpr const char *kPrefsNamespace = "rsvp";
constexpr const char *kPrefBookPath = "book";
constexpr const char *kPrefLegacyWordIndex = "word";
constexpr const char *kPrefWpm = "wpm";
constexpr const char *kPrefBrightness = "bright";
constexpr const char *kPrefDarkMode = "dark";
constexpr const char *kPrefNightMode = "night";
constexpr const char *kPrefUiLanguage = "ui_lang";
constexpr const char *kPrefReaderMode = "read_mode";
constexpr const char *kPrefHandedness = "handed";
constexpr const char *kPrefPhantomWords = "phantom_on";
constexpr const char *kPrefFooterMetricMode = "prog_md";
constexpr const char *kPrefBatteryLabelMode = "bat_md";
constexpr const char *kPrefScreensaverMode = "scrn_sv";
constexpr const char *kPrefReaderBatteryVisible = "read_bat";
constexpr const char *kPrefReaderChapterVisible = "read_ch";
constexpr const char *kPrefReaderProgressVisible = "read_pct";
constexpr const char *kPrefSavePointButtonVisible = "sp_btn";
constexpr const char *kPrefReaderFontSize = "font_size";
constexpr const char *kPrefReaderTypeface = "typeface";
constexpr const char *kPrefTypographyFocusHighlight = "type_hlt";
constexpr const char *kPrefFocusColorIndex = "foc_clr";
constexpr const char *kPrefLegacyPacingLong = "pace_len";
constexpr const char *kPrefLegacyPacingComplex = "pace_cpx";
constexpr const char *kPrefLegacyPacingPunctuation = "pace_pnc";
constexpr const char *kPrefPacingLongMs = "pace_lms";
constexpr const char *kPrefPacingComplexMs = "pace_cms";
constexpr const char *kPrefPacingPunctuationMs = "pace_pms";
constexpr const char *kPrefPauseMode = "pause_md";
constexpr const char *kPrefAccurateTime = "time_est_a";
constexpr const char *kPrefTypographyTracking = "type_trk";
constexpr const char *kPrefTypographyAnchor = "type_anc";
constexpr const char *kPrefTypographyGuideWidth = "type_wid";
constexpr const char *kPrefTypographyGuideGap = "type_gap";
constexpr const char *kPrefScrollFontSize = "sc_font";
constexpr const char *kPrefScrollLineSpacing = "sc_line_sp";
constexpr const char *kPrefScrollMargin = "sc_margin";
constexpr const char *kPrefScreensaverTimeout = "scrn_tmo";
constexpr const char *kPrefScreensaverAutoOff = "scrn_aof";
constexpr const char *kPrefScreensaverSleepGuard = "scrn_slp";
constexpr const char *kPrefRecentSeq = "seq";
constexpr const char *kPrefWifiSsid = "wifi_ssid";
constexpr const char *kPrefWifiPass = "wifi_pass";
constexpr const char *kPrefOtaAuto = "ota_auto";
constexpr const char *kPrefOtaOwner = "ota_owner";
constexpr const char *kPrefDevMode = "dev_mode";
constexpr const char *kPrefBleEnabled = "ble_on";
constexpr const char *kPrefShowHelpHints = "help_hints";
constexpr size_t kReaderFontSizeCount = 3;
constexpr size_t kPhantomBeforeCharTargets[] = {64, 96, 144};
constexpr size_t kPhantomAfterCharTargets[] = {96, 144, 208};
constexpr uint32_t kNoSavedWordIndex = 0xFFFFFFFFUL;
constexpr uint16_t kPacingDelayMinMs = 0;
constexpr uint16_t kPacingDelayMaxMs = 600;
constexpr uint16_t kPacingDelayStepMs = 50;
constexpr uint16_t kDefaultPacingDelayMs = 200;
constexpr uint16_t kSettingsWpmMin = 10;
constexpr uint16_t kSettingsWpmLowMax = 100;
constexpr uint16_t kSettingsWpmLowStep = 10;
constexpr uint16_t kSettingsWpmMax = 1000;
constexpr uint16_t kSettingsWpmHighStep = 25;
constexpr int8_t kTypographyTrackingMin = -2;
constexpr int8_t kTypographyTrackingMax = 3;
constexpr uint8_t kTypographyAnchorMin = 30;
constexpr uint8_t kTypographyAnchorMax = 40;
constexpr uint8_t kLeftHandAnchorOffset = 20;
constexpr uint8_t kLeftHandAnchorMin = kTypographyAnchorMin + kLeftHandAnchorOffset;
constexpr uint8_t kLeftHandAnchorMax = kTypographyAnchorMax + kLeftHandAnchorOffset;
constexpr uint8_t kTypographyGuideWidthMin = 12;
constexpr uint8_t kTypographyGuideWidthMax = 30;
constexpr uint8_t kTypographyGuideWidthStep = 2;
constexpr uint8_t kTypographyGuideGapMin = 2;
constexpr uint8_t kTypographyGuideGapMax = 8;
constexpr const char *kTypographyPreviewWords[] = {
    "minimum",
    "encyclopaedia",
    "state-of-the-art",
    "HTTP/2",
    "well-known",
    "rhythms",
    "illumination",
    "WAVEFORM",
    "I",
};
constexpr size_t kTypographyPreviewWordCount =
    sizeof(kTypographyPreviewWords) / sizeof(kTypographyPreviewWords[0]);
constexpr size_t kWifiPasswordMaxLength = 63;
constexpr uint16_t kKeyboardMarginX = 8;
constexpr uint16_t kKeyboardTopY = 48;
constexpr uint16_t kKeyboardRowGap = 4;
constexpr uint16_t kKeyboardRowHeight = 27;

void logApp(const char *message) {
  ESP_LOGI(kAppTag, "%s", message);
  Serial.printf("[app] %s\n", message);
}

String displayNameForPath(const String &path) {
  const int separator = path.lastIndexOf('/');
  String name = separator >= 0 ? path.substring(separator + 1) : path;
  String lowered = name;
  lowered.toLowerCase();
  if (lowered.endsWith(".txt")) {
    name.remove(name.length() - 4);
  }
  if (lowered.endsWith(".rsvp")) {
    name.remove(name.length() - 5);
  }
  return name;
}

uint32_t hashBookPath(const String &path) {
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < path.length(); ++i) {
    hash ^= static_cast<uint8_t>(path[i]);
    hash *= 16777619UL;
  }
  return hash;
}

int clampIntSetting(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

int nextCyclicSetting(int value, int minValue, int maxValue, int step = 1) {
  step = std::max(1, step);
  const int normalized = clampIntSetting(value, minValue, maxValue);
  int next = normalized + step;
  if (next > maxValue) {
    next = minValue;
  }
  return next;
}

uint16_t nextReaderWpmSetting(uint16_t value) {
  int normalized = clampIntSetting(value, kSettingsWpmMin, kSettingsWpmMax);
  if (normalized < static_cast<int>(kSettingsWpmLowMax)) {
    normalized += kSettingsWpmLowStep;
    if (normalized > static_cast<int>(kSettingsWpmLowMax)) {
      normalized = kSettingsWpmLowMax;
    }
    return static_cast<uint16_t>(normalized);
  }

  int next = normalized + kSettingsWpmHighStep;
  if (next > static_cast<int>(kSettingsWpmMax)) {
    next = kSettingsWpmMin;
  }
  return static_cast<uint16_t>(next);
}

DisplayManager::TypographyConfig defaultTypographyConfig() {
  return DisplayManager::TypographyConfig();
}

bool wifiNetworkRequiresPassword(uint8_t authMode) {
  return static_cast<wifi_auth_mode_t>(authMode) != WIFI_AUTH_OPEN;
}

String wifiSecurityLabel(uint8_t authMode) {
  return wifiNetworkRequiresPassword(authMode) ? "Secure" : "Open";
}

String maskedValue(const String &value) {
  String masked;
  masked.reserve(value.length());
  for (size_t i = 0; i < value.length(); ++i) {
    masked += '*';
  }
  return masked;
}

const char *keyboardRowText(uint8_t modeValue, size_t rowIndex) {
  static constexpr const char *kLowerRows[] = {
      "qwertyuiop",
      "asdfghjkl",
      "zxcvbnm",
  };
  static constexpr const char *kUpperRows[] = {
      "QWERTYUIOP",
      "ASDFGHJKL",
      "ZXCVBNM",
  };
  static constexpr const char *kSymbolRows[] = {
      "1234567890",
      "!@#$%^&*?",
      "-_=+/:;.,",
  };

  if (rowIndex >= 3) {
    return "";
  }

  switch (modeValue) {
    case 1:
      return kUpperRows[rowIndex];
    case 2:
      return kSymbolRows[rowIndex];
    default:
      return kLowerRows[rowIndex];
  }
}

String storedOrFallbackLabel(const String &value, const String &fallback) {
  return value.isEmpty() ? fallback : value;
}

size_t packedLifeWordCount(size_t cellCount) { return (cellCount + 31U) / 32U; }

bool packedLifeCellAlive(const std::vector<uint32_t> &cells, size_t index) {
  const size_t word = index / 32U;
  if (word >= cells.size()) {
    return false;
  }
  return (cells[word] & (1UL << (index % 32U))) != 0;
}

void setPackedLifeCell(std::vector<uint32_t> &cells, size_t index, bool alive) {
  const size_t word = index / 32U;
  if (word >= cells.size()) {
    return;
  }
  const uint32_t mask = 1UL << (index % 32U);
  if (alive) {
    cells[word] |= mask;
  } else {
    cells[word] &= ~mask;
  }
}

uint32_t advanceStandbyRng(uint32_t &rng) {
  rng = (rng * 1664525UL) + 1013904223UL;
  return rng;
}

struct LifePoint {
  int8_t x;
  int8_t y;
};

void setPackedLifeCellAt(std::vector<uint32_t> &cells, uint16_t columns, uint16_t rows, int x,
                         int y, bool alive) {
  if (x < 0 || y < 0 || x >= static_cast<int>(columns) || y >= static_cast<int>(rows)) {
    return;
  }
  setPackedLifeCell(cells, static_cast<size_t>(y) * columns + static_cast<size_t>(x), alive);
}

void clearPackedLifeRect(std::vector<uint32_t> &cells, uint16_t columns, uint16_t rows, int x,
                         int y, int width, int height) {
  const int xEnd = std::min(static_cast<int>(columns), x + width);
  const int yEnd = std::min(static_cast<int>(rows), y + height);
  for (int cy = std::max(0, y); cy < yEnd; ++cy) {
    for (int cx = std::max(0, x); cx < xEnd; ++cx) {
      setPackedLifeCellAt(cells, columns, rows, cx, cy, false);
    }
  }
}

void stampPackedLifePattern(std::vector<uint32_t> &cells, uint16_t columns, uint16_t rows,
                            const LifePoint *points, size_t pointCount, int originX,
                            int originY) {
  for (size_t i = 0; i < pointCount; ++i) {
    setPackedLifeCellAt(cells, columns, rows, originX + points[i].x, originY + points[i].y, true);
  }
}

void clearAndStampPackedLifePattern(std::vector<uint32_t> &cells, uint16_t columns, uint16_t rows,
                                    const LifePoint *points, size_t pointCount, int originX,
                                    int originY, int width, int height) {
  if (originX < 0 || originY < 0 || originX + width > static_cast<int>(columns) ||
      originY + height > static_cast<int>(rows)) {
    return;
  }
  constexpr int kPatternMargin = 5;
  clearPackedLifeRect(cells, columns, rows, originX - kPatternMargin, originY - kPatternMargin,
                      width + kPatternMargin * 2, height + kPatternMargin * 2);
  stampPackedLifePattern(cells, columns, rows, points, pointCount, originX, originY);
}

constexpr LifePoint kLifeGlider[] = {
    {1, 0},
    {2, 1},
    {0, 2},
    {1, 2},
    {2, 2},
};

constexpr LifePoint kLifeLightweightSpaceship[] = {
    {1, 0}, {4, 0}, {0, 1}, {0, 2}, {4, 2}, {0, 3}, {1, 3}, {2, 3}, {3, 3},
};

constexpr LifePoint kLifePentadecathlon[] = {
    {2, 0}, {2, 1}, {1, 2}, {3, 2}, {2, 3}, {2, 4},
    {2, 5}, {2, 6}, {1, 7}, {3, 7}, {2, 8}, {2, 9},
};

constexpr LifePoint kLifePulsar[] = {
    {2, 0},  {3, 0},  {4, 0},  {8, 0},  {9, 0},  {10, 0}, {0, 2},  {5, 2},
    {7, 2},  {12, 2}, {0, 3},  {5, 3},  {7, 3},  {12, 3}, {0, 4},  {5, 4},
    {7, 4},  {12, 4}, {2, 5},  {3, 5},  {4, 5},  {8, 5},  {9, 5},  {10, 5},
    {2, 7},  {3, 7},  {4, 7},  {8, 7},  {9, 7},  {10, 7}, {0, 8},  {5, 8},
    {7, 8},  {12, 8}, {0, 9},  {5, 9},  {7, 9},  {12, 9}, {0, 10}, {5, 10},
    {7, 10}, {12, 10}, {2, 12}, {3, 12}, {4, 12}, {8, 12}, {9, 12}, {10, 12},
};

constexpr LifePoint kLifeGosperGliderGun[] = {
    {24, 0}, {22, 1}, {24, 1}, {12, 2}, {13, 2}, {20, 2}, {21, 2}, {34, 2}, {35, 2},
    {11, 3}, {15, 3}, {20, 3}, {21, 3}, {34, 3}, {35, 3}, {0, 4},  {1, 4},
    {10, 4}, {16, 4}, {20, 4}, {21, 4}, {0, 5},  {1, 5},  {10, 5}, {14, 5},
    {16, 5}, {17, 5}, {22, 5}, {24, 5}, {10, 6}, {16, 6}, {24, 6}, {11, 7},
    {15, 7}, {12, 8}, {13, 8},
};

void copyOtaLabel(char *destination, size_t destinationSize, const String &source) {
  if (destination == nullptr || destinationSize == 0) {
    return;
  }

  const size_t copyLength = std::min(destinationSize - 1, source.length());
  for (size_t i = 0; i < copyLength; ++i) {
    destination[i] = source[i];
  }
  destination[copyLength] = '\0';
}

bool sdCardFolderRepairNeeded(const StorageManager::DiagnosticResult &result) {
  return result.mounted &&
         (!result.booksDirectory || !result.bookFilesDirectory ||
          !result.articleFilesDirectory || !result.configDirectory);
}

DisplayManager::ReaderTypeface readerTypefaceFromSetting(uint8_t value) {
  switch (static_cast<DisplayManager::ReaderTypeface>(value)) {
    case DisplayManager::ReaderTypeface::Standard:
    case DisplayManager::ReaderTypeface::OpenDyslexic:
    case DisplayManager::ReaderTypeface::AtkinsonHyperlegible:
      return static_cast<DisplayManager::ReaderTypeface>(value);
  }
  return DisplayManager::ReaderTypeface::Standard;
}

DisplayManager::ReaderTypeface nextReaderTypeface(DisplayManager::ReaderTypeface current) {
  switch (readerTypefaceFromSetting(static_cast<uint8_t>(current))) {
    case DisplayManager::ReaderTypeface::Standard:
      return DisplayManager::ReaderTypeface::AtkinsonHyperlegible;
    case DisplayManager::ReaderTypeface::AtkinsonHyperlegible:
      return DisplayManager::ReaderTypeface::OpenDyslexic;
    case DisplayManager::ReaderTypeface::OpenDyslexic:
    default:
      return DisplayManager::ReaderTypeface::Standard;
  }
}

App::ReaderMode readerModeFromSetting(uint8_t value) {
  switch (value) {
    case static_cast<uint8_t>(App::ReaderMode::Scroll):
    case 2:  // Migrate the removed word-scroll mode to page scroll.
      return App::ReaderMode::Scroll;
    case static_cast<uint8_t>(App::ReaderMode::Rsvp):
    default:
      return App::ReaderMode::Rsvp;
  }
}

App::HandednessMode handednessModeFromSetting(uint8_t value) {
  switch (value) {
    case static_cast<uint8_t>(App::HandednessMode::Left):
      return App::HandednessMode::Left;
    case static_cast<uint8_t>(App::HandednessMode::Right):
    default:
      return App::HandednessMode::Right;
  }
}

App::HandednessMode nextHandednessMode(App::HandednessMode current) {
  switch (handednessModeFromSetting(static_cast<uint8_t>(current))) {
    case App::HandednessMode::Left:
      return App::HandednessMode::Right;
    case App::HandednessMode::Right:
    default:
      return App::HandednessMode::Left;
  }
}

App::ReaderMode nextReaderMode(App::ReaderMode current) {
  switch (readerModeFromSetting(static_cast<uint8_t>(current))) {
    case App::ReaderMode::Rsvp:
      return App::ReaderMode::Scroll;
    case App::ReaderMode::Scroll:
    default:
      return App::ReaderMode::Rsvp;
  }
}

uint16_t pacingDelayMsForLegacyLevel(uint8_t levelIndex) {
  constexpr uint16_t kLegacyPacingDelayMs[] = {100, 150, 200, 250, 300};
  constexpr size_t kLegacyPacingLevelCount =
      sizeof(kLegacyPacingDelayMs) / sizeof(kLegacyPacingDelayMs[0]);

  if (levelIndex >= kLegacyPacingLevelCount) {
    levelIndex = 2;
  }
  return kLegacyPacingDelayMs[levelIndex];
}

uint16_t loadPacingDelayMs(Preferences &preferences, const char *key, const char *legacyKey) {
  if (preferences.isKey(key)) {
    return static_cast<uint16_t>(
        clampIntSetting(preferences.getUShort(key, kDefaultPacingDelayMs), kPacingDelayMinMs,
                        kPacingDelayMaxMs));
  }

  if (preferences.isKey(legacyKey)) {
    const uint16_t migratedDelayMs =
        pacingDelayMsForLegacyLevel(preferences.getUChar(legacyKey, 2));
    preferences.putUShort(key, migratedDelayMs);
    return migratedDelayMs;
  }

  return kDefaultPacingDelayMs;
}

}  // namespace

App::App() : button_(BoardConfig::PIN_BOOT_BUTTON), powerButton_(BoardConfig::PIN_PWR_BUTTON) {}

void App::begin() {
  BoardConfig::begin();
  button_.begin();
  powerButton_.begin();
  bootButtonReleasedSinceBoot_ = !button_.isHeld();
  bootButtonLongPressHandled_ = false;
  powerButtonReleasedSinceBoot_ = !powerButton_.isHeld();
  powerButtonLongPressHandled_ = false;
  storage_.setStatusCallback(&App::handleStorageStatus, this);
  preferences_.begin(kPrefsNamespace, false);
  pluginManager_.begin(preferences_);
  brightnessLevelIndex_ = preferences_.getUChar(kPrefBrightness, brightnessLevelIndex_);
  if (brightnessLevelIndex_ >= kBrightnessLevelCount) {
    brightnessLevelIndex_ = kBrightnessLevelCount - 1;
  }
  phantomWordsEnabled_ = preferences_.getBool(kPrefPhantomWords, phantomWordsEnabled_);
  readerBatteryVisibleWhilePlaying_ =
      preferences_.getBool(kPrefReaderBatteryVisible, readerBatteryVisibleWhilePlaying_);
  readerChapterVisibleWhilePlaying_ =
      preferences_.getBool(kPrefReaderChapterVisible, readerChapterVisibleWhilePlaying_);
  readerProgressVisibleWhilePlaying_ =
      preferences_.getBool(kPrefReaderProgressVisible, readerProgressVisibleWhilePlaying_);
  savePointButtonVisible_ =
      preferences_.getBool(kPrefSavePointButtonVisible, savePointButtonVisible_);
  showHelpHints_ = preferences_.getBool(kPrefShowHelpHints, showHelpHints_);
  uiLanguage_ =
      Localization::sanitizeLanguage(preferences_.getUChar(
          kPrefUiLanguage, static_cast<uint8_t>(uiLanguage_)));
  readerMode_ = readerModeFromSetting(
      preferences_.getUChar(kPrefReaderMode, static_cast<uint8_t>(readerMode_)));
  handednessMode_ = handednessModeFromSetting(
      preferences_.getUChar(kPrefHandedness, static_cast<uint8_t>(handednessMode_)));
  readerFontSizeIndex_ = preferences_.getUChar(kPrefReaderFontSize, readerFontSizeIndex_);
  if (readerFontSizeIndex_ >= kReaderFontSizeCount) {
    readerFontSizeIndex_ = 0;
  }
  scrollFontSize_ = preferences_.getUChar(kPrefScrollFontSize, scrollFontSize_);
  if (scrollFontSize_ > 8) scrollFontSize_ = 4;
  scrollLineSpacing_ = preferences_.getUChar(kPrefScrollLineSpacing, scrollLineSpacing_);
  if (scrollLineSpacing_ > 2) scrollLineSpacing_ = 1;
  scrollMargin_ = preferences_.getUChar(kPrefScrollMargin, scrollMargin_);
  if (scrollMargin_ > 2) scrollMargin_ = 1;
  switch (preferences_.getUChar(kPrefFooterMetricMode,
                                static_cast<uint8_t>(footerMetricMode_))) {
    case static_cast<uint8_t>(FooterMetricMode::ChapterTime):
      footerMetricMode_ = FooterMetricMode::ChapterTime;
      break;
    case static_cast<uint8_t>(FooterMetricMode::BookTime):
      footerMetricMode_ = FooterMetricMode::BookTime;
      break;
    case static_cast<uint8_t>(FooterMetricMode::Percentage):
    default:
      footerMetricMode_ = FooterMetricMode::Percentage;
      break;
  }
  switch (preferences_.getUChar(kPrefBatteryLabelMode,
                                static_cast<uint8_t>(batteryLabelMode_))) {
    case static_cast<uint8_t>(BatteryLabelMode::TimeRemaining):
      batteryLabelMode_ = BatteryLabelMode::TimeRemaining;
      break;
    case static_cast<uint8_t>(BatteryLabelMode::Voltage):
      batteryLabelMode_ = BatteryLabelMode::Voltage;
      break;
    case static_cast<uint8_t>(BatteryLabelMode::Percent):
    default:
      batteryLabelMode_ = BatteryLabelMode::Percent;
      break;
  }
  switch (preferences_.getUChar(kPrefScreensaverMode, static_cast<uint8_t>(screensaverMode_))) {
    case static_cast<uint8_t>(ScreensaverMode::Maze):
      screensaverMode_ = ScreensaverMode::Maze;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Voronoi):
      screensaverMode_ = ScreensaverMode::Voronoi;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Stars):
      screensaverMode_ = ScreensaverMode::Stars;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Matrix):
      screensaverMode_ = ScreensaverMode::Matrix;
      break;
    case static_cast<uint8_t>(ScreensaverMode::ScreenOff):
      screensaverMode_ = ScreensaverMode::ScreenOff;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Life):
    default:
      screensaverMode_ = ScreensaverMode::Life;
      break;
  }
  screensaverTimeoutIndex_ = preferences_.getUChar(kPrefScreensaverTimeout, 2);
  if (screensaverTimeoutIndex_ >= kScreensaverTimeoutCount) {
    screensaverTimeoutIndex_ = 2;
  }
  screensaverAutoOffIndex_ = preferences_.getUChar(kPrefScreensaverAutoOff, 0);
  if (screensaverAutoOffIndex_ >= kScreensaverAutoOffCount) {
    screensaverAutoOffIndex_ = 0;
  }
  screensaverSleepGuardIndex_ = preferences_.getUChar(kPrefScreensaverSleepGuard, 0);
  if (screensaverSleepGuardIndex_ >= kScreensaverSleepGuardCount) {
    screensaverSleepGuardIndex_ = 0;
  }
  switch (preferences_.getUChar(kPrefPauseMode, static_cast<uint8_t>(pauseMode_))) {
    case static_cast<uint8_t>(PauseMode::Instant):
      pauseMode_ = PauseMode::Instant;
      break;
    case static_cast<uint8_t>(PauseMode::SentenceEnd):
    default:
      pauseMode_ = PauseMode::SentenceEnd;
      break;
  }
  pacingLongWordDelayMs_ =
      loadPacingDelayMs(preferences_, kPrefPacingLongMs, kPrefLegacyPacingLong);
  pacingComplexWordDelayMs_ =
      loadPacingDelayMs(preferences_, kPrefPacingComplexMs, kPrefLegacyPacingComplex);
  pacingPunctuationDelayMs_ =
      loadPacingDelayMs(preferences_, kPrefPacingPunctuationMs, kPrefLegacyPacingPunctuation);
  accurateTimeEstimateEnabled_ = true;
  typographyConfig_ = defaultTypographyConfig();
  typographyConfig_.typeface = readerTypefaceFromSetting(
      preferences_.getUChar(kPrefReaderTypeface, static_cast<uint8_t>(typographyConfig_.typeface)));
  typographyConfig_.focusHighlight =
      preferences_.getBool(kPrefTypographyFocusHighlight, typographyConfig_.focusHighlight);
  typographyConfig_.trackingPx = static_cast<int8_t>(clampIntSetting(
      preferences_.getChar(kPrefTypographyTracking, typographyConfig_.trackingPx),
      kTypographyTrackingMin, kTypographyTrackingMax));
  typographyConfig_.anchorPercent = static_cast<uint8_t>(clampIntSetting(
      preferences_.getUChar(kPrefTypographyAnchor, typographyConfig_.anchorPercent),
      kTypographyAnchorMin, kTypographyAnchorMax));
  typographyConfig_.guideHalfWidth = static_cast<uint8_t>(clampIntSetting(
      preferences_.getUChar(kPrefTypographyGuideWidth, typographyConfig_.guideHalfWidth),
      kTypographyGuideWidthMin, kTypographyGuideWidthMax));
  typographyConfig_.guideGap = static_cast<uint8_t>(clampIntSetting(
      preferences_.getUChar(kPrefTypographyGuideGap, typographyConfig_.guideGap),
      kTypographyGuideGapMin, kTypographyGuideGapMax));
  darkMode_ = preferences_.getBool(kPrefDarkMode, darkMode_);
  nightMode_ = preferences_.getBool(kPrefNightMode, nightMode_);
  display_.setFocusColorIndex(preferences_.getUChar(kPrefFocusColorIndex, 0));
  applyHandednessSettings(0, false);
  applyDisplayPreferences(0, false);
  applyTypographySettings(0, false);
  applyPacingSettings();
  bootStartedMs_ = millis();
  lastStateLogMs_ = bootStartedMs_;
  lastScrollAnimationRenderMs_ = 0;
  Serial.printf("[app] version=%s\n", otaUpdater_.currentVersion().c_str());

  logApp("Initializing hardware modules");
  const bool displayReady = display_.begin();

  // Show boot splash immediately after display init — before anything else.
  // This ensures the user sees the animation ASAP with no black screen gap.
  if (displayReady) {
    display_.renderBootSplash();
    logApp("Display init ok");
  } else {
    ESP_LOGE(kAppTag, "Display init failed");
    Serial.println("[app] Display init failed");
  }

  // Initialize remaining hardware while FLOWER is still visible on screen.
  updateBatteryStatus(bootStartedMs_, true);
  touchInitialized_ = touch_.begin();
  audio_.begin();
  focusTimer_.begin();

#if RSVP_USB_TRANSFER_ENABLED && RSVP_USB_TRANSFER_AUTO_START
  state_ = AppState::Booting;
  Serial.println("[app] USB transfer auto-start active");
  enterUsbTransfer(millis());
  return;
#endif

  storageReady_ = storage_.begin();
  const uint16_t savedWpm = preferences_.getUShort(kPrefWpm, reader_.wpm());
  reader_.setWpm(savedWpm);

  pendingBootBookLoad_ = prepareBootBookLoad();
  if (!pendingBootBookLoad_) {
    usingStorageBook_ = false;
    chapterMarkers_.clear();
    paragraphStarts_.clear();
    currentBookPath_ = "";
    currentBookTitle_ = "Demo";
    reader_.begin(bootStartedMs_);
    invalidateContextPreviewWindow();
    rebuildTimeEstimateCache();
    Serial.println("[app] using built-in demo text");
  } else {
    currentBookTitle_ = storage_.bookDisplayName(pendingBootBookIndex_);
    if (currentBookTitle_.isEmpty()) {
      currentBookTitle_ = tr(TrKey::LoadingBook);
    }
  }

  maybeAutoCheckForUpdates(bootStartedMs_);

  // Auto-start BLE peripheral jeśli klient włączył go wcześniej.
  // Stan jest persistowany, więc telefon „znajduje" Flowera od razu po
  // boocie, bez wchodzenia do menu Polaczenia po każdym restarcie.
  if (preferences_.getBool(kPrefBleEnabled, false)) {
    ble_.begin(this);
    Serial.printf("[app] BLE auto-started (name=%s)\n", ble_.deviceName().c_str());
  }

  // Auto-start companion sync AP for 30 seconds on boot.
  // If no client connects within the timeout, AP shuts down to save power.
  {
    CompanionSyncManager::Config syncConfig;
    syncConfig.wifiSsid = "";
    syncConfig.wifiPassword = "";
    if (companionSync_.begin(syncConfig)) {
      autoSyncActive_ = true;
      autoSyncStartedMs_ = millis();
      autoSyncClientConnected_ = false;
      Serial.println("[app] auto-sync AP started (30s timeout)");
    } else {
      Serial.println("[app] auto-sync AP failed to start");
    }
  }

  Serial.printf("[app] WPM=%u interval=%lu ms\n", reader_.wpm(),
                static_cast<unsigned long>(reader_.wordIntervalMs()));

  state_ = AppState::Booting;
  lastActivityMs_ = millis();
  Serial.println("[app] READY splash active");
}

void App::update(uint32_t nowMs) {
  button_.update(nowMs);
  powerButton_.update(nowMs);
  const bool standbyComboConsumed = handleStandbyCombo(nowMs);
  if (!standbyComboConsumed) {
    handleBootButton(nowMs);
    handlePowerButton(nowMs);
  }
  if (powerOffStarted_) {
    return;
  }

  const bool batteryChanged = updateBatteryStatus(nowMs);
  if (powerOffStarted_) {
    return;
  }

  if (batteryWarningOverlayVisible_) {
    updateBatteryWarningOverlay(nowMs);
    if (batteryWarningOverlayVisible_) {
      if (nowMs - lastStateLogMs_ > 1500) {
        lastStateLogMs_ = nowMs;
        ESP_LOGI(kAppTag, "state=%s", stateName(state_));
        Serial.printf("[app] state=%s ms=%lu\n", stateName(state_),
                      static_cast<unsigned long>(nowMs));
      }
      return;
    }
  }

  if (state_ == AppState::Standby) {
    handleTouch(nowMs);
    updateStandbyScreensaver(nowMs);
    if (nowMs - lastStateLogMs_ > 1500) {
      lastStateLogMs_ = nowMs;
      ESP_LOGI(kAppTag, "state=%s", stateName(state_));
      Serial.printf("[app] state=%s ms=%lu\n", stateName(state_),
                    static_cast<unsigned long>(nowMs));
    }
    return;
  }

  pollOtaCheckResult(nowMs);
  updateState(nowMs);
  loadPendingBootBook(nowMs);
  maybeOpenUpdateConfirm(nowMs);
  updateFocusTimer(nowMs);
  updateReader(nowMs);
  handleTouch(nowMs);

  // Screensaver idle timeout: activate screensaver if user hasn't interacted
  if (lastActivityMs_ > 0 && state_ != AppState::Booting &&
      state_ != AppState::UsbTransfer && state_ != AppState::CompanionSync &&
      state_ != AppState::Sleeping && !powerOffStarted_) {
    const uint32_t timeoutMs =
        static_cast<uint32_t>(kScreensaverTimeoutMinutes[screensaverTimeoutIndex_]) * 60000UL;
    const uint32_t elapsed = nowMs - lastActivityMs_;

    if (state_ == AppState::Playing) {
      // Sleep guard: if reading and no touch for X minutes, pause and enter standby
      if (screensaverSleepGuardIndex_ > 0) {
        const uint32_t sleepGuardMs =
            static_cast<uint32_t>(kScreensaverSleepGuardMinutes[screensaverSleepGuardIndex_]) * 60000UL;
        if (elapsed >= sleepGuardMs) {
          Serial.println("[app] sleep guard: no touch while reading, entering standby");
          saveReadingPosition(true);
          enterStandby(nowMs);
          return;
        }
      }
    } else if (state_ == AppState::Paused || state_ == AppState::Menu) {
      // Normal idle timeout: enter screensaver when paused/menu idle
      if (elapsed >= timeoutMs) {
        Serial.println("[app] idle timeout: entering standby screensaver");
        enterStandby(nowMs);
        return;
      }
    }
  }

  updateWpmFeedback(nowMs);
  maybeSaveReadingPosition(nowMs);
  updateTimeEstimateBuild(nowMs);

  // BLE: drenuj komendy zakolejkowane przez host task (set-dev-mode itp.)
  // i sprawdź czy zmiana stanu (connect/disconnect) wymaga odświeżenia
  // menu Polaczenia. Bez tego label „Bluetooth: WLACZONY" zostaje stale
  // gdy telefon się podłączy w trakcie wyświetlania menu.
  ble_.update();
  if (ble_.consumeMenuDirty() && state_ == AppState::Menu &&
      menuScreen_ == MenuScreen::SettingsConnectivity) {
    rebuildSettingsMenuItems();
    renderSettings();
  }

  // Auto-sync AP timeout: shut down if no client connects within 30 seconds.
  // Once a client connects, keep AP alive until user manually exits sync
  // or enters CompanionSync state from menu (which takes over).
  if (autoSyncActive_ && state_ != AppState::CompanionSync) {
    companionSync_.update();
    const uint8_t clients = WiFi.softAPgetStationNum();
    if (clients > 0 && !autoSyncClientConnected_) {
      autoSyncClientConnected_ = true;
      Serial.println("[app] auto-sync: client connected, keeping AP alive");
    }
    if (!autoSyncClientConnected_ && (nowMs - autoSyncStartedMs_ >= 30000)) {
      Serial.println("[app] auto-sync: 30s timeout, no client — shutting down AP");
      companionSync_.end();
      autoSyncActive_ = false;
    }
  }

  if (batteryChanged && (state_ == AppState::Paused || state_ == AppState::Playing)) {
    renderActiveReader(nowMs);
  } else if (batteryChanged && state_ == AppState::Menu) {
    renderMenu();
  }

  if (nowMs - lastStateLogMs_ > 1500) {
    lastStateLogMs_ = nowMs;
    ESP_LOGI(kAppTag, "state=%s", stateName(state_));
    Serial.printf("[app] state=%s ms=%lu\n", stateName(state_),
                  static_cast<unsigned long>(nowMs));
  }
}

const char *App::stateName(AppState state) const {
  switch (state) {
    case AppState::Booting:
      return "Booting";
    case AppState::Paused:
      return "Paused";
    case AppState::Playing:
      return "Playing";
    case AppState::Menu:
      return "Menu";
    case AppState::CompanionSync:
      return "CompanionSync";
    case AppState::UsbTransfer:
      return "UsbTransfer";
    case AppState::Standby:
      return "Standby";
    case AppState::Sleeping:
      return "Sleeping";
  }
  return "Unknown";
}

const char *App::touchPhaseName(TouchPhase phase) const {
  switch (phase) {
    case TouchPhase::Start:
      return "Start";
    case TouchPhase::Move:
      return "Move";
    case TouchPhase::End:
      return "End";
  }
  return "Unknown";
}

void App::setState(AppState nextState, uint32_t nowMs) {
  if (nextState == state_) {
    return;
  }

  // Reset activity timer on any state change (user interaction drove us here)
  lastActivityMs_ = nowMs;

  const AppState previousState = state_;
  if (previousState == AppState::Menu && nextState != AppState::Menu) {
    flushPendingTimeEstimateRebuild();
  }

  if (nextState != AppState::Paused) {
    pausedTouch_.active = false;
    pausedTouchIntent_ = TouchIntent::None;
    contextViewVisible_ = false;
    invalidateContextPreviewWindow();
    wpmFeedbackVisible_ = false;
  }
  if (nextState != AppState::Playing) {
    touchPlayHeld_ = false;
    playLocked_ = false;
    pauseAtSentenceEndRequested_ = false;
    chapterTransitionVisible_ = false;
  }
  if (nextState != AppState::Paused && nextState != AppState::Playing) {
    resetReaderTapTracking();
  }

  state_ = nextState;

  switch (state_) {
    case AppState::Paused:
      renderActiveReader(nowMs);
      break;
    case AppState::Playing:
      reader_.start(nowMs);
      renderActiveReader(nowMs);
      break;
    case AppState::Menu:
      renderMenu();
      break;
    case AppState::CompanionSync:
      if (companionSync_.hasQrCode()) {
        display_.renderStatusWithQr("Wi-Fi", companionSync_.statusLine1(),
                                    companionSync_.qrCodeData(), companionSync_.qrCodeSize());
      } else {
        display_.renderStatus("Sync", companionSync_.statusLine1(), companionSync_.statusLine2());
      }
      break;
    case AppState::UsbTransfer:
      display_.renderStatus("USB", tr(TrKey::PreparingSD),
                            tr(TrKey::EjectWhenDone));
      break;
    case AppState::Standby:
      seedStandbyScreensaver(nowMs);
      updateStandbyScreensaver(nowMs, true);
      break;
    case AppState::Sleeping:
      display_.renderCenteredWord("SLEEP");
      break;
    case AppState::Booting:
      display_.renderCenteredWord("READY");
      break;
  }

  if (state_ == AppState::Paused && previousState == AppState::Playing) {
    saveReadingPosition(true);
  }

  ESP_LOGI(kAppTag, "state -> %s", stateName(state_));
  Serial.printf("[app] state -> %s at %lu ms\n", stateName(state_),
                static_cast<unsigned long>(nowMs));
}

void App::updateState(uint32_t nowMs) {
  if (state_ == AppState::Booting) {
    if (nowMs - bootStartedMs_ < kBootSplashMs) {
      return;
    }

    // Pierwsze uruchomienie po flashowaniu — pokaż welcome wizard zamiast
    // od razu otwierać czytnik. Ważne: ustaw menuScreen_ ZANIM zawołamy
    // setState(Menu), bo setState→renderMenu() patrzy na menuScreen_ i bez
    // tego renderuje Main menu zamiast naszej listy języków.
    if (!preferences_.getBool(kPrefSetupDone, false)) {
      menuScreen_ = MenuScreen::WelcomeLanguage;
      settingsSelectedIndex_ = 0;
      rebuildSettingsMenuItems();
      setState(AppState::Menu, nowMs);
      return;
    }

    // Setup done but tutorial not finished — resume tutorial
    tutorialCompleted_ = preferences_.getBool(kPrefTutorialDone, false);
    if (!tutorialCompleted_) {
      menuScreen_ = MenuScreen::TutorialStep1;
      setState(AppState::Menu, nowMs);
      return;
    }

    setState((touchPlayHeld_ || playLocked_ || pauseAtSentenceEndRequested_) ? AppState::Playing
                                                                              : AppState::Paused,
             nowMs);
    return;
  }

  if (state_ == AppState::UsbTransfer) {
    updateUsbTransfer(nowMs);
    return;
  }

  if (state_ == AppState::CompanionSync) {
    updateCompanionSync(nowMs);
    return;
  }

  if (state_ == AppState::Menu || state_ == AppState::Standby || state_ == AppState::Sleeping) {
    // Menu, standby, and sleeping state changes are driven by direct input and power events.
    return;
  }

  if (touchPlayHeld_ || playLocked_ || pauseAtSentenceEndRequested_) {
    setState(AppState::Playing, nowMs);
    return;
  }

  setState(AppState::Paused, nowMs);
}

void App::updateReader(uint32_t nowMs) {
  if (state_ != AppState::Playing) {
    return;
  }

  if (updateChapterTransition(nowMs)) {
    return;
  }

  if (!ensureCurrentBookWordAvailable(nowMs)) {
    return;
  }

  if (shouldFinalizeReaderPause(nowMs)) {
    finalizeReaderPause(nowMs);
    return;
  }

  const size_t previousIndex = reader_.currentIndex();
  const bool changed = reader_.update(nowMs, !pauseAtSentenceEndRequested_);
  if (!ensureCurrentBookWordAvailable(nowMs)) {
    return;
  }
  if (changed && maybeStartChapterTransition(previousIndex, reader_.currentIndex(), nowMs)) {
    return;
  }
  if (scrollModeEnabled()) {
    if (changed || nowMs - lastScrollAnimationRenderMs_ >= kScrollAnimationFrameMs) {
      renderScrollReader(nowMs);
      lastScrollAnimationRenderMs_ = nowMs;
    }
    return;
  }

  if (changed) {
    renderReaderWord();
  }
}

void App::maybeSaveReadingPosition(uint32_t nowMs) {
  if (!usingStorageBook_ || currentBookPath_.isEmpty() || state_ != AppState::Playing) {
    return;
  }

  if (nowMs - lastProgressSaveMs_ < kProgressSaveIntervalMs) {
    return;
  }

  lastProgressSaveMs_ = nowMs;
  saveReadingPosition(false);
}

bool App::handleStandbyCombo(uint32_t nowMs) {
  if (state_ == AppState::Booting || state_ == AppState::UsbTransfer ||
      state_ == AppState::CompanionSync ||
      state_ == AppState::Sleeping || powerOffStarted_ || !bootButtonReleasedSinceBoot_ ||
      !powerButtonReleasedSinceBoot_) {
    return false;
  }

  const bool bothHeld = button_.isHeld() && powerButton_.isHeld();
  if (state_ == AppState::Standby) {
    const bool pastGrace = nowMs - standbyEnteredMs_ >= kStandbyWakeGraceMs;
    if (!bothHeld && !button_.isHeld() && !powerButton_.isHeld() && pastGrace) {
      standbyButtonsReleased_ = true;
    }

    if (bothHeld) {
      if (standbyButtonsReleased_) {
        bootButtonLongPressHandled_ = true;
        powerButtonLongPressHandled_ = true;
        exitStandby(nowMs);
      }
      return true;
    }

    if (standbyComboActive_) {
      standbyComboActive_ = false;
      standbyComboHandled_ = false;
      bootButtonLongPressHandled_ = false;
      powerButtonLongPressHandled_ = false;
      return true;
    }

    return false;
  }

  if (bothHeld) {
    if (!standbyComboActive_) {
      standbyComboActive_ = true;
      standbyComboHandled_ = true;
      standbyComboStartedMs_ = nowMs;
      bootButtonLongPressHandled_ = true;
      powerButtonLongPressHandled_ = true;
      enterStandby(nowMs);
    }
    return true;
  }

  if (standbyComboActive_) {
    standbyComboActive_ = false;
    standbyComboHandled_ = false;
    bootButtonLongPressHandled_ = false;
    powerButtonLongPressHandled_ = false;
    return true;
  }

  return false;
}

void App::handleBootButton(uint32_t nowMs) {
  if (state_ == AppState::Standby) {
    if (!standbyButtonsReleased_ && !button_.isHeld() && !powerButton_.isHeld() &&
        nowMs - standbyEnteredMs_ >= kStandbyWakeGraceMs) {
      standbyButtonsReleased_ = true;
    }
    if (standbyButtonsReleased_ && button_.wasPressedEvent()) {
      bootButtonLongPressHandled_ = true;
      exitStandby(nowMs);
    }
    return;
  }

  if (state_ == AppState::Booting || state_ == AppState::UsbTransfer ||
      state_ == AppState::CompanionSync ||
      state_ == AppState::Sleeping || powerOffStarted_) {
    return;
  }

  if (showingHelpPopup_) {
    // Only dismiss on actual button press, not every frame
    if (button_.wasPressedEvent() || button_.wasReleasedEvent()) {
      dismissHelpPopup(nowMs);
      bootButtonLongPressHandled_ = true;
    }
    return;
  }

  if (!bootButtonReleasedSinceBoot_) {
    if (!button_.isHeld()) {
      bootButtonReleasedSinceBoot_ = true;
    }
    return;
  }

  if (button_.isHeld() || button_.wasPressedEvent() || button_.wasReleasedEvent()) {
    lastActivityMs_ = nowMs;
  }

  if (button_.isHeld() && !bootButtonLongPressHandled_ &&
      button_.heldDurationMs(nowMs) >= kThemeToggleHoldMs) {
    bootButtonLongPressHandled_ = true;
    cycleThemeMode(nowMs);
    return;
  }

  if (!button_.wasReleasedEvent()) {
    return;
  }

  if (bootButtonLongPressHandled_) {
    bootButtonLongPressHandled_ = false;
    return;
  }

  if (button_.lastHoldDurationMs() < kThemeToggleHoldMs) {
    // In settings menu with help enabled → show help instead of cycling brightness
    if (state_ == AppState::Menu && showHelpHints_ && !showingHelpPopup_ &&
        (menuScreen_ == MenuScreen::SettingsDisplay || menuScreen_ == MenuScreen::SettingsPacing)) {
      showHelpForCurrentItem();
      if (showingHelpPopup_) return;  // help was shown, don't cycle brightness
    }
    cycleBrightness();
  }
}

void App::handlePowerButton(uint32_t nowMs) {
  if (!powerButtonReleasedSinceBoot_) {
    if (!powerButton_.isHeld()) {
      powerButtonReleasedSinceBoot_ = true;
    }
    return;
  }

  if (state_ == AppState::Standby) {
    if (!standbyButtonsReleased_ && !button_.isHeld() && !powerButton_.isHeld() &&
        nowMs - standbyEnteredMs_ >= kStandbyWakeGraceMs) {
      standbyButtonsReleased_ = true;
    }
    if (standbyButtonsReleased_ && powerButton_.wasPressedEvent()) {
      powerButtonLongPressHandled_ = true;
      exitStandby(nowMs);
    }
    return;
  }

  if (state_ == AppState::UsbTransfer || state_ == AppState::CompanionSync || powerOffStarted_) {
    return;
  }

  if (powerButtonLongPressHandled_ && powerButton_.isHeld()) {
    return;
  }

  if (powerButton_.isHeld() || powerButton_.wasPressedEvent() || powerButton_.wasReleasedEvent()) {
    lastActivityMs_ = nowMs;
  }

  if (state_ == AppState::Menu && isFocusTimerMenuScreen(menuScreen_) &&
      powerButton_.isHeld() && nowMs - powerButton_.lastEdgeMs() >= kUsbTransferExitHoldMs) {
    powerButtonLongPressHandled_ = true;
    resetFocusTimer();
    menuScreen_ = MenuScreen::Main;
    renderMainMenu();
    return;
  }

  if (powerButton_.isHeld() && nowMs - powerButton_.lastEdgeMs() >= kPowerOffHoldMs) {
    powerButtonLongPressHandled_ = true;
    enterPowerOff(nowMs);
    return;
  }

  if (!powerButton_.wasReleasedEvent()) {
    return;
  }

  if (powerButtonLongPressHandled_) {
    powerButtonLongPressHandled_ = false;
    return;
  }

  toggleMenuFromPowerButton(nowMs);
}

void App::toggleMenuFromPowerButton(uint32_t nowMs) {
  if (state_ == AppState::Booting || state_ == AppState::UsbTransfer ||
      state_ == AppState::CompanionSync || state_ == AppState::Standby ||
      state_ == AppState::Sleeping) {
    return;
  }

  if (state_ == AppState::Menu) {
    if (menuScreen_ == MenuScreen::Main) {
      // Check for triple-tap save point: if we just opened the menu via PWR
      // and get another PWR tap quickly, count towards triple-tap.
      constexpr uint32_t kTripleTapWindowMs = 900;
      if (pwrTapCount_ > 0 && nowMs - pwrFirstTapMs_ <= kTripleTapWindowMs) {
        pwrTapCount_++;
        if (pwrTapCount_ >= 3) {
          pwrTapCount_ = 0;
          createSavePoint(nowMs);
          display_.renderStatus(uiText(UiText::SavePoints),
                                polish("Punkt zapisu dodany", "Save point added"), "");
          delay(1500);
          setState(AppState::Paused, nowMs);
          return;
        }
        // 2nd tap — go back to reading, wait for 3rd
        setState(AppState::Paused, nowMs);
        return;
      }
      // Normal single tap on Main menu = go back to reading
      pwrTapCount_ = 0;
      setState(AppState::Paused, nowMs);
    } else {
      if (menuScreen_ == MenuScreen::WelcomeConnect ||
          menuScreen_ == MenuScreen::WelcomeLanguage ||
          menuScreen_ == MenuScreen::WelcomeTheme ||
          menuScreen_ == MenuScreen::WelcomeHighlightColor ||
          menuScreen_ == MenuScreen::WelcomePacing) {
        finishWelcomeWizard(nowMs);
        return;
      }
      if (menuScreen_ == MenuScreen::TutorialStep1 ||
          menuScreen_ == MenuScreen::TutorialStep2 ||
          menuScreen_ == MenuScreen::TutorialStep3 ||
          menuScreen_ == MenuScreen::TutorialStep4 ||
          menuScreen_ == MenuScreen::TutorialStep5) {
        finishTutorial(nowMs);
        return;
      }
      if (isFocusTimerMenuScreen(menuScreen_)) {
        resetFocusTimer();
      }
      menuScreen_ = MenuScreen::Main;
      renderMainMenu();
    }
    return;
  }

  // Opening menu from Paused/Playing — start triple-tap tracking
  pwrTapCount_ = 1;
  pwrFirstTapMs_ = nowMs;
  openMainMenu(nowMs);
}

void App::openMainMenu(uint32_t nowMs) {
  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  touchPlayHeld_ = false;
  menuScreen_ = MenuScreen::Main;
  menuSelectedIndex_ = MenuRead;
  wpmFeedbackVisible_ = false;
  contextViewVisible_ = false;
  if (state_ == AppState::Playing) {
    saveReadingPosition(true);
  }
  setState(AppState::Menu, nowMs);
}

uint8_t App::currentBrightnessPercent() const {
  return nightMode_ ? kNightBrightnessLevels[brightnessLevelIndex_]
                    : kBrightnessLevels[brightnessLevelIndex_];
}

void App::applyDisplayPreferences(uint32_t nowMs, bool rerender) {
  display_.setDarkMode(darkMode_);
  display_.setNightMode(nightMode_);
  display_.setBrightnessPercent(currentBrightnessPercent());
  display_.setScrollFontSize(scrollFontSize_);
  display_.setScrollLineSpacing(scrollLineSpacing_);
  display_.setScrollMargin(scrollMargin_);

  if (!rerender) {
    return;
  }

  if (state_ == AppState::Menu) {
    if (isSettingsListScreen()) {
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    }
    renderMenu();
    return;
  }

  if (state_ == AppState::Paused || state_ == AppState::Playing) {
    renderActiveReader(nowMs);
    return;
  }

  if (state_ == AppState::Booting) {
    display_.renderCenteredWord("READY");
  }
}

void App::applyHandednessSettings(uint32_t nowMs, bool rerender) {
  (void)nowMs;
  applyReaderUiOrientation();

  if (!rerender) {
    return;
  }

  if (state_ == AppState::Menu && isSettingsListScreen()) {
    rebuildSettingsMenuItems();
  }

  applyTypographySettings(nowMs);
}

void App::reloadRuntimePreferences(uint32_t nowMs, bool rerender) {
  brightnessLevelIndex_ = preferences_.getUChar(kPrefBrightness, brightnessLevelIndex_);
  if (brightnessLevelIndex_ >= kBrightnessLevelCount) {
    brightnessLevelIndex_ = kBrightnessLevelCount - 1;
  }
  phantomWordsEnabled_ = preferences_.getBool(kPrefPhantomWords, phantomWordsEnabled_);
  readerBatteryVisibleWhilePlaying_ =
      preferences_.getBool(kPrefReaderBatteryVisible, readerBatteryVisibleWhilePlaying_);
  readerChapterVisibleWhilePlaying_ =
      preferences_.getBool(kPrefReaderChapterVisible, readerChapterVisibleWhilePlaying_);
  readerProgressVisibleWhilePlaying_ =
      preferences_.getBool(kPrefReaderProgressVisible, readerProgressVisibleWhilePlaying_);
  savePointButtonVisible_ =
      preferences_.getBool(kPrefSavePointButtonVisible, savePointButtonVisible_);
  showHelpHints_ = preferences_.getBool(kPrefShowHelpHints, showHelpHints_);
  uiLanguage_ =
      Localization::sanitizeLanguage(preferences_.getUChar(
          kPrefUiLanguage, static_cast<uint8_t>(uiLanguage_)));
  readerMode_ = readerModeFromSetting(
      preferences_.getUChar(kPrefReaderMode, static_cast<uint8_t>(readerMode_)));
  handednessMode_ = handednessModeFromSetting(
      preferences_.getUChar(kPrefHandedness, static_cast<uint8_t>(handednessMode_)));
  readerFontSizeIndex_ = preferences_.getUChar(kPrefReaderFontSize, readerFontSizeIndex_);
  if (readerFontSizeIndex_ >= kReaderFontSizeCount) {
    readerFontSizeIndex_ = 0;
  }
  scrollFontSize_ = preferences_.getUChar(kPrefScrollFontSize, scrollFontSize_);
  if (scrollFontSize_ > 8) scrollFontSize_ = 4;
  scrollLineSpacing_ = preferences_.getUChar(kPrefScrollLineSpacing, scrollLineSpacing_);
  if (scrollLineSpacing_ > 2) scrollLineSpacing_ = 1;
  scrollMargin_ = preferences_.getUChar(kPrefScrollMargin, scrollMargin_);
  if (scrollMargin_ > 2) scrollMargin_ = 1;

  switch (preferences_.getUChar(kPrefFooterMetricMode,
                                static_cast<uint8_t>(footerMetricMode_))) {
    case static_cast<uint8_t>(FooterMetricMode::ChapterTime):
      footerMetricMode_ = FooterMetricMode::ChapterTime;
      break;
    case static_cast<uint8_t>(FooterMetricMode::BookTime):
      footerMetricMode_ = FooterMetricMode::BookTime;
      break;
    case static_cast<uint8_t>(FooterMetricMode::Percentage):
    default:
      footerMetricMode_ = FooterMetricMode::Percentage;
      break;
  }

  switch (preferences_.getUChar(kPrefBatteryLabelMode,
                                static_cast<uint8_t>(batteryLabelMode_))) {
    case static_cast<uint8_t>(BatteryLabelMode::TimeRemaining):
      batteryLabelMode_ = BatteryLabelMode::TimeRemaining;
      break;
    case static_cast<uint8_t>(BatteryLabelMode::Voltage):
      batteryLabelMode_ = BatteryLabelMode::Voltage;
      break;
    case static_cast<uint8_t>(BatteryLabelMode::Percent):
    default:
      batteryLabelMode_ = BatteryLabelMode::Percent;
      break;
  }

  switch (preferences_.getUChar(kPrefScreensaverMode, static_cast<uint8_t>(screensaverMode_))) {
    case static_cast<uint8_t>(ScreensaverMode::Maze):
      screensaverMode_ = ScreensaverMode::Maze;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Voronoi):
      screensaverMode_ = ScreensaverMode::Voronoi;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Stars):
      screensaverMode_ = ScreensaverMode::Stars;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Matrix):
      screensaverMode_ = ScreensaverMode::Matrix;
      break;
    case static_cast<uint8_t>(ScreensaverMode::ScreenOff):
      screensaverMode_ = ScreensaverMode::ScreenOff;
      break;
    case static_cast<uint8_t>(ScreensaverMode::Life):
    default:
      screensaverMode_ = ScreensaverMode::Life;
      break;
  }
  screensaverTimeoutIndex_ = preferences_.getUChar(kPrefScreensaverTimeout, screensaverTimeoutIndex_);
  if (screensaverTimeoutIndex_ >= kScreensaverTimeoutCount) {
    screensaverTimeoutIndex_ = 2;
  }
  screensaverAutoOffIndex_ = preferences_.getUChar(kPrefScreensaverAutoOff, screensaverAutoOffIndex_);
  if (screensaverAutoOffIndex_ >= kScreensaverAutoOffCount) {
    screensaverAutoOffIndex_ = 0;
  }
  screensaverSleepGuardIndex_ = preferences_.getUChar(kPrefScreensaverSleepGuard, screensaverSleepGuardIndex_);
  if (screensaverSleepGuardIndex_ >= kScreensaverSleepGuardCount) {
    screensaverSleepGuardIndex_ = 0;
  }

  switch (preferences_.getUChar(kPrefPauseMode, static_cast<uint8_t>(pauseMode_))) {
    case static_cast<uint8_t>(PauseMode::Instant):
      pauseMode_ = PauseMode::Instant;
      break;
    case static_cast<uint8_t>(PauseMode::SentenceEnd):
    default:
      pauseMode_ = PauseMode::SentenceEnd;
      break;
  }

  pacingLongWordDelayMs_ =
      loadPacingDelayMs(preferences_, kPrefPacingLongMs, kPrefLegacyPacingLong);
  pacingComplexWordDelayMs_ =
      loadPacingDelayMs(preferences_, kPrefPacingComplexMs, kPrefLegacyPacingComplex);
  pacingPunctuationDelayMs_ =
      loadPacingDelayMs(preferences_, kPrefPacingPunctuationMs, kPrefLegacyPacingPunctuation);
  accurateTimeEstimateEnabled_ = true;

  typographyConfig_ = defaultTypographyConfig();
  typographyConfig_.typeface = readerTypefaceFromSetting(
      preferences_.getUChar(kPrefReaderTypeface, static_cast<uint8_t>(typographyConfig_.typeface)));
  typographyConfig_.focusHighlight =
      preferences_.getBool(kPrefTypographyFocusHighlight, typographyConfig_.focusHighlight);
  typographyConfig_.trackingPx = static_cast<int8_t>(clampIntSetting(
      preferences_.getChar(kPrefTypographyTracking, typographyConfig_.trackingPx),
      kTypographyTrackingMin, kTypographyTrackingMax));
  typographyConfig_.anchorPercent = static_cast<uint8_t>(clampIntSetting(
      preferences_.getUChar(kPrefTypographyAnchor, typographyConfig_.anchorPercent),
      kTypographyAnchorMin, kTypographyAnchorMax));
  typographyConfig_.guideHalfWidth = static_cast<uint8_t>(clampIntSetting(
      preferences_.getUChar(kPrefTypographyGuideWidth, typographyConfig_.guideHalfWidth),
      kTypographyGuideWidthMin, kTypographyGuideWidthMax));
  typographyConfig_.guideGap = static_cast<uint8_t>(clampIntSetting(
      preferences_.getUChar(kPrefTypographyGuideGap, typographyConfig_.guideGap),
      kTypographyGuideGapMin, kTypographyGuideGapMax));
  darkMode_ = preferences_.getBool(kPrefDarkMode, darkMode_);
  nightMode_ = preferences_.getBool(kPrefNightMode, nightMode_);
  display_.setFocusColorIndex(preferences_.getUChar(kPrefFocusColorIndex, 0));

  reader_.setWpm(preferences_.getUShort(kPrefWpm, reader_.wpm()));
  applyReaderUiOrientation();
  applyDisplayPreferences(nowMs, false);
  applyTypographySettings(nowMs, false);
  applyPacingSettings();
  if (rerender) {
    renderActiveReader(nowMs);
  }
}

void App::applyTypographySettings(uint32_t nowMs, bool rerender) {
  display_.setTypographyConfig(effectiveTypographyConfig());

  Serial.printf("[typography] face=%s highlight=%s track=%d anchor=%u guideWidth=%u guideGap=%u\n",
                readerTypefaceLabel().c_str(),
                focusHighlightLabel().c_str(),
                static_cast<int>(typographyConfig_.trackingPx),
                static_cast<unsigned int>(effectiveAnchorPercent()),
                static_cast<unsigned int>(typographyConfig_.guideHalfWidth),
                static_cast<unsigned int>(typographyConfig_.guideGap));

  if (!rerender) {
    return;
  }

  if (state_ == AppState::Menu) {
    renderMenu();
    return;
  }

  if (state_ == AppState::Paused || state_ == AppState::Playing) {
    renderActiveReader(nowMs);
  }
}

void App::cycleBrightness() {
  brightnessLevelIndex_ = static_cast<uint8_t>((brightnessLevelIndex_ + 1) % kBrightnessLevelCount);
  preferences_.putUChar(kPrefBrightness, brightnessLevelIndex_);
  const uint8_t percent = currentBrightnessPercent();
  Serial.printf("[display] brightness level %u/%u (%u%%)\n",
                static_cast<unsigned int>(brightnessLevelIndex_ + 1),
                static_cast<unsigned int>(kBrightnessLevelCount),
                static_cast<unsigned int>(percent));
  applyDisplayPreferences(millis());
}

void App::cycleThemeMode(uint32_t nowMs) {
  if (nightMode_) {
    nightMode_ = false;
    darkMode_ = true;
  } else if (darkMode_) {
    darkMode_ = false;
  } else {
    darkMode_ = true;
    nightMode_ = true;
  }

  preferences_.putBool(kPrefDarkMode, darkMode_);
  preferences_.putBool(kPrefNightMode, nightMode_);
  Serial.printf("[display] theme=%s\n", themeModeLabel().c_str());
  applyDisplayPreferences(nowMs);
}

void App::cycleUiLanguage(uint32_t nowMs) {
  uiLanguage_ = Localization::nextLanguage(uiLanguage_);
  preferences_.putUChar(kPrefUiLanguage, static_cast<uint8_t>(uiLanguage_));
  Serial.printf("[display] language=%s\n", uiLanguageLabel().c_str());

  if (state_ == AppState::Menu) {
    if (isSettingsListScreen()) {
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    }
    renderMenu();
    return;
  }

  if (state_ == AppState::Paused || state_ == AppState::Playing) {
    renderActiveReader(nowMs);
  }
}

void App::cycleReaderMode(uint32_t nowMs) {
  readerMode_ = nextReaderMode(readerMode_);
  preferences_.putUChar(kPrefReaderMode, static_cast<uint8_t>(readerMode_));
  Serial.printf("[display] reader mode=%s\n", readerModeLabel().c_str());
  invalidateContextPreviewWindow();

  if (state_ == AppState::Menu) {
    rebuildSettingsMenuItems();
    renderSettings();
    return;
  }

  renderActiveReader(nowMs);
}

void App::cycleHandednessMode(uint32_t nowMs) {
  handednessMode_ = nextHandednessMode(handednessMode_);
  preferences_.putUChar(kPrefHandedness, static_cast<uint8_t>(handednessMode_));
  Serial.printf("[display] handedness=%s rotation180=%u\n", handednessLabel().c_str(),
                uiRotated180() ? 1U : 0U);
  applyHandednessSettings(nowMs);
}

void App::togglePhantomWords(uint32_t nowMs) {
  phantomWordsEnabled_ = !phantomWordsEnabled_;
  preferences_.putBool(kPrefPhantomWords, phantomWordsEnabled_);
  Serial.printf("[display] phantom words=%s\n", phantomWordsLabel().c_str());
  applyDisplayPreferences(nowMs);
}

void App::cycleReaderFontSize(uint32_t nowMs) {
  readerFontSizeIndex_ = static_cast<uint8_t>((readerFontSizeIndex_ + 1) % kReaderFontSizeCount);
  preferences_.putUChar(kPrefReaderFontSize, readerFontSizeIndex_);
  Serial.printf("[display] font size=%s\n", readerFontSizeLabel().c_str());
  applyDisplayPreferences(nowMs);
}

bool App::updateBatteryStatus(uint32_t nowMs, bool force) {
  if (!force) {
    const bool lowBatteryKnown =
        batteryPresent_ && batterySampleInitialized_ &&
        (batteryFilteredVoltage_ <= kBatteryLowWarningVoltage ||
         batteryDisplayedPercent_ <= kBatteryLowWarningPercent);
    uint32_t sampleIntervalMs =
        lowBatteryKnown ? kBatteryLowSampleIntervalMs : kBatterySampleIntervalMs;
    if (state_ == AppState::Playing && !lowBatteryKnown) {
      sampleIntervalMs = kBatteryPlayingSampleIntervalMs;
    }
    if (nowMs - lastBatterySampleMs_ < sampleIntervalMs) {
      return false;
    }
  }

  lastBatterySampleMs_ = nowMs;

  BoardConfig::BatteryStatus status;
  if (BoardConfig::readBatteryStatus(status)) {
    batteryPresent_ = true;
    if (!batterySampleInitialized_) {
      batteryFilteredVoltage_ = status.voltage;
      batteryFilteredPercent_ = status.percent;
      batteryDisplayedPercent_ = status.percent;
      batteryRuntimeAnchorPercent_ = status.percent;
      batteryRuntimeAnchorMs_ = nowMs;
      batterySampleInitialized_ = true;
    } else {
      batteryFilteredVoltage_ = (batteryFilteredVoltage_ * 0.72f) + (status.voltage * 0.28f);
      batteryFilteredPercent_ = (batteryFilteredPercent_ * 0.72f) + (status.percent * 0.28f);

      const int filteredPercent =
          std::max(0, std::min(100, static_cast<int>(batteryFilteredPercent_ + 0.5f)));
      const int delta = filteredPercent - static_cast<int>(batteryDisplayedPercent_);
      if (force || abs(delta) >= kBatteryDisplayHysteresisPercent ||
          filteredPercent <= 10 || filteredPercent >= 99) {
        batteryDisplayedPercent_ = static_cast<uint8_t>(filteredPercent);
      }

      if (batteryDisplayedPercent_ > batteryRuntimeAnchorPercent_) {
        batteryRuntimeAnchorPercent_ = batteryDisplayedPercent_;
        batteryRuntimeAnchorMs_ = nowMs;
        batteryRuntimeEstimateReady_ = false;
      } else {
        const uint8_t percentDrop = batteryRuntimeAnchorPercent_ - batteryDisplayedPercent_;
        const uint32_t elapsedMs = nowMs - batteryRuntimeAnchorMs_;
        if (percentDrop >= kBatteryRuntimeMinDropPercent &&
            elapsedMs >= kBatteryRuntimeMinElapsedMs) {
          const float minutesPerPercent =
              (static_cast<float>(elapsedMs) / 60000.0f) / static_cast<float>(percentDrop);
          batteryRuntimeMinutesRemaining_ =
              static_cast<uint32_t>(batteryDisplayedPercent_ * minutesPerPercent + 0.5f);
          batteryRuntimeEstimateReady_ = true;
        }
      }
    }
  } else {
    batteryPresent_ = false;
    batteryCriticalSampleCount_ = 0;
  }

  handleBatteryProtection(nowMs);
  if (powerOffStarted_) {
    return false;
  }

  const String nextLabel = currentBatteryLabel();
  if (nextLabel == batteryLabel_) {
    return false;
  }

  batteryLabel_ = nextLabel;
  display_.setBatteryLabel(batteryLabel_);
  if (!batteryLabel_.isEmpty()) {
    Serial.printf("[power] battery %.2f V raw=%u%% shown=%u%% label=%s\n", status.voltage,
                  static_cast<unsigned int>(status.percent),
                  static_cast<unsigned int>(batteryDisplayedPercent_), batteryLabel_.c_str());
  } else {
    Serial.println("[power] battery not detected");
  }
  return true;
}

void App::handleBatteryProtection(uint32_t nowMs) {
  if (!batteryPresent_ || !batterySampleInitialized_) {
    batteryCriticalSampleCount_ = 0;
    return;
  }

  const bool critical = batteryFilteredVoltage_ <= kBatteryCriticalVoltage ||
                        batteryDisplayedPercent_ <= kBatteryCriticalPercent;
  if (critical) {
    if (batteryCriticalSampleCount_ < 255) {
      ++batteryCriticalSampleCount_;
    }
  } else {
    batteryCriticalSampleCount_ = 0;
  }

  if (batteryCriticalSampleCount_ >= kBatteryCriticalConsecutiveSamples) {
    const String line2 =
        batteryVoltageLabel() + " " + String(static_cast<unsigned int>(batteryDisplayedPercent_)) +
        "%";
    Serial.printf("[power] critical battery %.2f V %u%%; powering off\n",
                  static_cast<double>(batteryFilteredVoltage_),
                  static_cast<unsigned int>(batteryDisplayedPercent_));
    display_.renderStatus(tr(TrKey::LowBattery),
                          tr(TrKey::PoweringOff), line2);
    delay(kBatteryShutdownNoticeMs);
    enterPowerOff(millis());
    return;
  }

  const bool low = batteryFilteredVoltage_ <= kBatteryLowWarningVoltage ||
                   batteryDisplayedPercent_ <= kBatteryLowWarningPercent;
  if (!low) {
    return;
  }

  if (lastLowBatteryWarningMs_ == 0 ||
      nowMs - lastLowBatteryWarningMs_ >= kBatteryLowWarningRepeatMs) {
    showLowBatteryWarning(nowMs);
  }
}

void App::showLowBatteryWarning(uint32_t nowMs) {
  lastLowBatteryWarningMs_ = nowMs;
  batteryWarningOverlayVisible_ = true;
  batteryWarningRestoreAtMs_ = nowMs + kBatteryWarningVisibleMs;
  touchPlayHeld_ = false;
  playLocked_ = false;
  pauseAtSentenceEndRequested_ = false;
  wpmFeedbackVisible_ = false;
  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;

  if (state_ == AppState::Playing) {
    setState(AppState::Paused, nowMs);
  }

  const String line1 =
      String(static_cast<unsigned int>(batteryDisplayedPercent_)) +
      tr(TrKey::Remaining);
  display_.renderStatus(tr(TrKey::LowBattery), line1,
                        batteryVoltageLabel() +
                            tr(TrKey::ChargeSoon));
  Serial.printf("[power] low battery warning %.2f V %u%%\n",
                static_cast<double>(batteryFilteredVoltage_),
                static_cast<unsigned int>(batteryDisplayedPercent_));
}

void App::updateBatteryWarningOverlay(uint32_t nowMs) {
  if (!batteryWarningOverlayVisible_ || nowMs < batteryWarningRestoreAtMs_) {
    return;
  }

  batteryWarningOverlayVisible_ = false;
  if (state_ == AppState::Paused || state_ == AppState::Playing) {
    renderActiveReader(nowMs);
  } else if (state_ == AppState::Menu) {
    renderMenu();
  } else if (state_ == AppState::Standby) {
    updateStandbyScreensaver(nowMs, true);
  }
}

void App::updateWpmFeedback(uint32_t nowMs) {
  if (!wpmFeedbackVisible_ || state_ != AppState::Paused) {
    return;
  }

  if (nowMs < wpmFeedbackUntilMs_) {
    return;
  }

  wpmFeedbackVisible_ = false;
  renderActiveReader(nowMs);
}

void App::resetReaderTapTracking() { lastReaderTapValid_ = false; }

bool App::isFooterMetricTap(uint16_t x, uint16_t y) const {
  return x >= BoardConfig::DISPLAY_WIDTH - kFooterMetricTapWidthPx &&
         y >= BoardConfig::DISPLAY_HEIGHT - kFooterMetricTapHeightPx;
}

bool App::isBatteryBadgeTap(uint16_t x, uint16_t y) const {
  return x >= BoardConfig::DISPLAY_WIDTH - kBatteryBadgeTapWidthPx &&
         y <= kBatteryBadgeTapHeightPx;
}

bool App::isPreviousSentenceTap(uint16_t x, uint16_t y) const {
  // Left portion of top area only (not overlapping with SP button)
  return x < 38 && y < 30;
}

bool App::isSavePointButtonTap(uint16_t x, uint16_t y) const {
  // Right of "<<", top area: matches "SP" text at (52, 8)
  return savePointButtonVisible_ && x >= 38 && x < 120 && y < 30;
}

bool App::isActivelyReading() const { return state_ == AppState::Playing; }

DisplayManager::ReaderChrome App::readerChrome() const {
  DisplayManager::ReaderChrome chrome;
  const bool reading = isActivelyReading();
  chrome.showBattery = !reading || readerBatteryVisibleWhilePlaying_;
  chrome.showChapter = !reading || readerChapterVisibleWhilePlaying_;
  chrome.showProgress = !reading || readerProgressVisibleWhilePlaying_;
  chrome.showPreviousSentenceHint = !contextViewVisible_ || scrollModeEnabled();
  chrome.showSavePointButton = savePointButtonVisible_;
  return chrome;
}

bool App::readerFooterVisible() const {
  const DisplayManager::ReaderChrome chrome = readerChrome();
  return chrome.showChapter || chrome.showProgress;
}

String App::readerFooterStatusLabel() const {
  if (isActivelyReading()) {
    return String(readingProgressPercent()) + "%";
  }

  return currentFooterMetricLabel();
}

String App::onOffLabel(bool enabled) const { return enabled ? uiText(UiText::On) : uiText(UiText::Off); }

bool App::handlePreviousSentenceTap(uint16_t x, uint16_t y, uint32_t nowMs) {
  if (!isPreviousSentenceTap(x, y)) {
    return false;
  }

  resetReaderTapTracking();
  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  wpmFeedbackVisible_ = false;
  contextViewVisible_ = false;
  reader_.rewindSentence();

  if (state_ == AppState::Playing) {
    setState(AppState::Paused, nowMs);
  } else {
    renderActiveReader(nowMs);
    saveReadingPosition(true);
  }

  Serial.printf("[app] sentence rewind index=%u word=%s\n",
                static_cast<unsigned int>(reader_.currentIndex()), reader_.currentWord().c_str());
  return true;
}

bool App::handleFooterMetricTap(uint16_t x, uint16_t y, uint32_t nowMs) {
  if (isActivelyReading() || !readerFooterVisible() || !isFooterMetricTap(x, y)) {
    return false;
  }

  switch (footerMetricMode_) {
    case FooterMetricMode::Percentage:
      footerMetricMode_ = FooterMetricMode::ChapterTime;
      break;
    case FooterMetricMode::ChapterTime:
      footerMetricMode_ = FooterMetricMode::BookTime;
      break;
    case FooterMetricMode::BookTime:
    default:
      footerMetricMode_ = FooterMetricMode::Percentage;
      break;
  }

  preferences_.putUChar(kPrefFooterMetricMode, static_cast<uint8_t>(footerMetricMode_));
  resetReaderTapTracking();
  renderActiveReader(nowMs);
  const char *modeName = "percent";
  switch (footerMetricMode_) {
    case FooterMetricMode::ChapterTime:
      modeName = "chapter";
      break;
    case FooterMetricMode::BookTime:
      modeName = "book";
      break;
    case FooterMetricMode::Percentage:
    default:
      modeName = "percent";
      break;
  }
  Serial.printf("[reader] footer metric=%s\n", modeName);
  return true;
}

bool App::handleBatteryBadgeTap(uint16_t x, uint16_t y, uint32_t nowMs) {
  if (batteryLabel_.isEmpty() || !readerChrome().showBattery || !isBatteryBadgeTap(x, y)) {
    return false;
  }

  switch (batteryLabelMode_) {
    case BatteryLabelMode::Percent:
      batteryLabelMode_ = BatteryLabelMode::TimeRemaining;
      break;
    case BatteryLabelMode::TimeRemaining:
      batteryLabelMode_ = BatteryLabelMode::Voltage;
      break;
    case BatteryLabelMode::Voltage:
    default:
      batteryLabelMode_ = BatteryLabelMode::Percent;
      break;
  }
  preferences_.putUChar(kPrefBatteryLabelMode, static_cast<uint8_t>(batteryLabelMode_));
  batteryLabel_ = currentBatteryLabel();
  display_.setBatteryLabel(batteryLabel_);
  resetReaderTapTracking();
  renderActiveReader(nowMs);
  const char *modeName = "percent";
  if (batteryLabelMode_ == BatteryLabelMode::TimeRemaining) {
    modeName = "time";
  } else if (batteryLabelMode_ == BatteryLabelMode::Voltage) {
    modeName = "voltage";
  }
  Serial.printf("[power] battery label mode=%s label=%s\n", modeName, batteryLabel_.c_str());
  return true;
}

void App::handleReaderTap(uint16_t x, uint16_t y, uint32_t nowMs) {
  const bool recentTap =
      lastReaderTapValid_ && nowMs - lastReaderTapMs_ <= kReaderDoubleTapWindowMs;
  const bool sameRegion =
      recentTap &&
      abs(static_cast<int>(x) - static_cast<int>(lastReaderTapX_)) <=
          static_cast<int>(kReaderDoubleTapSlopPx) &&
      abs(static_cast<int>(y) - static_cast<int>(lastReaderTapY_)) <=
          static_cast<int>(kReaderDoubleTapSlopPx);

  if (sameRegion) {
    resetReaderTapTracking();

    if (state_ == AppState::Playing) {
      requestReaderPauseAtSentenceEnd(nowMs);
    } else if (state_ == AppState::Paused) {
      playLocked_ = true;
      pauseAtSentenceEndRequested_ = false;
      wpmFeedbackVisible_ = false;
      setState(AppState::Playing, nowMs);
    }
    Serial.printf("[touch] reader double tap state=%s\n", stateName(state_));
    return;
  }

  if (recentTap) {
    Serial.printf("[touch] double tap miss dx=%d dy=%d dt=%lu\n",
                  static_cast<int>(x) - static_cast<int>(lastReaderTapX_),
                  static_cast<int>(y) - static_cast<int>(lastReaderTapY_),
                  static_cast<unsigned long>(nowMs - lastReaderTapMs_));
  }

  lastReaderTapValid_ = true;
  lastReaderTapMs_ = nowMs;
  lastReaderTapX_ = x;
  lastReaderTapY_ = y;
}

void App::requestReaderPauseAtSentenceEnd(uint32_t nowMs) {
  if (state_ != AppState::Playing) {
    return;
  }

  playLocked_ = false;
  touchPlayHeld_ = false;
  if (pauseMode_ == PauseMode::Instant) {
    pauseAtSentenceEndRequested_ = false;
    setState(AppState::Paused, nowMs);
    return;
  }

  if (!pauseAtSentenceEndRequested_) {
    pauseAtSentenceEndRequested_ = true;
    Serial.println("[app] pause requested at sentence end");
  }

  if (shouldFinalizeReaderPause(nowMs)) {
    finalizeReaderPause(nowMs);
  }
}

bool App::shouldFinalizeReaderPause(uint32_t nowMs) const {
  if (state_ != AppState::Playing || !pauseAtSentenceEndRequested_) {
    return false;
  }

  const uint32_t durationMs = reader_.currentWordDurationMs();
  if (durationMs == 0 || reader_.elapsedInCurrentWordMs(nowMs) < durationMs) {
    return false;
  }

  return reader_.currentWordEndsSentence() || reader_.atEnd();
}

void App::finalizeReaderPause(uint32_t nowMs) {
  pauseAtSentenceEndRequested_ = false;
  playLocked_ = false;
  touchPlayHeld_ = false;
  setState(AppState::Paused, nowMs);
}

void App::handleTouch(uint32_t nowMs) {
  if (!touchInitialized_) {
    return;
  }

  if (state_ == AppState::Booting || state_ == AppState::UsbTransfer ||
      state_ == AppState::Sleeping) {
    touch_.cancel();
    pausedTouch_.active = false;
    pausedTouchIntent_ = TouchIntent::None;
    touchPlayHeld_ = false;
    resetReaderTapTracking();
    return;
  }

  if (state_ == AppState::Standby) {
    TouchEvent ev;
    if (touch_.poll(ev)) {
      // Ignore touch during standby — only physical buttons wake the device
      // to prevent accidental wake in pocket
    }
    return;
  }

  TouchEvent ev;
  if (!touch_.poll(ev)) {
    return;
  }

  lastActivityMs_ = nowMs;

  Serial.printf("[touch] phase=%s touched=%u x=%u y=%u gesture=%u state=%s\n",
                touchPhaseName(ev.phase), ev.touched ? 1 : 0, ev.x, ev.y, ev.gesture,
                stateName(state_));
  if (state_ == AppState::Menu) {
    if (menuScreen_ == MenuScreen::FocusTimerSession) {
      applyFocusTimerTouch(ev, nowMs);
    } else {
      applyMenuTouchGesture(ev, nowMs);
    }
  } else {
    applyPausedTouchGesture(ev, nowMs);
  }
}

void App::applyPausedTouchGesture(const TouchEvent &event, uint32_t nowMs) {
  if (event.phase == TouchPhase::End && touchPlayHeld_) {
    resetReaderTapTracking();
    pausedTouch_.active = false;
    pausedTouchIntent_ = TouchIntent::None;
    touchPlayHeld_ = false;
    // Auto-save position when releasing hold-to-read
    saveReadingPosition(true);
    requestReaderPauseAtSentenceEnd(nowMs);
    return;
  }

  if (event.phase == TouchPhase::Start) {
    pausedTouch_.active = true;
    pausedTouchIntent_ = TouchIntent::None;
    if (state_ != AppState::Playing) {
      invalidateContextPreviewWindow();
    }
    pausedTouch_.startX = event.x;
    pausedTouch_.startY = event.y;
    pausedTouch_.lastX = event.x;
    pausedTouch_.lastY = event.y;
    pausedTouch_.startMs = nowMs;
    pausedTouch_.lastMs = nowMs;
    pausedTouch_.startWordIndex = reader_.currentIndex();
    pausedTouch_.gestureStepsApplied = 0;
    pausedTouch_.browseOffsetPermille = 0;
    return;
  }

  if (!pausedTouch_.active) {
    return;
  }

  const uint32_t elapsedSinceLastEventMs = nowMs - pausedTouch_.lastMs;
  pausedTouch_.lastX = event.x;
  pausedTouch_.lastY = event.y;
  pausedTouch_.lastMs = nowMs;

  const int deltaX = static_cast<int>(pausedTouch_.lastX) - static_cast<int>(pausedTouch_.startX);
  const int deltaY = static_cast<int>(pausedTouch_.lastY) - static_cast<int>(pausedTouch_.startY);
  const int absDeltaX = abs(deltaX);
  const int absDeltaY = abs(deltaY);
  const uint32_t pressDurationMs = nowMs - pausedTouch_.startMs;
  const bool ended = event.phase == TouchPhase::End;
  const bool tapLike = absDeltaX <= static_cast<int>(kTapSlopPx) &&
                       absDeltaY <= static_cast<int>(kTapSlopPx);
  const bool previewBrowseMode = contextViewVisible_ && !scrollModeEnabled();

  if (state_ == AppState::Playing) {
    if (ended) {
      pausedTouch_.active = false;
      pausedTouchIntent_ = TouchIntent::None;
      if (tapLike) {
        if (handleBatteryBadgeTap(event.x, event.y, nowMs)) {
          return;
        }
        if (handleFooterMetricTap(event.x, event.y, nowMs)) {
          return;
        }
        if (handlePreviousSentenceTap(event.x, event.y, nowMs)) {
          return;
        }
        if (isSavePointButtonTap(event.x, event.y)) {
          resetReaderTapTracking();
          // Pause reading and open name entry for save point
          if (state_ == AppState::Playing) {
            setState(AppState::Paused, nowMs);
          }
          saveReadingPosition(true);
          const String chapter = currentChapterLabel();
          String defaultName;
          if (chapter.isEmpty() || chapter == currentBookTitle_) {
            defaultName = currentBookTitle_.substring(0, 16) + " " +
                          String(static_cast<unsigned int>(readingProgressPercent())) + "%";
          } else {
            defaultName = chapter.substring(0, 20) + " " +
                          String(static_cast<unsigned int>(readingProgressPercent())) + "%";
          }
          menuScreen_ = MenuScreen::Main;
          setState(AppState::Menu, nowMs);
          openTextEntry(TextEntryPurpose::SavePointName,
                        polish("Nazwij zakladke", "Name bookmark"),
                        polish("Wpisz nazwe:", "Enter name:"),
                        "", defaultName, "", false, 30,
                        MenuScreen::SavePointsList);
          return;
        }
        if (playLocked_ || pauseAtSentenceEndRequested_) {
          resetReaderTapTracking();
          requestReaderPauseAtSentenceEnd(nowMs);
        } else {
          handleReaderTap(event.x, event.y, nowMs);
        }
      } else {
        resetReaderTapTracking();
      }
    }
    return;
  }

  if (!previewBrowseMode && !ended && pausedTouchIntent_ == TouchIntent::None &&
      pressDurationMs >= kTouchPlayHoldMs && tapLike) {
    resetReaderTapTracking();
    touchPlayHeld_ = true;
    pausedTouchIntent_ = TouchIntent::PlayHold;
    wpmFeedbackVisible_ = false;
    setState(AppState::Playing, nowMs);
    return;
  }

  if (pausedTouchIntent_ == TouchIntent::None) {
    if (absDeltaX >= static_cast<int>(kSwipeThresholdPx) &&
        absDeltaX > absDeltaY + static_cast<int>(kAxisBiasPx)) {
      resetReaderTapTracking();
      pausedTouchIntent_ = TouchIntent::Scrub;
    } else if (previewBrowseMode && !ended && pressDurationMs >= kPreviewBrowseHoldMs &&
               absDeltaY > absDeltaX + static_cast<int>(kAxisBiasPx)) {
      resetReaderTapTracking();
      pausedTouchIntent_ = TouchIntent::BrowseScroll;
    } else if (!previewBrowseMode && absDeltaY >= static_cast<int>(kSwipeThresholdPx) &&
               absDeltaY > absDeltaX + static_cast<int>(kAxisBiasPx)) {
      resetReaderTapTracking();
      pausedTouchIntent_ = TouchIntent::Wpm;
    }
  }

  if (pausedTouchIntent_ == TouchIntent::Scrub) {
    applyScrubTarget(scrubStepsForDrag(deltaX), nowMs);
    if (ended) {
      pausedTouch_.active = false;
      pausedTouchIntent_ = TouchIntent::None;
      saveReadingPosition(true);
    }
    return;
  }

  if (pausedTouchIntent_ == TouchIntent::BrowseScroll) {
    applyBrowseHoldScroll(event.y, elapsedSinceLastEventMs, nowMs);
    if (ended) {
      pausedTouch_.active = false;
      pausedTouchIntent_ = TouchIntent::None;
      saveReadingPosition(true);
    }
    return;
  }

  if (pausedTouchIntent_ == TouchIntent::Wpm) {
    if (!ended) {
      return;
    }

    const int wpmDelta = (deltaY < 0) ? 1 : -1;
    reader_.adjustWpm(wpmDelta);
    preferences_.putUShort(kPrefWpm, reader_.wpm());
    renderWpmFeedback(nowMs);
    Serial.printf("[app] WPM=%u interval=%lu ms\n", reader_.wpm(),
                  static_cast<unsigned long>(reader_.wordIntervalMs()));
    pausedTouch_.active = false;
    pausedTouchIntent_ = TouchIntent::None;
    return;
  }

  if (ended) {
    pausedTouch_.active = false;
    pausedTouchIntent_ = TouchIntent::None;
    if (tapLike && handleBatteryBadgeTap(event.x, event.y, nowMs)) {
      return;
    }
    if (tapLike && handleFooterMetricTap(event.x, event.y, nowMs)) {
      return;
    }
    if (tapLike && handlePreviousSentenceTap(event.x, event.y, nowMs)) {
      return;
    }
    if (tapLike && isSavePointButtonTap(event.x, event.y)) {
      resetReaderTapTracking();
      saveReadingPosition(true);
      const String chapter = currentChapterLabel();
      String defaultName;
      if (chapter.isEmpty() || chapter == currentBookTitle_) {
        defaultName = currentBookTitle_.substring(0, 16) + " " +
                      String(static_cast<unsigned int>(readingProgressPercent())) + "%";
      } else {
        defaultName = chapter.substring(0, 20) + " " +
                      String(static_cast<unsigned int>(readingProgressPercent())) + "%";
      }
      menuScreen_ = MenuScreen::Main;
      setState(AppState::Menu, nowMs);
      openTextEntry(TextEntryPurpose::SavePointName,
                    polish("Nazwij zakladke", "Name bookmark"),
                    polish("Wpisz nazwe:", "Enter name:"),
                    "", defaultName, "", false, 30,
                    MenuScreen::SavePointsList);
      return;
    }
    if (tapLike && previewBrowseMode) {
      resetReaderTapTracking();
      contextViewVisible_ = false;
      renderActiveReader(nowMs);
    } else if (tapLike) {
      handleReaderTap(event.x, event.y, nowMs);
    } else {
      resetReaderTapTracking();
    }
  }
}

int App::scrubStepsForDrag(int deltaX) const {
  const int absDeltaX = abs(deltaX);
  if (absDeltaX < static_cast<int>(kSwipeThresholdPx)) {
    return 0;
  }

  int steps = 1 + ((absDeltaX - static_cast<int>(kSwipeThresholdPx)) /
                   static_cast<int>(kScrubStepPx));
  steps = std::min(steps, kMaxScrubStepsPerGesture);

  return (deltaX > 0) ? steps : -steps;
}

void App::applyScrubTarget(int targetSteps, uint32_t nowMs) {
  if (targetSteps == pausedTouch_.gestureStepsApplied) {
    return;
  }

  reader_.seekRelative(pausedTouch_.startWordIndex, targetSteps);
  pausedTouch_.gestureStepsApplied = targetSteps;
  if (!scrollModeEnabled()) {
    contextViewVisible_ = true;
  }
  wpmFeedbackVisible_ = false;
  renderActiveReader(nowMs);
  Serial.printf("[app] scrub target=%d word=%s\n", targetSteps, reader_.currentWord().c_str());
}

int App::browseScrollRatePermille(uint16_t y) const {
  const int centerY = BoardConfig::DISPLAY_HEIGHT / 2;
  const int signedDistance = static_cast<int>(y) - centerY;
  const int absDistance = abs(signedDistance);
  if (absDistance <= static_cast<int>(kBrowseNeutralZonePx)) {
    return 0;
  }

  const int activeRange = std::max(1, centerY - static_cast<int>(kBrowseNeutralZonePx));
  const int activeDistance =
      std::min(activeRange, absDistance - static_cast<int>(kBrowseNeutralZonePx));
  const uint32_t speedPermille =
      kBrowseMinWordsPerSecondPermille +
      ((kBrowseMaxWordsPerSecondPermille - kBrowseMinWordsPerSecondPermille) *
       static_cast<uint32_t>(activeDistance)) /
          static_cast<uint32_t>(activeRange);

  return signedDistance < 0 ? -static_cast<int>(speedPermille) : static_cast<int>(speedPermille);
}

void App::renderContextBrowsePreview(size_t currentIndex, uint16_t scrollProgressPermille) {
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0) {
    renderReaderWord();
    return;
  }

  if (currentIndex >= wordCount) {
    currentIndex = wordCount - 1;
    scrollProgressPermille = 0;
  }

  updateContextPreviewWindow(currentIndex);
  contextViewVisible_ = true;
  const DisplayManager::ReaderChrome chrome = readerChrome();
  display_.renderScrollView(contextPreviewWords_, currentReaderContentToken(),
                            contextPreviewStartIndex_, currentIndex, scrollProgressPermille,
                            currentChapterLabel(), readingProgressPercent(), "",
                            readerFooterStatusLabel(), chrome);
}

void App::applyBrowseHoldScroll(uint16_t y, uint32_t elapsedMs, uint32_t nowMs) {
  if (elapsedMs == 0) {
    return;
  }

  const int ratePermille = browseScrollRatePermille(y);
  pausedTouch_.browseOffsetPermille +=
      (static_cast<int32_t>(ratePermille) * static_cast<int32_t>(elapsedMs)) / 1000;

  int targetWords = pausedTouch_.browseOffsetPermille / 1000;
  int32_t remainderPermille = pausedTouch_.browseOffsetPermille % 1000;
  if (remainderPermille < 0) {
    remainderPermille += 1000;
    --targetWords;
  }

  reader_.seekRelative(pausedTouch_.startWordIndex, targetWords);
  if (!ensureCurrentBookWordAvailable(nowMs)) {
    return;
  }
  pausedTouch_.gestureStepsApplied = targetWords;
  contextViewVisible_ = true;
  wpmFeedbackVisible_ = false;
  renderContextBrowsePreview(reader_.currentIndex(),
                             static_cast<uint16_t>(remainderPermille));
  Serial.printf("[app] browse hold target=%d progress=%ld word=%s\n", targetWords,
                static_cast<long>(remainderPermille), reader_.currentWord().c_str());
}

void App::applyMenuTouchGesture(const TouchEvent &event, uint32_t nowMs) {
  if (event.phase == TouchPhase::Start) {
    pausedTouch_.active = true;
    pausedTouchIntent_ = TouchIntent::None;
    pausedTouch_.startX = event.x;
    pausedTouch_.startY = event.y;
    pausedTouch_.lastX = event.x;
    pausedTouch_.lastY = event.y;
    pausedTouch_.startMs = nowMs;
    pausedTouch_.lastMs = nowMs;
    return;
  }

  if (!pausedTouch_.active) {
    return;
  }

  pausedTouch_.lastX = event.x;
  pausedTouch_.lastY = event.y;
  pausedTouch_.lastMs = nowMs;

  // Help popup dismiss: any touch while popup is showing dismisses it
  if (showingHelpPopup_ && event.phase == TouchPhase::End) {
    pausedTouch_.active = false;
    dismissHelpPopup(nowMs);
    return;
  }



  if (event.phase != TouchPhase::End) {
    return;
  }

  pausedTouch_.active = false;

  const int deltaX = static_cast<int>(pausedTouch_.lastX) - static_cast<int>(pausedTouch_.startX);
  const int deltaY = static_cast<int>(pausedTouch_.lastY) - static_cast<int>(pausedTouch_.startY);
  const int absDeltaX = abs(deltaX);
  const int absDeltaY = abs(deltaY);

  if (menuScreen_ == MenuScreen::TextEntry) {
    if (absDeltaX <= static_cast<int>(kTapSlopPx) && absDeltaY <= static_cast<int>(kTapSlopPx)) {
      handleTextEntryTap(event.x, event.y, nowMs);
    }
    return;
  }

  if (menuScreen_ == MenuScreen::TypographyTuning &&
      absDeltaX >= static_cast<int>(kSwipeThresholdPx) &&
      absDeltaX > absDeltaY + static_cast<int>(kAxisBiasPx)) {
    cycleTypographyPreviewSample(deltaX < 0 ? 1 : -1);
    return;
  }

  if (absDeltaY >= static_cast<int>(kSwipeThresholdPx) &&
      absDeltaY > absDeltaX + static_cast<int>(kAxisBiasPx)) {
    moveMenuSelection(deltaY < 0 ? -1 : 1);
    return;
  }

  if (absDeltaX <= static_cast<int>(kTapSlopPx) && absDeltaY <= static_cast<int>(kTapSlopPx)) {
    // Top-left corner tap = Back button on any menu screen
    if (event.x < 80 && event.y < 35 && menuScreen_ != MenuScreen::Main) {
      // Simulate selecting "Back" (index 0) for list-based screens
      if (isSettingsListScreen() || menuScreen_ == MenuScreen::BookPicker ||
          menuScreen_ == MenuScreen::BookDetails || menuScreen_ == MenuScreen::ChapterPicker ||
          menuScreen_ == MenuScreen::SavePointsList || menuScreen_ == MenuScreen::PluginsList ||
          menuScreen_ == MenuScreen::FocusTimerGenres || menuScreen_ == MenuScreen::TypographyTuning) {
        // Set selection to Back and select it
        if (menuScreen_ == MenuScreen::Presets || menuScreen_ == MenuScreen::PresetsDeleteConfirm) {
          presetsSelectedIndex_ = 0;
        } else if (isSettingsListScreen()) {
          settingsSelectedIndex_ = 0;
        } else if (menuScreen_ == MenuScreen::BookPicker) {
          bookPickerSelectedIndex_ = 0;
        } else if (menuScreen_ == MenuScreen::BookDetails) {
          bookDetailsSelectedIndex_ = 0;
        } else if (menuScreen_ == MenuScreen::ChapterPicker) {
          chapterPickerSelectedIndex_ = 0;
        } else if (menuScreen_ == MenuScreen::SavePointsList) {
          savePointSelectedIndex_ = 0;
        } else if (menuScreen_ == MenuScreen::PluginsList) {
          pluginsSelectedIndex_ = 0;
        } else if (menuScreen_ == MenuScreen::FocusTimerGenres) {
          focusTimerGenreSelectedIndex_ = 0;
        } else if (menuScreen_ == MenuScreen::TypographyTuning) {
          typographyTuningSelectedIndex_ = TypographyTuningBack;
        }
        selectMenuItem(nowMs);
        return;
      }
      // For confirm screens, just go back to main
      menuScreen_ = MenuScreen::Main;
      renderMainMenu();
      return;
    }
    selectMenuItem(nowMs);
  }
}

void App::applyFocusTimerTouch(const TouchEvent &event, uint32_t nowMs) {
  if (event.phase == TouchPhase::Start) {
    pausedTouch_.active = true;
    pausedTouch_.startX = event.x;
    pausedTouch_.startY = event.y;
    pausedTouch_.lastX = event.x;
    pausedTouch_.lastY = event.y;
    pausedTouch_.startMs = nowMs;
    pausedTouch_.lastMs = nowMs;
    focusTimerCancelHoldTriggered_ = false;
    return;
  }

  if (!pausedTouch_.active) {
    return;
  }

  pausedTouch_.lastX = event.x;
  pausedTouch_.lastY = event.y;
  pausedTouch_.lastMs = nowMs;

  const int deltaX = static_cast<int>(pausedTouch_.lastX) - static_cast<int>(pausedTouch_.startX);
  const int deltaY = static_cast<int>(pausedTouch_.lastY) - static_cast<int>(pausedTouch_.startY);
  const int absDeltaX = abs(deltaX);
  const int absDeltaY = abs(deltaY);
  const bool tapLike = absDeltaX <= static_cast<int>(kTapSlopPx) &&
                       absDeltaY <= static_cast<int>(kTapSlopPx);

  if (focusTimer_.isActiveTimerRunning() && !focusTimerCancelHoldTriggered_ &&
      event.phase != TouchPhase::End &&
      absDeltaX <= static_cast<int>(kFocusTimerCancelHoldMaxDriftPx) &&
      absDeltaY <= static_cast<int>(kFocusTimerCancelHoldMaxDriftPx) &&
      nowMs - pausedTouch_.startMs >= kFocusTimerCancelHoldMs) {
    focusTimer_.cancelActiveTimer(nowMs);
    pausedTouch_.active = false;
    focusTimerCancelHoldTriggered_ = true;
    renderFocusTimerSession();
    return;
  }

  if (event.phase != TouchPhase::End) {
    return;
  }

  pausedTouch_.active = false;

  if (focusTimerCancelHoldTriggered_) {
    focusTimerCancelHoldTriggered_ = false;
    return;
  }

  // Top-left tap = back/exit from timer
  if (tapLike && event.x < 80 && event.y < 35) {
    resetFocusTimer();
    menuScreen_ = MenuScreen::PluginsList;
    openPluginsList();
    return;
  }

  (void)tapLike;
}

void App::openFocusTimer() {
  focusTimer_.open();
  rebuildFocusTimerGenreMenuItems();
  focusTimerGenreSelectedIndex_ =
      focusTimerGenreMenuItems_.size() > 1 ? kFocusTimerGenreFirstIndex : kFocusTimerGenreBackIndex;
  focusTimerCancelHoldTriggered_ = false;
  menuScreen_ = (focusTimer_.state() == FocusTimer::State::GenreSelect)
                    ? MenuScreen::FocusTimerGenres
                    : MenuScreen::FocusTimerSession;
  renderMenu();
}

void App::updateFocusTimer(uint32_t nowMs) {
  if (state_ != AppState::Menu || menuScreen_ != MenuScreen::FocusTimerSession) {
    return;
  }

  focusTimer_.update(nowMs);
  if (focusTimer_.consumeCompletionCue()) {
    playFocusTimerCompletionCue();
  }
  if (focusTimer_.state() == FocusTimer::State::GenreSelect) {
    menuScreen_ = MenuScreen::FocusTimerGenres;
    rebuildFocusTimerGenreMenuItems();
    renderFocusTimerGenres();
    return;
  }

  renderFocusTimerSession();
}

void App::resetFocusTimer() {
  focusTimer_.abandon();
  focusTimerCancelHoldTriggered_ = false;
  pausedTouch_.active = false;
  focusTimerGenreSelectedIndex_ = kFocusTimerGenreBackIndex;
}

void App::rebuildFocusTimerGenreMenuItems() {
  focusTimerGenreMenuItems_.clear();
  focusTimerGenreMenuItems_.push_back(uiText(UiText::Back));
  focusTimerGenreMenuItems_.push_back("Chores - 15 min");
  focusTimerGenreMenuItems_.push_back("Work - 25 min");
  focusTimerGenreMenuItems_.push_back("Fitness - 30 min");
  focusTimerGenreMenuItems_.push_back("Self Care - 20 min");
  focusTimerGenreMenuItems_.push_back("Other - 10 min");

  if (focusTimerGenreSelectedIndex_ >= focusTimerGenreMenuItems_.size()) {
    focusTimerGenreSelectedIndex_ =
        focusTimerGenreMenuItems_.size() > 1 ? kFocusTimerGenreFirstIndex : kFocusTimerGenreBackIndex;
  }
}

void App::selectFocusTimerGenre(uint32_t nowMs) {
  if (focusTimerGenreMenuItems_.empty()) {
    rebuildFocusTimerGenreMenuItems();
  }

  if (focusTimerGenreSelectedIndex_ == kFocusTimerGenreBackIndex) {
    resetFocusTimer();
    menuScreen_ = MenuScreen::Main;
    menuSelectedIndex_ = MenuPlugins;
    renderMainMenu();
    return;
  }

  FocusTimer::Genre genre = FocusTimer::Genre::None;
  switch (focusTimerGenreSelectedIndex_) {
    case 1:
      genre = FocusTimer::Genre::Chores;
      break;
    case 2:
      genre = FocusTimer::Genre::RsvpNano;
      break;
    case 3:
      genre = FocusTimer::Genre::StrengthLabs;
      break;
    case 4:
      genre = FocusTimer::Genre::SelfCare;
      break;
    case 5:
      genre = FocusTimer::Genre::Other;
      break;
    default:
      break;
  }

  if (genre == FocusTimer::Genre::None) {
    return;
  }

  focusTimer_.chooseGenre(genre, nowMs);
  focusTimerCancelHoldTriggered_ = false;
  menuScreen_ = MenuScreen::FocusTimerSession;
  renderFocusTimerSession();
}

void App::moveMenuSelection(int direction) {
  if (direction == 0 || menuScreen_ == MenuScreen::TextEntry) {
    return;
  }

  size_t *selectedIndex = &menuSelectedIndex_;
  size_t itemCount = MenuItemCount;
  if (menuScreen_ == MenuScreen::Presets || menuScreen_ == MenuScreen::PresetsDeleteConfirm) {
    selectedIndex = &presetsSelectedIndex_;
    itemCount = settingsMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::SettingsHome || menuScreen_ == MenuScreen::SettingsDisplay ||
      menuScreen_ == MenuScreen::SettingsPacing || menuScreen_ == MenuScreen::WifiSettings ||
      menuScreen_ == MenuScreen::SettingsConnectivity ||
      menuScreen_ == MenuScreen::SettingsAbout || menuScreen_ == MenuScreen::ScreensaverSettings ||
      menuScreen_ == MenuScreen::WelcomeLanguage || menuScreen_ == MenuScreen::WelcomeTheme || menuScreen_ == MenuScreen::WelcomeHighlightColor || menuScreen_ == MenuScreen::WelcomePacing || menuScreen_ == MenuScreen::WelcomeConnect) {
    selectedIndex = &settingsSelectedIndex_;
    itemCount = settingsMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::WifiNetworks) {
    selectedIndex = &wifiNetworkSelectedIndex_;
    itemCount = wifiNetworkMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::TypographyTuning) {
    selectedIndex = &typographyTuningSelectedIndex_;
    itemCount = TypographyTuningItemCount;
  } else if (menuScreen_ == MenuScreen::BookPicker) {
    selectedIndex = &bookPickerSelectedIndex_;
    itemCount = bookMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::BookDetails) {
    selectedIndex = &bookDetailsSelectedIndex_;
    itemCount = bookDetailsMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::ChapterPicker) {
    selectedIndex = &chapterPickerSelectedIndex_;
    itemCount = chapterMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::SavePointsList) {
    selectedIndex = &savePointSelectedIndex_;
    itemCount = savePointMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::PluginsList) {
    selectedIndex = &pluginsSelectedIndex_;
    itemCount = pluginsMenuItems_.size();
  } else if (menuScreen_ == MenuScreen::RestartConfirm) {
    selectedIndex = &restartConfirmSelectedIndex_;
    itemCount = RestartConfirmItemCount;
  } else if (menuScreen_ == MenuScreen::SdCardRepairConfirm) {
    selectedIndex = &sdCardRepairConfirmSelectedIndex_;
    itemCount = SdCardRepairConfirmItemCount;
  } else if (menuScreen_ == MenuScreen::UpdateConfirm) {
    selectedIndex = &updateConfirmSelectedIndex_;
    itemCount = UpdateConfirmItemCount;
  } else if (menuScreen_ == MenuScreen::FocusTimerGenres) {
    selectedIndex = &focusTimerGenreSelectedIndex_;
    itemCount = focusTimerGenreMenuItems_.size();
  }

  if (itemCount == 0) {
    return;
  }

  const int next = static_cast<int>(*selectedIndex) + direction;
  if (next < 0) {
    *selectedIndex = itemCount - 1;
  } else if (next >= static_cast<int>(itemCount)) {
    *selectedIndex = 0;
  } else {
    *selectedIndex = static_cast<size_t>(next);
  }

  renderMenu();
  if (menuScreen_ == MenuScreen::Presets || menuScreen_ == MenuScreen::PresetsDeleteConfirm) {
    Serial.printf("[presets] selected=%s\n", settingsMenuItems_[presetsSelectedIndex_].c_str());
  } else if (menuScreen_ == MenuScreen::SettingsHome || menuScreen_ == MenuScreen::SettingsDisplay ||
      menuScreen_ == MenuScreen::SettingsPacing || menuScreen_ == MenuScreen::WifiSettings ||
      menuScreen_ == MenuScreen::SettingsConnectivity ||
      menuScreen_ == MenuScreen::SettingsAbout || menuScreen_ == MenuScreen::ScreensaverSettings ||
      menuScreen_ == MenuScreen::WelcomeLanguage || menuScreen_ == MenuScreen::WelcomeTheme || menuScreen_ == MenuScreen::WelcomeHighlightColor || menuScreen_ == MenuScreen::WelcomePacing || menuScreen_ == MenuScreen::WelcomeConnect) {
    Serial.printf("[settings] selected=%s\n", settingsMenuItems_[settingsSelectedIndex_].c_str());
  } else if (menuScreen_ == MenuScreen::WifiNetworks) {
    Serial.printf("[wifi] selected=%s\n", wifiNetworkMenuItems_[wifiNetworkSelectedIndex_].title.c_str());
  } else if (menuScreen_ == MenuScreen::TypographyTuning) {
    Serial.printf("[typography] selected=%s\n", typographyTuningLabel().c_str());
  } else if (menuScreen_ == MenuScreen::BookPicker) {
    Serial.printf("[book-picker] selected=%s\n",
                  bookMenuItems_[bookPickerSelectedIndex_].title.c_str());
  } else if (menuScreen_ == MenuScreen::ChapterPicker) {
    Serial.printf("[chapter-picker] selected=%s\n",
                  chapterMenuItems_[chapterPickerSelectedIndex_].c_str());
  } else if (menuScreen_ == MenuScreen::RestartConfirm) {
    String selectedLabel = uiText(UiText::AreYouSure);
    switch (restartConfirmSelectedIndex_) {
      case RestartConfirmNo:
        selectedLabel = uiText(UiText::NoKeepPlace);
        break;
      case RestartConfirmYes:
        selectedLabel = uiText(UiText::YesRestart);
        break;
      default:
        break;
    }
    Serial.printf("[restart] selected=%s\n", selectedLabel.c_str());
  } else if (menuScreen_ == MenuScreen::SdCardRepairConfirm) {
    const String selectedLabel =
        sdCardRepairConfirmSelectedIndex_ == SdCardRepairConfirmYes ? "Create folders" : "Not now";
    Serial.printf("[sd-check] selected=%s\n", selectedLabel.c_str());
  } else if (menuScreen_ == MenuScreen::UpdateConfirm) {
    const String selectedLabel =
        updateConfirmSelectedIndex_ == UpdateConfirmUpdate ? "Update" : "Skip for now";
    Serial.printf("[ota] selected=%s\n", selectedLabel.c_str());
  } else if (menuScreen_ == MenuScreen::FocusTimerGenres) {
    Serial.printf("[timer] selected genre=%s\n",
                  focusTimerGenreMenuItems_[focusTimerGenreSelectedIndex_].c_str());
  } else {
    String selectedLabel = uiText(UiText::Read);
    switch (menuSelectedIndex_) {
      case MenuRead:
        selectedLabel = uiText(UiText::Read);
        break;
      case MenuLibrary:
        selectedLabel = uiText(UiText::Library);
        break;
      case MenuSavePoints:
        selectedLabel = uiText(UiText::SavePoints);
        break;
      case MenuSettings:
        selectedLabel = uiText(UiText::Settings);
        break;
      case MenuPlugins:
        selectedLabel = uiText(UiText::Plugins);
        break;
      case MenuPowerOff:
        selectedLabel = uiText(UiText::PowerOff);
        break;
      default:
        break;
    }
    Serial.printf("[menu] selected=%s\n", selectedLabel.c_str());
  }
}

void App::selectMenuItem(uint32_t nowMs) {
  if (menuScreen_ == MenuScreen::TutorialStep1 ||
      menuScreen_ == MenuScreen::TutorialStep2 ||
      menuScreen_ == MenuScreen::TutorialStep3 ||
      menuScreen_ == MenuScreen::TutorialStep4 ||
      menuScreen_ == MenuScreen::TutorialStep5) {
    handleTutorialTap(nowMs);
    return;
  }
  if (isSettingsListScreen()) {
    selectSettingsItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::WifiNetworks) {
    selectWifiNetworkItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::TypographyTuning) {
    selectTypographyTuningItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::BookPicker) {
    selectBookPickerItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::BookDetails) {
    selectBookDetailsItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::ChapterPicker) {
    selectChapterPickerItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::SavePointsList) {
    selectSavePointItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::PluginsList) {
    selectPluginsItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::RestartConfirm) {
    selectRestartConfirmItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::SdCardRepairConfirm) {
    selectSdCardRepairConfirmItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::UpdateConfirm) {
    selectUpdateConfirmItem(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::FocusTimerGenres) {
    selectFocusTimerGenre(nowMs);
    return;
  }
  if (menuScreen_ == MenuScreen::FocusTimerSession) {
    return;
  }

  switch (menuSelectedIndex_) {
    case MenuRead:
      setState(AppState::Paused, nowMs);
      return;
    case MenuLibrary:
      openBookPicker(false);
      return;
    case MenuSavePoints:
      openSavePointsList();
      return;
    case MenuSettings:
      openSettings();
      return;
    case MenuPlugins:
      openPluginsList();
      return;
    case MenuPowerOff:
      enterPowerOff(nowMs);
      return;
    default:
      return;
  }
}

void App::openSettings() {
  settingsSelectedIndex_ = kSettingsHomeReadingIndex;
  menuScreen_ = MenuScreen::SettingsHome;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectSettingsItem(uint32_t nowMs) {
  if (settingsMenuItems_.empty()) {
    if (menuScreen_ == MenuScreen::WifiSettings) {
      openWifiSettings();
    } else {
      openSettings();
    }
    return;
  }

  if (menuScreen_ == MenuScreen::SettingsHome) {
    switch (settingsSelectedIndex_) {
      case kSettingsBackIndex:
        menuScreen_ = MenuScreen::Main;
        renderMainMenu();
        return;
      case kSettingsHomeReadingIndex:
        settingsSelectedIndex_ = kSettingsPacingReadingModeIndex;
        menuScreen_ = MenuScreen::SettingsPacing;
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsHomeDisplayIndex:
        settingsSelectedIndex_ = kSettingsDisplayThemeIndex;
        menuScreen_ = MenuScreen::SettingsDisplay;
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsHomeConnectivityIndex:
        openSettingsConnectivity();
        return;
      case kSettingsHomePresetsIndex:
        openPresets();
        return;
      case kSettingsHomeAboutIndex:
        openSettingsAbout();
        return;
      // Typography is now always visible (not dev-only)
      case kSettingsHomeTypographyIndex:
        openTypographyTuning();
        return;
      // Pozycje dev-only — branchują tylko gdy menu je faktycznie pokazało.
      case kSettingsHomeWifiIndex:
        if (devModeEnabled()) openWifiSettings();
        return;
      case kSettingsHomeUpdateIndex:
        if (devModeEnabled()) runFirmwareUpdate(preferredOtaConfig(), false, nowMs);
        return;
      default:
        return;
    }
  }

  if (menuScreen_ == MenuScreen::Presets) {
    selectPresetsItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::PresetsDeleteConfirm) {
    if (presetsSelectedIndex_ == 0) {
      // Back → return to presets list
      openPresets();
    } else if (presetsSelectedIndex_ == 1) {
      // Apply → restore the preset
      executeRestorePreset(presetsDeleteTargetIndex_, nowMs);
    } else if (presetsSelectedIndex_ == 2) {
      // Delete → delete the preset
      executeDeletePreset(nowMs);
    }
    return;
  }

  if (menuScreen_ == MenuScreen::SettingsConnectivity) {
    selectSettingsConnectivityItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::SettingsAbout) {
    selectSettingsAboutItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::WelcomeLanguage) {
    selectWelcomeLanguageItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::WelcomeTheme) {
    selectWelcomeThemeItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::WelcomeHighlightColor) {
    selectWelcomeHighlightColorItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::WelcomePacing) {
    selectWelcomePacingItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::WelcomeConnect) {
    selectWelcomeConnectItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::WifiSettings) {
    selectWifiSettingsItem(nowMs);
    return;
  }

  if (menuScreen_ == MenuScreen::SettingsDisplay) {
    switch (settingsSelectedIndex_) {
      case kSettingsBackIndex:
        settingsSelectedIndex_ = kSettingsHomeDisplayIndex;
        menuScreen_ = MenuScreen::SettingsHome;
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplayThemeIndex:
        cycleThemeMode(nowMs);
        return;
      case kSettingsDisplayBrightnessIndex:
        cycleBrightness();
        return;
      case kSettingsDisplayHandednessIndex:
        cycleHandednessMode(nowMs);
        return;
      case kSettingsDisplayFooterIndex:
        switch (footerMetricMode_) {
          case FooterMetricMode::Percentage:
            footerMetricMode_ = FooterMetricMode::ChapterTime;
            break;
          case FooterMetricMode::ChapterTime:
            footerMetricMode_ = FooterMetricMode::BookTime;
            break;
          case FooterMetricMode::BookTime:
            footerMetricMode_ = FooterMetricMode::Percentage;
            break;
        }
        preferences_.putUChar(kPrefFooterMetricMode, static_cast<uint8_t>(footerMetricMode_));
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplayBatteryIndex:
        switch (batteryLabelMode_) {
          case BatteryLabelMode::Percent:
            batteryLabelMode_ = BatteryLabelMode::TimeRemaining;
            break;
          case BatteryLabelMode::TimeRemaining:
            batteryLabelMode_ = BatteryLabelMode::Voltage;
            break;
          case BatteryLabelMode::Voltage:
          default:
            batteryLabelMode_ = BatteryLabelMode::Percent;
            break;
        }
        preferences_.putUChar(kPrefBatteryLabelMode, static_cast<uint8_t>(batteryLabelMode_));
        batteryLabel_ = currentBatteryLabel();
        display_.setBatteryLabel(batteryLabel_);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplayScreensaverIndex:
        openScreensaverSettings();
        return;
      case kSettingsDisplayReaderBatteryIndex:
        readerBatteryVisibleWhilePlaying_ = !readerBatteryVisibleWhilePlaying_;
        preferences_.putBool(kPrefReaderBatteryVisible, readerBatteryVisibleWhilePlaying_);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplayReaderChapterIndex:
        readerChapterVisibleWhilePlaying_ = !readerChapterVisibleWhilePlaying_;
        preferences_.putBool(kPrefReaderChapterVisible, readerChapterVisibleWhilePlaying_);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplayReaderProgressIndex:
        readerProgressVisibleWhilePlaying_ = !readerProgressVisibleWhilePlaying_;
        preferences_.putBool(kPrefReaderProgressVisible, readerProgressVisibleWhilePlaying_);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplayLanguageIndex:
        cycleUiLanguage(nowMs);
        return;
      case kSettingsDisplayFocusColorIndex:
        cycleFocusColor(nowMs);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplaySavePointBtnIndex:
        savePointButtonVisible_ = !savePointButtonVisible_;
        preferences_.putBool(kPrefSavePointButtonVisible, savePointButtonVisible_);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsDisplayHelpHintsIndex:
        showHelpHints_ = !showHelpHints_;
        preferences_.putBool(kPrefShowHelpHints, showHelpHints_);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      default:
        return;
    }
  }

  if (menuScreen_ == MenuScreen::ScreensaverSettings) {
    selectScreensaverSettingsItem(nowMs);
    return;
  }

  if (menuScreen_ != MenuScreen::SettingsPacing) {
    return;
  }

  bool pacingConfigChanged = false;
  switch (settingsSelectedIndex_) {
    case kSettingsBackIndex:
      flushPendingTimeEstimateRebuild();
      settingsSelectedIndex_ = kSettingsHomePacingIndex;
      menuScreen_ = MenuScreen::SettingsHome;
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kSettingsPacingReadingModeIndex:
      cycleReaderMode(nowMs);
      return;
  }

  // Scroll mode settings (indices 2, 3, 4, 5 when in Scroll mode)
  if (readerMode_ == ReaderMode::Scroll) {
    switch (settingsSelectedIndex_) {
      case kSettingsPacingScrollFontSizeIndex:
        scrollFontSize_ = static_cast<uint8_t>((scrollFontSize_ + 1) % 9);
        preferences_.putUChar(kPrefScrollFontSize, scrollFontSize_);
        display_.setScrollFontSize(scrollFontSize_);
        Serial.printf("[settings] scroll font size=%u\n", scrollFontSize_);
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsPacingScrollLineSpacingIndex:
        scrollLineSpacing_ = static_cast<uint8_t>((scrollLineSpacing_ + 1) % 3);
        preferences_.putUChar(kPrefScrollLineSpacing, scrollLineSpacing_);
        display_.setScrollLineSpacing(scrollLineSpacing_);
        Serial.printf("[settings] scroll line spacing=%s\n", scrollLineSpacingLabel().c_str());
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsPacingScrollMarginIndex:
        scrollMargin_ = static_cast<uint8_t>((scrollMargin_ + 1) % 3);
        preferences_.putUChar(kPrefScrollMargin, scrollMargin_);
        display_.setScrollMargin(scrollMargin_);
        Serial.printf("[settings] scroll margin=%s\n", scrollMarginLabel().c_str());
        rebuildSettingsMenuItems();
        renderSettings();
        return;
      case kSettingsPacingScrollPreviewIndex:
        showScrollSettingsPreview();
        return;
      default:
        return;
    }
  }

  switch (settingsSelectedIndex_) {
    case kSettingsPacingPauseModeIndex:
      pauseMode_ =
          pauseMode_ == PauseMode::SentenceEnd ? PauseMode::Instant : PauseMode::SentenceEnd;
      preferences_.putUChar(kPrefPauseMode, static_cast<uint8_t>(pauseMode_));
      Serial.printf("[settings] pause mode=%s\n", pauseModeLabel().c_str());
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kSettingsPacingWpmIndex:
      reader_.setWpm(nextReaderWpmSetting(reader_.wpm()));
      preferences_.putUShort(kPrefWpm, reader_.wpm());
      Serial.printf("[settings] WPM=%u interval=%lu ms\n", reader_.wpm(),
                    static_cast<unsigned long>(reader_.wordIntervalMs()));
      break;
    case kSettingsPacingLongWordsIndex:
      pacingLongWordDelayMs_ = static_cast<uint16_t>(nextCyclicSetting(
          pacingLongWordDelayMs_, kPacingDelayMinMs, kPacingDelayMaxMs, kPacingDelayStepMs));
      preferences_.putUShort(kPrefPacingLongMs, pacingLongWordDelayMs_);
      pacingConfigChanged = true;
      break;
    case kSettingsPacingComplexityIndex:
      pacingComplexWordDelayMs_ = static_cast<uint16_t>(nextCyclicSetting(
          pacingComplexWordDelayMs_, kPacingDelayMinMs, kPacingDelayMaxMs, kPacingDelayStepMs));
      preferences_.putUShort(kPrefPacingComplexMs, pacingComplexWordDelayMs_);
      pacingConfigChanged = true;
      break;
    case kSettingsPacingPunctuationIndex:
      pacingPunctuationDelayMs_ = static_cast<uint16_t>(nextCyclicSetting(
          pacingPunctuationDelayMs_, kPacingDelayMinMs, kPacingDelayMaxMs, kPacingDelayStepMs));
      preferences_.putUShort(kPrefPacingPunctuationMs, pacingPunctuationDelayMs_);
      pacingConfigChanged = true;
      break;
    case kSettingsPacingResetIndex:
      pacingLongWordDelayMs_ = kDefaultPacingDelayMs;
      pacingComplexWordDelayMs_ = kDefaultPacingDelayMs;
      pacingPunctuationDelayMs_ = kDefaultPacingDelayMs;
      preferences_.putUShort(kPrefPacingLongMs, pacingLongWordDelayMs_);
      preferences_.putUShort(kPrefPacingComplexMs, pacingComplexWordDelayMs_);
      preferences_.putUShort(kPrefPacingPunctuationMs, pacingPunctuationDelayMs_);
      pacingConfigChanged = true;
      break;
    default:
      return;
  }

  if (pacingConfigChanged) {
    applyPacingSettings();
  }
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::openWifiSettings() {
  settingsSelectedIndex_ = kWifiSettingsChooseIndex;
  menuScreen_ = MenuScreen::WifiSettings;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectWifiSettingsItem(uint32_t nowMs) {
  (void)nowMs;

  switch (settingsSelectedIndex_) {
    case kSettingsBackIndex:
      settingsSelectedIndex_ = kSettingsHomeWifiIndex;
      menuScreen_ = MenuScreen::SettingsHome;
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kWifiSettingsNetworkIndex:
    case kWifiSettingsChooseIndex:
      scanWifiNetworks();
      return;
    case kWifiSettingsAutoUpdateIndex:
      preferences_.putBool(kPrefOtaAuto, !otaAutoCheckEnabled());
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kWifiSettingsForgetIndex:
      preferences_.remove(kPrefWifiSsid);
      preferences_.remove(kPrefWifiPass);
      WiFi.disconnect(true, true);  // disconnect + erase stored credentials
      display_.renderStatus("Wi-Fi", tr2(TrKey2::CredentialsCleared), "");
      delay(900);
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kWifiSettingsOtaOwnerIndex:
      openTextEntry(TextEntryPurpose::OtaOwner, "OTA Source", "GitHub owner", "",
                    preferences_.getString(kPrefOtaOwner, ""), "", false, 39,
                    MenuScreen::WifiSettings);
      return;
    default:
      return;
  }
}

void App::scanWifiNetworks() {
  if (blockNetworkActionForOtaCheck("Wi-Fi", millis())) {
    return;
  }

  display_.renderProgress("Wi-Fi", "Scanning networks", "", 5);

  WiFi.persistent(false);
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_STA);
  WiFi.scanDelete();

  const int networkCount = WiFi.scanNetworks(false, true);
  wifiNetworks_.clear();
  wifiNetworkMenuItems_.clear();
  wifiNetworkMenuItems_.push_back({uiText(UiText::Back), ""});

  if (networkCount > 0) {
    for (int i = 0; i < networkCount; ++i) {
      const String ssid = WiFi.SSID(i);
      if (ssid.isEmpty()) {
        continue;
      }

      WifiNetworkInfo network;
      network.ssid = ssid;
      network.rssi = WiFi.RSSI(i);
      network.authMode = static_cast<uint8_t>(WiFi.encryptionType(i));
      wifiNetworks_.push_back(network);
    }
  }

  WiFi.scanDelete();
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);

  if (wifiNetworks_.empty()) {
    display_.renderStatus("Wi-Fi", tr2(TrKey2::NoNetworksFound), "");
    delay(1200);
    openWifiSettings();
    return;
  }

  const String savedSsid = configuredWifiSsid();
  std::stable_sort(wifiNetworks_.begin(), wifiNetworks_.end(),
                   [&savedSsid](const WifiNetworkInfo &left, const WifiNetworkInfo &right) {
                     const bool leftSaved = !savedSsid.isEmpty() && left.ssid == savedSsid;
                     const bool rightSaved = !savedSsid.isEmpty() && right.ssid == savedSsid;
                     if (leftSaved != rightSaved) {
                       return leftSaved;
                     }
                     if (left.rssi != right.rssi) {
                       return left.rssi > right.rssi;
                     }
                     return left.ssid < right.ssid;
                   });

  wifiNetworkMenuItems_.reserve(wifiNetworks_.size() + 1);
  for (const WifiNetworkInfo &network : wifiNetworks_) {
    wifiNetworkMenuItems_.push_back(
        {network.ssid, wifiSecurityLabel(network.authMode) + "  " + String(network.rssi) + " dBm"});
  }

  wifiNetworkSelectedIndex_ =
      wifiNetworkMenuItems_.size() > 1 ? kWifiNetworksFirstItemIndex : kWifiNetworksBackIndex;
  menuScreen_ = MenuScreen::WifiNetworks;
  renderWifiNetworks();
}

void App::renderWifiNetworks() {
  if (wifiNetworkMenuItems_.empty()) {
    display_.renderStatus("Wi-Fi", tr2(TrKey2::NoNetworksFound), "");
    return;
  }

  display_.renderLibrary(wifiNetworkMenuItems_, wifiNetworkSelectedIndex_);
}

void App::selectWifiNetworkItem(uint32_t nowMs) {
  (void)nowMs;

  if (wifiNetworkSelectedIndex_ == kWifiNetworksBackIndex || wifiNetworkMenuItems_.size() <= 1) {
    openWifiSettings();
    return;
  }

  const size_t networkIndex = wifiNetworkSelectedIndex_ - kWifiNetworksFirstItemIndex;
  if (networkIndex >= wifiNetworks_.size()) {
    openWifiSettings();
    return;
  }

  const WifiNetworkInfo &network = wifiNetworks_[networkIndex];
  if (wifiNetworkRequiresPassword(network.authMode)) {
    String initialValue;
    if (configuredWifiSsid() == network.ssid) {
      initialValue = preferredOtaConfig().wifiPassword;
    }
    openTextEntry(TextEntryPurpose::WifiPassword, network.ssid, "Password", "",
                  initialValue, network.ssid, true, kWifiPasswordMaxLength,
                  MenuScreen::WifiNetworks);
    return;
  }

  preferences_.putString(kPrefWifiSsid, network.ssid);
  preferences_.putString(kPrefWifiPass, "");
  display_.renderStatus("Wi-Fi", tr2(TrKey2::NetworkSaved), network.ssid);
  delay(900);
  openWifiSettings();
}

void App::openTextEntry(TextEntryPurpose purpose, const String &title, const String &prompt,
                        const String &helperText, const String &initialValue,
                        const String &contextValue, bool masked, size_t maxLength,
                        MenuScreen returnScreen) {
  textEntrySession_ = TextEntrySession();
  textEntrySession_.active = true;
  textEntrySession_.purpose = purpose;
  textEntrySession_.mode = KeyboardMode::Lower;
  textEntrySession_.returnScreen = returnScreen;
  textEntrySession_.title = title;
  textEntrySession_.prompt = prompt;
  textEntrySession_.helperText = helperText;
  textEntrySession_.value = initialValue;
  textEntrySession_.contextValue = contextValue;
  textEntrySession_.maxLength = maxLength;
  textEntrySession_.masked = masked;
  textEntrySession_.revealValue = false;
  menuScreen_ = MenuScreen::TextEntry;
  rebuildTextEntryButtons();
  renderTextEntry();
}

void App::rebuildTextEntryButtons() {
  textEntryButtons_.clear();
  if (!textEntrySession_.active) {
    return;
  }

  const uint16_t rowPitch = kKeyboardRowHeight + kKeyboardRowGap;
  for (size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
    const String rowChars = keyboardRowText(static_cast<uint8_t>(textEntrySession_.mode), rowIndex);
    const size_t keyCount = rowChars.length();
    if (keyCount == 0) {
      continue;
    }

    const int availableWidth =
        BoardConfig::DISPLAY_WIDTH - (2 * kKeyboardMarginX) -
        static_cast<int>((keyCount - 1) * kKeyboardRowGap);
    const int keyWidth = std::max(28, availableWidth / static_cast<int>(keyCount));
    const int totalWidth =
        keyWidth * static_cast<int>(keyCount) + static_cast<int>((keyCount - 1) * kKeyboardRowGap);
    int x = std::max(0, (BoardConfig::DISPLAY_WIDTH - totalWidth) / 2);
    const int y = kKeyboardTopY + static_cast<int>(rowIndex * rowPitch);

    for (size_t charIndex = 0; charIndex < keyCount; ++charIndex) {
      TextEntryButton button;
      button.view.label = String(rowChars[charIndex]);
      button.view.x = static_cast<uint16_t>(x);
      button.view.y = static_cast<uint16_t>(y);
      button.view.width = static_cast<uint16_t>(keyWidth);
      button.view.height = kKeyboardRowHeight;
      button.action = TextEntryAction::Insert;
      button.payload = String(rowChars[charIndex]);
      textEntryButtons_.push_back(button);
      x += keyWidth + kKeyboardRowGap;
    }
  }

  struct ControlButtonDef {
    String label;
    TextEntryAction action;
    uint16_t units;
    bool accent;
    bool active;
  };

  const bool revealActive = textEntrySession_.masked && textEntrySession_.revealValue;
  const ControlButtonDef controls[] = {
      {"abc", TextEntryAction::SetLower, 11, false,
       textEntrySession_.mode == KeyboardMode::Lower},
      {"ABC", TextEntryAction::SetUpper, 11, false,
       textEntrySession_.mode == KeyboardMode::Upper},
      {"123", TextEntryAction::SetSymbols, 11, false,
       textEntrySession_.mode == KeyboardMode::Symbols},
      {"space", TextEntryAction::Space, 24, false, false},
      {"back", TextEntryAction::Backspace, 13, false, false},
      {textEntrySession_.masked ? (revealActive ? "hide" : "show") : "clear",
       textEntrySession_.masked ? TextEntryAction::ToggleMask : TextEntryAction::Clear, 13, false,
       revealActive},
      {"save", TextEntryAction::Save, 12, true, false},
      {"cancel", TextEntryAction::Cancel, 14, false, false},
  };

  uint16_t totalUnits = 0;
  for (const ControlButtonDef &control : controls) {
    totalUnits += control.units;
  }

  const size_t controlCount = sizeof(controls) / sizeof(controls[0]);
  const int totalGapWidth = static_cast<int>((controlCount - 1) * kKeyboardRowGap);
  const int availableWidth = BoardConfig::DISPLAY_WIDTH - (2 * kKeyboardMarginX) - totalGapWidth;
  int remainingWidth = availableWidth;
  uint16_t x = kKeyboardMarginX;
  const uint16_t y = kKeyboardTopY + static_cast<uint16_t>(3 * rowPitch);

  for (size_t i = 0; i < controlCount; ++i) {
    const ControlButtonDef &control = controls[i];
    int width = remainingWidth;
    if (i + 1 < controlCount) {
      width = (availableWidth * control.units) / totalUnits;
      remainingWidth -= width;
    }

    TextEntryButton button;
    button.view.label = control.label;
    button.view.x = x;
    button.view.y = y;
    button.view.width = static_cast<uint16_t>(std::max(28, width));
    button.view.height = kKeyboardRowHeight;
    button.view.accent = control.accent;
    button.view.active = control.active;
    button.action = control.action;
    textEntryButtons_.push_back(button);

    x = static_cast<uint16_t>(x + button.view.width + kKeyboardRowGap);
  }
}

void App::renderTextEntry() {
  if (!textEntrySession_.active) {
    return;
  }

  const String visibleValue =
      (textEntrySession_.masked && !textEntrySession_.revealValue)
          ? maskedValue(textEntrySession_.value)
          : textEntrySession_.value;

  std::vector<DisplayManager::Button> buttons;
  buttons.reserve(textEntryButtons_.size());
  for (const TextEntryButton &button : textEntryButtons_) {
    buttons.push_back(button.view);
  }

  display_.renderTextEntry(textEntrySession_.title, textEntrySession_.prompt, visibleValue,
                           textEntrySession_.helperText, buttons);
}

bool App::handleTextEntryTap(uint16_t x, uint16_t y, uint32_t nowMs) {
  if (!textEntrySession_.active) {
    return false;
  }

  for (size_t i = 0; i < textEntryButtons_.size(); ++i) {
    const DisplayManager::Button &button = textEntryButtons_[i].view;
    const uint16_t maxX = button.x + button.width;
    const uint16_t maxY = button.y + button.height;
    if (x < button.x || x > maxX || y < button.y || y > maxY) {
      continue;
    }

    activateTextEntryButton(i, nowMs);
    return true;
  }

  return false;
}

void App::activateTextEntryButton(size_t buttonIndex, uint32_t nowMs) {
  if (buttonIndex >= textEntryButtons_.size()) {
    return;
  }

  TextEntryButton &button = textEntryButtons_[buttonIndex];
  switch (button.action) {
    case TextEntryAction::Insert:
      if (textEntrySession_.value.length() < textEntrySession_.maxLength) {
        textEntrySession_.value += button.payload;
      }
      break;
    case TextEntryAction::SetLower:
      textEntrySession_.mode = KeyboardMode::Lower;
      break;
    case TextEntryAction::SetUpper:
      textEntrySession_.mode = KeyboardMode::Upper;
      break;
    case TextEntryAction::SetSymbols:
      textEntrySession_.mode = KeyboardMode::Symbols;
      break;
    case TextEntryAction::Space:
      if (textEntrySession_.value.length() < textEntrySession_.maxLength) {
        textEntrySession_.value += ' ';
      }
      break;
    case TextEntryAction::Backspace:
      if (!textEntrySession_.value.isEmpty()) {
        textEntrySession_.value.remove(textEntrySession_.value.length() - 1);
      }
      break;
    case TextEntryAction::Clear:
      textEntrySession_.value = "";
      break;
    case TextEntryAction::ToggleMask:
      if (textEntrySession_.masked) {
        textEntrySession_.revealValue = !textEntrySession_.revealValue;
      }
      break;
    case TextEntryAction::Save:
      commitTextEntry(nowMs);
      return;
    case TextEntryAction::Cancel:
      menuScreen_ = textEntrySession_.returnScreen;
      textEntrySession_ = TextEntrySession();
      textEntryButtons_.clear();
      renderMenu();
      return;
  }

  rebuildTextEntryButtons();
  renderTextEntry();
}

void App::commitTextEntry(uint32_t nowMs) {
  (void)nowMs;

  switch (textEntrySession_.purpose) {
    case TextEntryPurpose::WifiPassword: {
      if (textEntrySession_.value.isEmpty()) {
        display_.renderStatus("Wi-Fi", tr2(TrKey2::PasswordRequired), textEntrySession_.contextValue);
        delay(1000);
        renderTextEntry();
        return;
      }

      const String ssid = textEntrySession_.contextValue;
      preferences_.putString(kPrefWifiSsid, ssid);
      preferences_.putString(kPrefWifiPass, textEntrySession_.value);
      textEntrySession_ = TextEntrySession();
      textEntryButtons_.clear();
      display_.renderStatus("Wi-Fi", tr2(TrKey2::NetworkSaved), ssid);
      delay(900);
      openWifiSettings();
      return;
    }
    case TextEntryPurpose::OtaOwner: {
      const String owner = textEntrySession_.value;
      if (owner.isEmpty()) {
        preferences_.remove(kPrefOtaOwner);
        display_.renderStatus("OTA", tr2(TrKey2::ResetToDefault), "");
      } else {
        preferences_.putString(kPrefOtaOwner, owner);
        display_.renderStatus("OTA", tr2(TrKey2::OwnerSaved), owner);
      }
      delay(900);
      textEntrySession_ = TextEntrySession();
      textEntryButtons_.clear();
      openWifiSettings();
      return;
    }
    case TextEntryPurpose::SavePointName: {
      // Create save point with the entered name
      String name = textEntrySession_.value;
      if (name.isEmpty()) {
        // Use context value as default name
        name = textEntrySession_.contextValue;
      }
      textEntrySession_ = TextEntrySession();
      textEntryButtons_.clear();

      if (usingStorageBook_ && !currentBookPath_.isEmpty()) {
        SavePoint sp;
        sp.bookPath = currentBookPath_;
        sp.bookTitle = currentBookTitle_;
        sp.wordIndex = reader_.currentIndex();
        sp.progressPercent = readingProgressPercent();
        sp.name = name;
        loadSavePoints();
        savePoints_.insert(savePoints_.begin(), sp);
        if (savePoints_.size() > kMaxSavePoints) {
          savePoints_.pop_back();
        }
        persistSavePoints();
        Serial.printf("[save-point] created: %s word=%u\n", sp.name.c_str(),
                      static_cast<unsigned int>(sp.wordIndex));
        display_.renderStatus(uiText(UiText::SavePoints),
                              polish("Zakladka dodana", "Bookmark added"), name);
        delay(1200);
      }
      openSavePointsList();
      return;
    }
    case TextEntryPurpose::PresetName: {
      executeSavePreset(nowMs);
      return;
    }
    case TextEntryPurpose::None:
    default:
      menuScreen_ = textEntrySession_.returnScreen;
      textEntrySession_ = TextEntrySession();
      textEntryButtons_.clear();
      renderMenu();
      return;
  }
}

void App::openTypographyTuning() {
  if (typographyTuningSelectedIndex_ >= TypographyTuningItemCount) {
    typographyTuningSelectedIndex_ = TypographyTuningFontSize;
  }
  if (typographyTuningSelectedIndex_ == TypographyTuningBack) {
    typographyTuningSelectedIndex_ = TypographyTuningFontSize;
  }
  menuScreen_ = MenuScreen::TypographyTuning;
  renderTypographyTuning();
}

void App::selectTypographyTuningItem(uint32_t nowMs) {
  switch (typographyTuningSelectedIndex_) {
    case TypographyTuningBack:
      settingsSelectedIndex_ = kSettingsHomeTypographyIndex;
      menuScreen_ = MenuScreen::SettingsHome;
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case TypographyTuningFontSize:
      cycleReaderFontSize(nowMs);
      return;
    case TypographyTuningTypeface:
      typographyConfig_.typeface = nextReaderTypeface(typographyConfig_.typeface);
      preferences_.putUChar(kPrefReaderTypeface, static_cast<uint8_t>(typographyConfig_.typeface));
      break;
    case TypographyTuningPhantomWords:
      togglePhantomWords(nowMs);
      return;
    case TypographyTuningFocusHighlight:
      typographyConfig_.focusHighlight = !typographyConfig_.focusHighlight;
      preferences_.putBool(kPrefTypographyFocusHighlight, typographyConfig_.focusHighlight);
      break;
    case TypographyTuningTracking:
      typographyConfig_.trackingPx = static_cast<int8_t>(
          nextCyclicSetting(typographyConfig_.trackingPx, kTypographyTrackingMin,
                            kTypographyTrackingMax));
      preferences_.putChar(kPrefTypographyTracking, typographyConfig_.trackingPx);
      break;
    case TypographyTuningAnchor: {
      const uint8_t anchorMin =
          (handednessMode_ == HandednessMode::Left) ? kLeftHandAnchorMin : kTypographyAnchorMin;
      const uint8_t anchorMax =
          (handednessMode_ == HandednessMode::Left) ? kLeftHandAnchorMax : kTypographyAnchorMax;
      const uint8_t nextAnchorPercent = static_cast<uint8_t>(
          nextCyclicSetting(effectiveAnchorPercent(), anchorMin, anchorMax));
      typographyConfig_.anchorPercent = (handednessMode_ == HandednessMode::Left)
                                            ? static_cast<uint8_t>(nextAnchorPercent -
                                                                   kLeftHandAnchorOffset)
                                            : nextAnchorPercent;
      preferences_.putUChar(kPrefTypographyAnchor, typographyConfig_.anchorPercent);
      break;
    }
    case TypographyTuningGuideWidth:
      typographyConfig_.guideHalfWidth = static_cast<uint8_t>(nextCyclicSetting(
          typographyConfig_.guideHalfWidth, kTypographyGuideWidthMin,
          kTypographyGuideWidthMax, kTypographyGuideWidthStep));
      preferences_.putUChar(kPrefTypographyGuideWidth, typographyConfig_.guideHalfWidth);
      break;
    case TypographyTuningGuideGap:
      typographyConfig_.guideGap = static_cast<uint8_t>(nextCyclicSetting(
          typographyConfig_.guideGap, kTypographyGuideGapMin, kTypographyGuideGapMax));
      preferences_.putUChar(kPrefTypographyGuideGap, typographyConfig_.guideGap);
      break;
    case TypographyTuningReset:
      typographyConfig_ = defaultTypographyConfig();
      preferences_.putUChar(kPrefReaderTypeface, static_cast<uint8_t>(typographyConfig_.typeface));
      preferences_.putBool(kPrefTypographyFocusHighlight, typographyConfig_.focusHighlight);
      preferences_.putChar(kPrefTypographyTracking, typographyConfig_.trackingPx);
      preferences_.putUChar(kPrefTypographyAnchor, typographyConfig_.anchorPercent);
      preferences_.putUChar(kPrefTypographyGuideWidth, typographyConfig_.guideHalfWidth);
      preferences_.putUChar(kPrefTypographyGuideGap, typographyConfig_.guideGap);
      break;
    default:
      return;
  }

  applyTypographySettings(nowMs);
}

void App::cycleTypographyPreviewSample(int direction) {
  if (kTypographyPreviewWordCount == 0 || direction == 0) {
    return;
  }

  const int current = static_cast<int>(typographyPreviewSampleIndex_);
  int next = current + direction;
  if (next < 0) {
    next = static_cast<int>(kTypographyPreviewWordCount) - 1;
  } else if (next >= static_cast<int>(kTypographyPreviewWordCount)) {
    next = 0;
  }
  typographyPreviewSampleIndex_ = static_cast<size_t>(next);
  renderTypographyTuning();
}

void App::rebuildSettingsMenuItems() {
  settingsMenuItems_.clear();
  settingsMenuItems_.reserve(SettingsItemCount);
  if (menuScreen_ == MenuScreen::SettingsHome) {
    // Nowy układ: 6 pozycji zawsze widocznych, dev-only na końcu.
    settingsMenuItems_.push_back(uiText(UiText::Back));
    settingsMenuItems_.push_back(polish("Czytanie", "Reading"));   // 1 = Reading settings
    settingsMenuItems_.push_back(uiText(UiText::Display));         // 2 = Display
    settingsMenuItems_.push_back(uiText(UiText::TypographyTune)); // 3 = Typography (always)
    settingsMenuItems_.push_back(tr(TrKey::Connectivity));         // 4
    settingsMenuItems_.push_back(polish("Presety", "Presets"));    // 5 = Presets
    settingsMenuItems_.push_back(tr(TrKey::AboutHelp));            // 6
    if (devModeEnabled()) {
      settingsMenuItems_.push_back(tr(TrKey::WifiAdvanced));       // 7 dev
      settingsMenuItems_.push_back(firmwareUpdateMenuLabel());     // 8 dev
    }
  } else if (menuScreen_ == MenuScreen::SettingsConnectivity) {
    settingsMenuItems_.push_back(uiText(UiText::Back));
    // 1. Wi-Fi
    settingsMenuItems_.push_back(String(tr(TrKey::HomeWifi)) +
                                 storedOrFallbackLabel(configuredWifiSsid(),
                                                       tr(TrKey::NotSet)));
#if FLOWER_BLE_ENABLED
    // 2. Bluetooth
    settingsMenuItems_.push_back(
        String("Bluetooth: ") +
        (ble_.isActive()
             ? (ble_.isConnected() ? tr(TrKey::Connected)
                                   : tr(TrKey::Yes))
             : tr(TrKey::No)));
#endif
    // 3. Synchronizacja z telefonem
    const bool syncActive = state_ == AppState::CompanionSync;
    settingsMenuItems_.push_back(
        String(tr(TrKey::PhoneSync)) +
        (syncActive ? tr(TrKey::Yes) : tr(TrKey::No)));
    // 4. Kanaly RSS
    settingsMenuItems_.push_back(tr2(TrKey2::RssFeeds));
#if RSVP_USB_TRANSFER_ENABLED
    // 5. Kopiuj przez USB
    settingsMenuItems_.push_back(uiText(UiText::UsbTransfer));
#endif
  } else if (menuScreen_ == MenuScreen::SettingsAbout) {
    settingsMenuItems_.push_back(uiText(UiText::Back));
    settingsMenuItems_.push_back(String(tr(TrKey::Version)) +
                                 otaUpdater_.currentVersion());
    settingsMenuItems_.push_back(tr(TrKey::BrandLabel));
    settingsMenuItems_.push_back(tr2(TrKey2::SdCardCheck));
    settingsMenuItems_.push_back(polish("Samouczek", "Tutorial"));
    if (devModeEnabled()) {
      settingsMenuItems_.push_back(tr(TrKey::DevModeOn));
    }
  } else if (menuScreen_ == MenuScreen::WelcomeLanguage) {
    // First-run wizard — krok 1/2. Lista języków po ich własnych nazwach
    // (te się nie tłumaczą — Polski to zawsze Polski, niezależnie od języka UI).
    settingsMenuItems_.push_back("English");
    settingsMenuItems_.push_back("Polski");
    settingsMenuItems_.push_back("Deutsch");
    settingsMenuItems_.push_back("Espanol");
    settingsMenuItems_.push_back("Francais");
    settingsMenuItems_.push_back("Romana");
  } else if (menuScreen_ == MenuScreen::WelcomeTheme) {
    // First-run wizard — krok 2/5. Po wyborze języka w step 1 (już zapisanym
    // w uiLanguage_), te labele lecą przez UiText więc są przetłumaczone.
    settingsMenuItems_.push_back(uiText(UiText::Light));
    settingsMenuItems_.push_back(uiText(UiText::Dark));
    settingsMenuItems_.push_back(uiText(UiText::Night));
  } else if (menuScreen_ == MenuScreen::WelcomeHighlightColor) {
    // First-run wizard — krok 3/5. Kolor podświetlenia litery.
    settingsMenuItems_.push_back(polish("Czerwony", "Red"));
    settingsMenuItems_.push_back(polish("Niebieski", "Blue"));
    settingsMenuItems_.push_back(polish("Zielony", "Green"));
    settingsMenuItems_.push_back(polish("Zolty", "Yellow"));
    settingsMenuItems_.push_back(polish("Pomaranczowy", "Orange"));
    settingsMenuItems_.push_back(polish("Fioletowy", "Purple"));
  } else if (menuScreen_ == MenuScreen::WelcomePacing) {
    // First-run wizard — krok 4/5. Spowolnienie po kropkach/długich słowach.
    settingsMenuItems_.push_back(polish("Brak (0 ms)", "None (0 ms)"));
    settingsMenuItems_.push_back(polish("Lekkie (100 ms)", "Light (100 ms)"));
    settingsMenuItems_.push_back(polish("Srednie (200 ms)", "Medium (200 ms)"));
    settingsMenuItems_.push_back(polish("Mocne (300 ms)", "Strong (300 ms)"));
    settingsMenuItems_.push_back(polish("Bardzo mocne (400 ms)", "Very strong (400 ms)"));
  } else if (menuScreen_ == MenuScreen::WelcomeConnect) {
    // First-run wizard — krok 5/5. Połączenie z telefonem.
    // Więcej pozycji żeby scroll nie przeskakiwał od razu na "Pomiń".
    const String ssid = companionSync_.active() ? companionSync_.statusLine1() : "Flower";
    settingsMenuItems_.push_back(String("Wi-Fi: ") + ssid);
    settingsMenuItems_.push_back("IP: 192.168.4.1");
    settingsMenuItems_.push_back("---");
    settingsMenuItems_.push_back(tr2(TrKey2::CompanionSync));
    settingsMenuItems_.push_back(tr2(TrKey2::SkipForNow));
  } else if (menuScreen_ == MenuScreen::SettingsDisplay) {
    settingsMenuItems_.push_back(uiText(UiText::Back));
    settingsMenuItems_.push_back(String(uiText(UiText::Theme)) + ": " + themeModeLabel());
    settingsMenuItems_.push_back(uiText(UiText::Brightness) + ": " +
                                 String(currentBrightnessPercent()) + "%");
    settingsMenuItems_.push_back(String(tr(TrKey::ReaderHand)) + handednessLabel());
    settingsMenuItems_.push_back(String(tr(TrKey::FooterLabel)) +
                                 footerMetricModeLabel());
    settingsMenuItems_.push_back(String(tr(TrKey::BatteryLabel)) +
                                 batteryLabelModeLabel());
    settingsMenuItems_.push_back(String(tr(TrKey::Screensaver)) +
                                 screensaverModeLabel() + " >");
    settingsMenuItems_.push_back(String(tr(TrKey::ReadingBattery)) +
                                 onOffLabel(readerBatteryVisibleWhilePlaying_));
    settingsMenuItems_.push_back(String(tr(TrKey::ReadingChapter)) +
                                 onOffLabel(readerChapterVisibleWhilePlaying_));
    settingsMenuItems_.push_back(String(tr(TrKey::ReadingPercent)) +
                                 onOffLabel(readerProgressVisibleWhilePlaying_));
    settingsMenuItems_.push_back(uiText(UiText::Language) + ": " + uiLanguageLabel());
    settingsMenuItems_.push_back(polish("Kolor litery: ", "Focus color: ") + focusColorLabel());
    settingsMenuItems_.push_back(polish("Przycisk zapisu: ", "Save btn: ") +
                                 onOffLabel(savePointButtonVisible_));
    settingsMenuItems_.push_back(polish("Pomoc (?): ", "Help (?): ") +
                                 onOffLabel(showHelpHints_));
  } else if (menuScreen_ == MenuScreen::ScreensaverSettings) {
    settingsMenuItems_.push_back(uiText(UiText::Back));
    settingsMenuItems_.push_back(String(tr(TrKey::ScreensaverStyle)) +
                                 screensaverModeLabel());
    settingsMenuItems_.push_back(String(tr(TrKey::ScreensaverTimeout)) +
                                 screensaverTimeoutLabel());
    settingsMenuItems_.push_back(String(tr(TrKey::ScreensaverAutoOff)) +
                                 screensaverAutoOffLabel());
    settingsMenuItems_.push_back(String(tr(TrKey::ScreensaverSleepGuard)) +
                                 screensaverSleepGuardLabel());
    settingsMenuItems_.push_back(tr(TrKey::ScreensaverPreview));
  } else if (menuScreen_ == MenuScreen::SettingsPacing) {
    settingsMenuItems_.push_back(uiText(UiText::Back));
    settingsMenuItems_.push_back(uiText(UiText::ReadingMode) + ": " + readerModeLabel());
    if (readerMode_ == ReaderMode::Scroll) {
      settingsMenuItems_.push_back(uiText(UiText::FontSize) + ": " + scrollFontSizeLabel());
      settingsMenuItems_.push_back(uiText(UiText::ScrollLineSpacing) + ": " + scrollLineSpacingLabel());
      settingsMenuItems_.push_back(uiText(UiText::ScrollMargins) + ": " + scrollMarginLabel());
      settingsMenuItems_.push_back(String("Preview"));
    } else {
      settingsMenuItems_.push_back(String(tr(TrKey::PauseBehaviour)) +
                                   pauseModeLabel());
      settingsMenuItems_.push_back(String(tr(TrKey::BaseSpeed)) +
                                   String(reader_.wpm()) + " WPM");
      settingsMenuItems_.push_back(uiText(UiText::LongWords) + ": " +
                                   pacingDelayLabel(pacingLongWordDelayMs_));
      settingsMenuItems_.push_back(uiText(UiText::Complexity) + ": " +
                                   pacingDelayLabel(pacingComplexWordDelayMs_));
      settingsMenuItems_.push_back(uiText(UiText::Punctuation) + ": " +
                                   pacingDelayLabel(pacingPunctuationDelayMs_));
      settingsMenuItems_.push_back(uiText(UiText::ResetPacing));
    }
  } else if (menuScreen_ == MenuScreen::WifiSettings) {
    settingsMenuItems_.push_back(uiText(UiText::Back));
    settingsMenuItems_.push_back(String(tr(TrKey::Network)) +
                                 storedOrFallbackLabel(configuredWifiSsid(),
                                                       tr(TrKey::NotSet)));
    settingsMenuItems_.push_back(tr(TrKey::ChooseNetwork));
    settingsMenuItems_.push_back(tr(TrKey::ForgetNetwork));
    // Advanced — pokazywane tylko w trybie developera. Klient nie potrzebuje
    // grzebać w „OTA Owner" ani „Auto OTA"; te rzeczy steruje się z aplikacji.
    if (devModeEnabled()) {
      settingsMenuItems_.push_back(String("Auto OTA: ") +
                                   (otaAutoCheckEnabled() ? tr(TrKey::Yes) : tr(TrKey::No)));
      settingsMenuItems_.push_back(String("OTA Owner: ") + otaOwnerLabel());
    }
  }

  if (settingsSelectedIndex_ >= settingsMenuItems_.size()) {
    settingsSelectedIndex_ = kSettingsBackIndex;
  }
}

void App::applyPacingSettings() {
  ReadingLoop::PacingConfig pacingConfig;
  pacingConfig.longWordDelayMs = pacingLongWordDelayMs_;
  pacingConfig.complexWordDelayMs = pacingComplexWordDelayMs_;
  pacingConfig.punctuationDelayMs = pacingPunctuationDelayMs_;
  reader_.setPacingConfig(pacingConfig);

  Serial.printf("[settings] pacing long=%u ms complexity=%u ms punctuation=%u ms\n",
                static_cast<unsigned int>(pacingLongWordDelayMs_),
                static_cast<unsigned int>(pacingComplexWordDelayMs_),
                static_cast<unsigned int>(pacingPunctuationDelayMs_));
  if (state_ == AppState::Menu && menuScreen_ == MenuScreen::SettingsPacing) {
    pacingCacheDirty_ = true;
  } else {
    rebuildTimeEstimateCache();
  }
}

void App::flushPendingTimeEstimateRebuild() {
  if (!pacingCacheDirty_) {
    return;
  }
  rebuildTimeEstimateCache();
}

String App::otaOwnerLabel() {
  if (preferences_.isKey(kPrefOtaOwner)) {
    return preferences_.getString(kPrefOtaOwner, "");
  }
  OtaUpdater::Config cfg;
  otaUpdater_.loadConfig(cfg);
  return cfg.githubOwner;
}

bool App::devModeEnabled() {
  return preferences_.getBool(kPrefDevMode, false);
}

String App::firmwareVersionLabel() const { return otaUpdater_.currentVersion(); }

void App::setDevModeEnabled(bool enabled) {
  preferences_.putBool(kPrefDevMode, enabled);
  if (state_ == AppState::Menu &&
      (menuScreen_ == MenuScreen::WifiSettings || menuScreen_ == MenuScreen::SettingsHome ||
       menuScreen_ == MenuScreen::SettingsAbout)) {
    rebuildSettingsMenuItems();
  }
}

bool App::isSettingsListScreen() const {
  return menuScreen_ == MenuScreen::SettingsHome ||
         menuScreen_ == MenuScreen::SettingsDisplay ||
         menuScreen_ == MenuScreen::SettingsPacing ||
         menuScreen_ == MenuScreen::SettingsConnectivity ||
         menuScreen_ == MenuScreen::SettingsAbout ||
         menuScreen_ == MenuScreen::ScreensaverSettings ||
         menuScreen_ == MenuScreen::WifiSettings ||
         menuScreen_ == MenuScreen::Presets ||
         menuScreen_ == MenuScreen::PresetsDeleteConfirm ||
         menuScreen_ == MenuScreen::WelcomeLanguage ||
         menuScreen_ == MenuScreen::WelcomeTheme ||
         menuScreen_ == MenuScreen::WelcomeHighlightColor ||
         menuScreen_ == MenuScreen::WelcomePacing ||
         menuScreen_ == MenuScreen::WelcomeConnect;
}

void App::showHelpForCurrentItem() {
  if (!showHelpHints_) return;
  if (settingsSelectedIndex_ == kSettingsBackIndex) return;

  const HelpEntry* entry = nullptr;

  if (menuScreen_ == MenuScreen::SettingsDisplay) {
    entry = HelpTexts::getDisplayHelp(settingsSelectedIndex_ - 1);
  } else if (menuScreen_ == MenuScreen::SettingsPacing) {
    if (readerMode_ == ReaderMode::Scroll) {
      entry = HelpTexts::getPacingScrollHelp(settingsSelectedIndex_ - 1);
    } else {
      entry = HelpTexts::getPacingHelp(settingsSelectedIndex_ - 1);
    }
  }

  if (entry == nullptr) return;

  const bool isPl = (uiLanguage_ == UiLanguage::Polish);
  helpPopupTitle_ = isPl ? entry->titlePl : entry->titleEn;
  helpPopupDesc_ = isPl ? entry->line1Pl : entry->line1En;
  showingHelpPopup_ = true;

  const char* line2 = isPl ? entry->line2Pl : entry->line2En;
  display_.renderStatus(String(helpPopupTitle_), String(helpPopupDesc_),
                        line2 ? String(line2) : "");
}

void App::dismissHelpPopup(uint32_t nowMs) {
  (void)nowMs;
  showingHelpPopup_ = false;
  helpPopupTitle_ = nullptr;
  helpPopupDesc_ = nullptr;
  rebuildSettingsMenuItems();
  renderSettings();
}

const char *App::polish(const char *pl, const char *en) const {
  // DEPRECATED — use tr(TrKey) instead. Kept temporarily for any stragglers.
  return uiLanguage_ == UiLanguage::Polish ? pl : en;
}

const char *App::tr(TrKey key) const {
  return Translations::tr(uiLanguage_, key);
}

const char *App::tr2(TrKey2 key) const {
  return Translations2::tr2(uiLanguage_, key);
}

// ─── SettingsConnectivity ────────────────────────────────────────────────────

void App::openSettingsConnectivity() {
  menuScreen_ = MenuScreen::SettingsConnectivity;
  settingsSelectedIndex_ = kSettingsConnWifiIndex;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectSettingsConnectivityItem(uint32_t nowMs) {
  switch (settingsSelectedIndex_) {
    case kSettingsBackIndex:
      settingsSelectedIndex_ = kSettingsHomeConnectivityIndex;
      menuScreen_ = MenuScreen::SettingsHome;
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kSettingsConnWifiIndex:
      openWifiSettings();
      return;
#if FLOWER_BLE_ENABLED
    case kSettingsConnBluetoothIndex: {
      const bool wasOn = ble_.isActive();
      if (wasOn) {
        ble_.stop();
        preferences_.putBool(kPrefBleEnabled, false);
        Serial.println("[app] BLE turned OFF by user");
      } else {
        ble_.begin(this);
        preferences_.putBool(kPrefBleEnabled, true);
        Serial.printf("[app] BLE turned ON by user (name=%s)\n", ble_.deviceName().c_str());
      }
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    }
#endif
    case kSettingsConnSyncToggleIndex:
      if (state_ == AppState::CompanionSync) {
        exitCompanionSync(nowMs);
      } else {
        enterCompanionSync(nowMs);
      }
      return;
    case kSettingsConnRssIndex:
      runRssFeedCheck(nowMs);
      return;
#if RSVP_USB_TRANSFER_ENABLED
    case kSettingsConnUsbIndex:
      enterUsbTransfer(nowMs);
      return;
#endif
    default:
      return;
  }
}

// ─── SettingsAbout ───────────────────────────────────────────────────────────

void App::openSettingsAbout() {
  menuScreen_ = MenuScreen::SettingsAbout;
  settingsSelectedIndex_ = kSettingsAboutVersionIndex;
  aboutTapCount_ = 0;
  aboutLastTapMs_ = 0;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectSettingsAboutItem(uint32_t nowMs) {
  switch (settingsSelectedIndex_) {
    case kSettingsBackIndex:
      settingsSelectedIndex_ = kSettingsHomeAboutIndex;
      menuScreen_ = MenuScreen::SettingsHome;
      aboutTapCount_ = 0;
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kSettingsAboutVersionIndex: {
      // Łańcuch tapów odblokowuje dev mode (jak Android Build number).
      constexpr uint32_t kTapWindowMs = 1500;
      constexpr uint8_t kTapsToUnlock = 10;
      if (aboutLastTapMs_ != 0 && nowMs - aboutLastTapMs_ > kTapWindowMs) {
        aboutTapCount_ = 0;
      }
      aboutLastTapMs_ = nowMs;
      aboutTapCount_++;
      if (!devModeEnabled() && aboutTapCount_ >= kTapsToUnlock) {
        setDevModeEnabled(true);
        aboutTapCount_ = 0;
      }
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    }
    case kSettingsAboutSdCardIndex:
      runSdCardCheck(nowMs);
      return;
    case kSettingsAboutTutorialIndex:
      openTutorialStep1();
      return;
    case kSettingsAboutDevModeIndex:
      // Pokazane tylko gdy dev mode jest już włączone — pozwala wyłączyć.
      if (devModeEnabled()) {
        setDevModeEnabled(false);
      }
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kSettingsAboutBrandIndex:
    default:
      return;
  }
}

// ─── First-run welcome wizard ────────────────────────────────────────────────

void App::openWelcomeLanguage() {
  menuScreen_ = MenuScreen::WelcomeLanguage;
  // Bez „Back" — w wizardzie zaczynamy od pierwszego elementu listy.
  settingsSelectedIndex_ = 0;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectWelcomeLanguageItem(uint32_t /*nowMs*/) {
  // Zachowaj kolejność z rebuildSettingsMenuItems() dla WelcomeLanguage:
  // 0 English, 1 Polski, 2 Deutsch, 3 Español, 4 Français, 5 Română.
  // Mapujemy na UiLanguage enum (0=English, 1=Spanish, 2=French,
  // 3=German, 4=Romanian, 5=Polish — patrz Localization.h).
  static const uint8_t kLangByIndex[] = {0, 5, 3, 1, 2, 4};
  if (settingsSelectedIndex_ < sizeof(kLangByIndex)) {
    // Tak jak cycleUiLanguage: najpierw member field, potem pref.
    uiLanguage_ = Localization::sanitizeLanguage(kLangByIndex[settingsSelectedIndex_]);
    preferences_.putUChar(kPrefUiLanguage, static_cast<uint8_t>(uiLanguage_));
    Serial.printf("[welcome] language=%s (idx=%u → enum=%u)\n",
                  uiLanguageLabel().c_str(),
                  static_cast<unsigned>(settingsSelectedIndex_),
                  static_cast<unsigned>(uiLanguage_));
  }
  openWelcomeTheme();
}

void App::openWelcomeTheme() {
  menuScreen_ = MenuScreen::WelcomeTheme;
  settingsSelectedIndex_ = 0;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectWelcomeThemeItem(uint32_t nowMs) {
  // 0 Light, 1 Dark, 2 Night.
  const bool dark = settingsSelectedIndex_ >= 1;
  const bool night = settingsSelectedIndex_ == 2;
  // KRYTYCZNE — applyDisplayPreferences czyta member fields, nie preferences.
  // Bez ustawienia darkMode_/nightMode_ przed apply, motyw się nie zmieni.
  // To samo robi cycleThemeMode (App.cpp:1466) — kopiujemy ten wzorzec.
  darkMode_ = dark;
  nightMode_ = night;
  preferences_.putBool(kPrefDarkMode, darkMode_);
  preferences_.putBool(kPrefNightMode, nightMode_);
  Serial.printf("[welcome] theme dark=%d night=%d\n",
                static_cast<int>(darkMode_), static_cast<int>(nightMode_));
  applyDisplayPreferences(nowMs);
  openWelcomeHighlightColor();
}

void App::openWelcomeHighlightColor() {
  menuScreen_ = MenuScreen::WelcomeHighlightColor;
  settingsSelectedIndex_ = display_.focusColorIndex();
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectWelcomeHighlightColorItem(uint32_t /*nowMs*/) {
  // 0=red, 1=blue, 2=green, 3=yellow, 4=orange, 5=purple
  const uint8_t colorIndex = static_cast<uint8_t>(settingsSelectedIndex_);
  display_.setFocusColorIndex(colorIndex);
  preferences_.putUChar(kPrefFocusColorIndex, colorIndex);
  Serial.printf("[welcome] highlight color=%u\n", static_cast<unsigned>(colorIndex));
  openWelcomePacing();
}

void App::openWelcomePacing() {
  menuScreen_ = MenuScreen::WelcomePacing;
  // Default selection: Medium (200ms) = index 2
  settingsSelectedIndex_ = 2;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectWelcomePacingItem(uint32_t nowMs) {
  // 0=None(0ms), 1=Light(100ms), 2=Medium(200ms), 3=Strong(300ms), 4=VeryStrong(400ms)
  constexpr uint16_t kPacingValues[] = {0, 100, 200, 300, 400};
  const uint16_t delayMs = kPacingValues[settingsSelectedIndex_ < 5 ? settingsSelectedIndex_ : 2];
  pacingLongWordDelayMs_ = delayMs;
  pacingComplexWordDelayMs_ = delayMs;
  pacingPunctuationDelayMs_ = delayMs;
  preferences_.putUShort(kPrefPacingLongMs, delayMs);
  preferences_.putUShort(kPrefPacingComplexMs, delayMs);
  preferences_.putUShort(kPrefPacingPunctuationMs, delayMs);
  applyPacingSettings();
  Serial.printf("[welcome] pacing delay=%u ms\n", static_cast<unsigned>(delayMs));
  openWelcomeConnect(nowMs);
}

void App::openWelcomeConnect(uint32_t nowMs) {
  (void)nowMs;
  menuScreen_ = MenuScreen::WelcomeConnect;
  settingsSelectedIndex_ = 0;
  rebuildSettingsMenuItems();
  renderSettings();

  // Start AP if not already running from auto-sync
  if (!autoSyncActive_ && !companionSync_.active()) {
    CompanionSyncManager::Config syncConfig;
    syncConfig.wifiSsid = "";
    syncConfig.wifiPassword = "";
    if (companionSync_.begin(syncConfig)) {
      autoSyncActive_ = true;
      autoSyncStartedMs_ = millis();
      autoSyncClientConnected_ = false;
      Serial.println("[welcome] started AP for phone pairing");
    }
  }
}

void App::selectWelcomeConnectItem(uint32_t nowMs) {
  // 0 = Wi-Fi name (info), 1 = IP (info), 2 = separator, 3 = Connect, 4 = Skip
  if (settingsSelectedIndex_ <= 2) {
    // Info rows — treat tap as "Skip" so the UI never feels frozen.
    // Previously these did nothing, leaving the user stuck without
    // an obvious touch-based exit path.
    settingsSelectedIndex_ = 4;
  }
  if (settingsSelectedIndex_ == 4) {
    // User chose to skip — shut down AP if it was started for wizard
    if (autoSyncActive_ && !autoSyncClientConnected_) {
      companionSync_.end();
      autoSyncActive_ = false;
      Serial.println("[welcome] user skipped phone connect, AP stopped");
    }
  } else {
    // User chose "Connect" (index 3) — keep AP alive
    Serial.println("[welcome] user confirmed phone connect, AP stays active");
  }
  finishWelcomeWizard(nowMs);
}

void App::finishWelcomeWizard(uint32_t nowMs) {
  (void)nowMs;
  preferences_.putBool(kPrefSetupDone, true);
  // Start tutorial after wizard
  openTutorialStep1();
}

// ─── Post-wizard tutorial ────────────────────────────────────────────────────

void App::openTutorialStep1() {
  menuScreen_ = MenuScreen::TutorialStep1;
  renderTutorialStep();
}

void App::openTutorialStep2() {
  menuScreen_ = MenuScreen::TutorialStep2;
  renderTutorialStep();
}

void App::openTutorialStep3() {
  menuScreen_ = MenuScreen::TutorialStep3;
  renderTutorialStep();
}

void App::openTutorialStep4() {
  menuScreen_ = MenuScreen::TutorialStep4;
  renderTutorialStep();
}

void App::openTutorialStep5() {
  menuScreen_ = MenuScreen::TutorialStep5;
  renderTutorialStep();
}

void App::renderTutorialStep() {
  const char *title = "";
  const char *desc = "";
  int step = 0;

  switch (menuScreen_) {
    case MenuScreen::TutorialStep1:
      title = "RSVP";
      desc = polish("Slowa jedno po drugim. Litera ORP kieruje wzrok.",
                    "Words one at a time. ORP letter guides your eye.");
      step = 1;
      break;
    case MenuScreen::TutorialStep2:
      title = polish("Tempo", "Speed");
      desc = polish("Przytrzymaj + gora/dol = zmiana predkosci.",
                    "Hold + up/down = change speed.");
      step = 2;
      break;
    case MenuScreen::TutorialStep3:
      title = polish("Pauza", "Pause");
      desc = polish("Dotknij ekranu by pauzowac/wznowic.",
                    "Tap screen to pause/resume.");
      step = 3;
      break;
    case MenuScreen::TutorialStep4:
      title = "Menu";
      desc = polish("Przycisk z boku otwiera menu.",
                    "Side button opens the menu.");
      step = 4;
      break;
    case MenuScreen::TutorialStep5:
      title = polish("Pomoc ?", "Help ?");
      desc = polish("W menu nacisnij boczny przycisk = opis.",
                    "In menu press side button = description.");
      step = 5;
      break;
    default:
      return;
  }

  String progress = String(step) + "/5";
  display_.renderStatus(title, desc, progress);
}

void App::handleTutorialTap(uint32_t nowMs) {
  switch (menuScreen_) {
    case MenuScreen::TutorialStep1: openTutorialStep2(); break;
    case MenuScreen::TutorialStep2: openTutorialStep3(); break;
    case MenuScreen::TutorialStep3: openTutorialStep4(); break;
    case MenuScreen::TutorialStep4: openTutorialStep5(); break;
    case MenuScreen::TutorialStep5: finishTutorial(nowMs); break;
    default: break;
  }
}

void App::finishTutorial(uint32_t nowMs) {
  tutorialCompleted_ = true;
  preferences_.putBool(kPrefTutorialDone, true);
  menuScreen_ = MenuScreen::Main;
  menuSelectedIndex_ = 0;
  settingsSelectedIndex_ = kSettingsBackIndex;
  setState((touchPlayHeld_ || playLocked_ || pauseAtSentenceEndRequested_)
               ? AppState::Playing
               : AppState::Paused,
           nowMs);
}

OtaUpdater::Config App::preferredOtaConfig() {
  OtaUpdater::Config otaConfig;
  otaUpdater_.loadConfig(otaConfig);

  if (preferences_.isKey(kPrefWifiSsid)) {
    otaConfig.wifiSsid = preferences_.getString(kPrefWifiSsid, "");
  }
  if (preferences_.isKey(kPrefWifiPass)) {
    otaConfig.wifiPassword = preferences_.getString(kPrefWifiPass, "");
  }
  if (preferences_.isKey(kPrefOtaAuto)) {
    otaConfig.autoCheck = preferences_.getBool(kPrefOtaAuto, otaConfig.autoCheck);
  }
  if (preferences_.isKey(kPrefOtaOwner)) {
    otaConfig.githubOwner = preferences_.getString(kPrefOtaOwner, "");
  }

  return otaConfig;
}

String App::configuredWifiSsid() {
  String ssid = preferences_.getString(kPrefWifiSsid, "");
  if (ssid.isEmpty()) {
    OtaUpdater::Config otaConfig;
    otaUpdater_.loadConfig(otaConfig);
    ssid = otaConfig.wifiSsid;
  }
  ssid.trim();
  return ssid;
}

bool App::otaAutoCheckEnabled() {
  if (preferences_.isKey(kPrefOtaAuto)) {
    return preferences_.getBool(kPrefOtaAuto, false);
  }

  OtaUpdater::Config otaConfig;
  otaUpdater_.loadConfig(otaConfig);
  return otaConfig.autoCheck;
}

void App::maybeAutoCheckForUpdates(uint32_t nowMs) {
  (void)nowMs;
  OtaUpdater::Config otaConfig = preferredOtaConfig();
  if (!otaConfig.autoCheck || !otaUpdater_.isConfigured(otaConfig)) {
    return;
  }

  Serial.println("[ota] auto-check enabled");
  startBackgroundOtaCheck(otaConfig);
}

bool App::startBackgroundOtaCheck(const OtaUpdater::Config &config) {
  if (otaCheckInProgress_) {
    Serial.println("[ota] background check already running");
    return false;
  }

  if (otaCheckQueue_ == nullptr) {
    otaCheckQueue_ = xQueueCreate(1, sizeof(OtaCheckResult));
    if (otaCheckQueue_ == nullptr) {
      Serial.println("[ota] could not create result queue");
      return false;
    }
  }
  xQueueReset(otaCheckQueue_);

  OtaCheckTaskParams *params = new OtaCheckTaskParams();
  if (params == nullptr) {
    Serial.println("[ota] could not allocate task params");
    return false;
  }
  params->config = config;
  params->resultQueue = otaCheckQueue_;

  otaCheckInProgress_ = true;
  BaseType_t created = xTaskCreatePinnedToCore(otaCheckTask, "ota_check",
                                               kOtaCheckTaskStackBytes, params, 1, nullptr, 0);
  if (created != pdPASS) {
    Serial.printf("[ota] background task create failed: %ld\n", static_cast<long>(created));
    otaCheckInProgress_ = false;
    delete params;
    return false;
  }

  Serial.println("[ota] background check started");
  return true;
}

void App::otaCheckTask(void *params) {
  OtaCheckTaskParams *taskParams = static_cast<OtaCheckTaskParams *>(params);
  if (taskParams == nullptr) {
    vTaskDelete(nullptr);
    return;
  }

  OtaCheckResult queuedResult;

  const OtaUpdater::Result result =
      OtaUpdater().checkOnly(taskParams->config, nullptr, nullptr);
  queuedResult.code = result.code;
  copyOtaLabel(queuedResult.currentVersion, sizeof(queuedResult.currentVersion),
               result.currentVersion);
  copyOtaLabel(queuedResult.latestVersion, sizeof(queuedResult.latestVersion),
               result.latestVersion);
  copyOtaLabel(queuedResult.summary, sizeof(queuedResult.summary), result.summary);
  copyOtaLabel(queuedResult.detail, sizeof(queuedResult.detail), result.detail);

  if (taskParams->resultQueue != nullptr) {
    xQueueOverwrite(taskParams->resultQueue, &queuedResult);
  }

  delete taskParams;
  vTaskDelete(nullptr);
}

void App::pollOtaCheckResult(uint32_t nowMs) {
  (void)nowMs;
  if (otaCheckQueue_ == nullptr) {
    return;
  }

  OtaCheckResult result;
  while (xQueueReceive(otaCheckQueue_, &result, 0) == pdTRUE) {
    otaCheckInProgress_ = false;
    Serial.printf("[ota] background result code=%u current=%s latest=%s summary=%s detail=%s\n",
                  static_cast<unsigned int>(result.code), result.currentVersion,
                  result.latestVersion, result.summary, result.detail);

    if (result.code == OtaUpdater::ResultCode::UpdateAvailable) {
      pendingUpdateCurrentVersion_ = String(result.currentVersion);
      pendingUpdateNewVersion_ = String(result.latestVersion);
      otaUpdatePromptPending_ = true;
    }
  }
}

bool App::updateConfirmCanOpen() const {
  return otaUpdatePromptPending_ && !pendingBootBookLoad_ && state_ == AppState::Paused;
}

void App::maybeOpenUpdateConfirm(uint32_t nowMs) {
  if (!updateConfirmCanOpen()) {
    return;
  }

  otaUpdatePromptPending_ = false;
  setState(AppState::Menu, nowMs);
  openUpdateConfirm();
}

bool App::blockNetworkActionForOtaCheck(const String &title, uint32_t nowMs) {
  pollOtaCheckResult(nowMs);
  if (!otaCheckInProgress_) {
    return false;
  }

  display_.renderStatus(title, "OTA check running", "Try again soon");
  delay(1200);
  renderMenu();
  return true;
}

void App::runFirmwareUpdate(const OtaUpdater::Config &config, bool automatic, uint32_t nowMs) {
  if (blockNetworkActionForOtaCheck("OTA", nowMs)) {
    return;
  }

  if (!automatic) {
    otaUpdatePromptPending_ = false;
  }

  if (!otaUpdater_.isConfigured(config)) {
    if (!automatic) {
      display_.renderStatus("OTA", tr(TrKey::WifiNotSet),
                            tr(TrKey::SettingsWifi));
      delay(1600);
      if (state_ == AppState::Menu && isSettingsListScreen()) {
        rebuildSettingsMenuItems();
        renderSettings();
      } else {
        menuScreen_ = MenuScreen::Main;
        setState(AppState::Paused, nowMs);
      }
    }
    return;
  }

  saveReadingPosition(true);
  const OtaUpdater::Result result =
      otaUpdater_.checkAndInstall(config, &App::handleStorageStatus, this);

  Serial.printf("[ota] code=%u current=%s latest=%s summary=%s detail=%s\n",
                static_cast<unsigned int>(result.code), result.currentVersion.c_str(),
                result.latestVersion.c_str(), result.summary.c_str(), result.detail.c_str());

  if (result.rebootRequired) {
    display_.renderStatus("OTA", tr(TrKey::Restarting), result.latestVersion);
    delay(300);
    ESP.restart();
    return;
  }

  if (automatic) {
    return;
  }

  const String line2 = result.detail.isEmpty() ? result.currentVersion : result.detail;
  display_.renderStatus("OTA", result.summary, line2);
  delay(1600);
  if (state_ == AppState::Menu && isSettingsListScreen()) {
    rebuildSettingsMenuItems();
    renderSettings();
  } else {
    menuScreen_ = MenuScreen::Main;
    setState(AppState::Paused, nowMs);
  }
}

void App::runRssFeedCheck(uint32_t nowMs) {
  (void)nowMs;
  if (blockNetworkActionForOtaCheck("RSS", nowMs)) {
    return;
  }

  saveReadingPosition(true);

  display_.renderStatus("RSS", tr2(TrKey2::CheckingFeeds), tr(TrKey::PleaseWait));
  const RssFeedManager::Result result =
      rssFeedManager_.checkFeeds(preferredOtaConfig(), preferences_, &App::handleStorageStatus, this);

  Serial.printf("[rss] feeds=%u saved=%u skipped=%u summary=%s detail=%s\n",
                static_cast<unsigned int>(result.feedsChecked),
                static_cast<unsigned int>(result.articlesSaved),
                static_cast<unsigned int>(result.articlesSkipped), result.summary.c_str(),
                result.detail.c_str());

  storage_.refreshBooks(false);
  display_.renderStatus("RSS", result.summary, result.detail);
  delay(1800);
  renderMainMenu();
}

String App::pacingDelayLabel(uint16_t delayMs) const { return String(delayMs) + " ms"; }

String App::firmwareUpdateMenuLabel() const {
  return tr(TrKey::FirmwareUpdate);
}

String App::uiText(UiText key) const { return Localization::text(uiLanguage_, key); }

String App::themeModeLabel() const {
  if (nightMode_) {
    return uiText(UiText::Night);
  }
  return darkMode_ ? uiText(UiText::Dark) : uiText(UiText::Light);
}

String App::phantomWordsLabel() const {
  return phantomWordsEnabled_ ? uiText(UiText::On) : uiText(UiText::Off);
}

String App::focusHighlightLabel() const {
  return typographyConfig_.focusHighlight ? uiText(UiText::On) : uiText(UiText::Off);
}

String App::focusColorLabel() const {
  switch (display_.focusColorIndex()) {
    case 0: return polish("Czerwony", "Red");
    case 1: return polish("Niebieski", "Blue");
    case 2: return polish("Zielony", "Green");
    case 3: return polish("Zolty", "Yellow");
    case 4: return polish("Pomaranczowy", "Orange");
    case 5: return polish("Fioletowy", "Purple");
    default: return polish("Czerwony", "Red");
  }
}

void App::cycleFocusColor(uint32_t nowMs) {
  uint8_t next = display_.focusColorIndex() + 1;
  if (next >= 6) next = 0;
  display_.setFocusColorIndex(next);
  preferences_.putUChar(kPrefFocusColorIndex, next);
  applyDisplayPreferences(nowMs);
}

String App::uiLanguageLabel() const { return Localization::languageName(uiLanguage_); }

String App::readerModeLabel() const {
  switch (readerMode_) {
    case ReaderMode::Scroll:
      return uiText(UiText::ScrollMode);
    case ReaderMode::Rsvp:
    default:
      return uiText(UiText::RsvpMode);
  }
}

void App::showScrollSettingsPreview() {
  // Build sample words for preview
  static const char *const kScrollPreviewText[] = {
      "The", "quick", "brown", "fox", "jumps", "over",
      "the", "lazy", "dog.", "Reading", "is", "a",
      "wonderful", "way", "to", "explore", "new", "worlds.",
      "Every", "book", "opens", "a", "door", "to",
      "knowledge", "and", "imagination."
  };
  constexpr size_t kScrollPreviewWordCount = sizeof(kScrollPreviewText) / sizeof(kScrollPreviewText[0]);

  std::vector<DisplayManager::ContextWord> previewWords;
  previewWords.reserve(kScrollPreviewWordCount);
  for (size_t i = 0; i < kScrollPreviewWordCount; ++i) {
    DisplayManager::ContextWord w;
    w.text = kScrollPreviewText[i];
    w.paragraphStart = (i == 0 || i == 9 || i == 18);
    w.current = (i == 5);  // highlight "over" as current word
    previewWords.push_back(w);
  }

  // Apply current scroll display settings before rendering
  display_.setScrollFontSize(scrollFontSize_);
  display_.setScrollLineSpacing(scrollLineSpacing_);
  display_.setScrollMargin(scrollMargin_);

  // Build a label showing current setting values
  String overlayText = "Font:" + scrollFontSizeLabel() +
                       " Spacing:" + scrollLineSpacingLabel() +
                       " Margin:" + scrollMarginLabel();

  DisplayManager::ReaderChrome chrome;
  chrome.showBattery = false;
  chrome.showChapter = false;
  chrome.showProgress = false;
  chrome.showPreviousSentenceHint = false;
  chrome.showSavePointButton = false;

  // Force a fresh render by using a unique content token
  static uint32_t previewToken = 90000;
  ++previewToken;

  display_.renderScrollView(previewWords, previewToken,
                            0, 5, 0,
                            "", 0, overlayText,
                            "", chrome);

  // Show preview for 1500ms then return to menu
  delay(1500);
  rebuildSettingsMenuItems();
  renderSettings();
}

String App::scrollFontSizeLabel() const {
  return String(scrollFontSize_);
}

String App::scrollLineSpacingLabel() const {
  static const char *const labels[] = {"Compact", "Normal", "Relaxed"};
  const uint8_t idx = scrollLineSpacing_ <= 2 ? scrollLineSpacing_ : 1;
  return String(labels[idx]);
}

String App::scrollMarginLabel() const {
  static const char *const labels[] = {"Narrow", "Normal", "Wide"};
  const uint8_t idx = scrollMargin_ <= 2 ? scrollMargin_ : 1;
  return String(labels[idx]);
}

String App::pauseModeLabel() const {
  return pauseMode_ == PauseMode::Instant ? tr(TrKey::Instant)
                                          : tr(TrKey::Sentence);
}

String App::handednessLabel() const {
  return handednessMode_ == HandednessMode::Left ? tr(TrKey::LeftHand)
                                                 : tr(TrKey::RightHand);
}

String App::readerFontSizeLabel() const {
  uint8_t levelIndex = readerFontSizeIndex_;
  if (levelIndex >= kReaderFontSizeCount) {
    levelIndex = 0;
  }

  switch (levelIndex) {
    case 0:
      return uiText(UiText::Large);
    case 1:
      return uiText(UiText::Medium);
    case 2:
    default:
      return uiText(UiText::Small);
  }
}

String App::readerTypefaceLabel() const {
  switch (typographyConfig_.typeface) {
    case DisplayManager::ReaderTypeface::AtkinsonHyperlegible:
      return "Atkinson";
    case DisplayManager::ReaderTypeface::OpenDyslexic:
      return "OpenDyslexic";
    case DisplayManager::ReaderTypeface::Standard:
    default:
      return uiText(UiText::Standard);
  }
}

String App::typographyTuningLabel() const {
  switch (typographyTuningSelectedIndex_) {
    case TypographyTuningBack:
      return uiText(UiText::Back);
    case TypographyTuningFontSize:
      return uiText(UiText::FontSize);
    case TypographyTuningTypeface:
      return uiText(UiText::Typeface);
    case TypographyTuningPhantomWords:
      return uiText(UiText::PhantomWords);
    case TypographyTuningFocusHighlight:
      return uiText(UiText::RedHighlight);
    case TypographyTuningTracking:
      return uiText(UiText::Tracking);
    case TypographyTuningAnchor:
      return uiText(UiText::Anchor);
    case TypographyTuningGuideWidth:
      return uiText(UiText::GuideWidth);
    case TypographyTuningGuideGap:
      return uiText(UiText::GuideGap);
    case TypographyTuningReset:
      return uiText(UiText::Reset);
    default:
      return uiText(UiText::Typography);
  }
}

String App::typographyTuningValueLabel() const {
  switch (typographyTuningSelectedIndex_) {
    case TypographyTuningBack:
      return uiText(UiText::TapToExit);
    case TypographyTuningFontSize:
      return readerFontSizeLabel();
    case TypographyTuningTypeface:
      return readerTypefaceLabel();
    case TypographyTuningPhantomWords:
      return phantomWordsLabel();
    case TypographyTuningFocusHighlight:
      return focusHighlightLabel();
    case TypographyTuningTracking:
      return String(typographyConfig_.trackingPx >= 0 ? "+" : "") +
             String(static_cast<int>(typographyConfig_.trackingPx)) + " px";
    case TypographyTuningAnchor:
      return String(static_cast<unsigned int>(effectiveAnchorPercent())) + "%";
    case TypographyTuningGuideWidth:
      return String(static_cast<unsigned int>(typographyConfig_.guideHalfWidth)) + " px";
    case TypographyTuningGuideGap:
      return String(static_cast<unsigned int>(typographyConfig_.guideGap)) + " px";
    case TypographyTuningReset:
      return uiText(UiText::TapToReset);
    default:
      return "";
  }
}

void App::openBookPicker(bool articlesOnly) {
  storage_.refreshBooks();
  bookMenuItems_.clear();
  bookPickerBookIndices_.clear();
  bookMenuItems_.push_back({uiText(UiText::Back), ""});

  const size_t count = storage_.bookCount();
  std::vector<size_t> sortedBookIndices;
  sortedBookIndices.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    // Library shows all items (books + articles together)
    (void)articlesOnly;
    sortedBookIndices.push_back(i);
  }

  std::stable_sort(sortedBookIndices.begin(), sortedBookIndices.end(),
                   [this](size_t leftIndex, size_t rightIndex) {
                     const bool leftCurrent =
                         usingStorageBook_ && leftIndex == currentBookIndex_;
                     const bool rightCurrent =
                         usingStorageBook_ && rightIndex == currentBookIndex_;
                     if (leftCurrent != rightCurrent) {
                       return leftCurrent;
                     }

                     const uint32_t leftRecent =
                         bookRecentSequence(storage_.bookPath(leftIndex));
                     const uint32_t rightRecent =
                         bookRecentSequence(storage_.bookPath(rightIndex));
                     const bool leftHasRecent = leftRecent > 0;
                     const bool rightHasRecent = rightRecent > 0;
                     if (leftHasRecent != rightHasRecent) {
                       return leftHasRecent;
                     }
                     if (leftRecent != rightRecent) {
                       return leftRecent > rightRecent;
                     }

                     return false;
                   });

  for (size_t bookIndex : sortedBookIndices) {
    bookPickerBookIndices_.push_back(bookIndex);
    bookMenuItems_.push_back(libraryItemForBook(bookIndex));
  }

  if (sortedBookIndices.empty()) {
    Serial.printf("[book-picker] No SD %s available\n", articlesOnly ? "articles" : "books");
  }

  menuScreen_ = MenuScreen::BookPicker;
  bookPickerSelectedIndex_ = kBookPickerBackIndex;
  if (usingStorageBook_) {
    for (size_t row = 0; row < bookPickerBookIndices_.size(); ++row) {
      if (bookPickerBookIndices_[row] == currentBookIndex_) {
        bookPickerSelectedIndex_ = row + 1;
        break;
      }
    }
  }
  renderBookPicker();
}

void App::selectBookPickerItem(uint32_t nowMs) {
  if (bookPickerSelectedIndex_ == kBookPickerBackIndex || bookMenuItems_.size() <= 1) {
    menuScreen_ = MenuScreen::Main;
    menuSelectedIndex_ = MenuLibrary;
    renderMainMenu();
    return;
  }

  const size_t rowIndex = bookPickerSelectedIndex_ - 1;
  if (rowIndex >= bookPickerBookIndices_.size()) {
    renderBookPicker();
    return;
  }

  const size_t bookIndex = bookPickerBookIndices_[rowIndex];
  openBookDetails(bookIndex, nowMs);
}

void App::openChapterPicker() {
  chapterMenuItems_.clear();
  chapterMenuItems_.push_back(uiText(UiText::Back));

  if (chapterMarkers_.empty()) {
    chapterMenuItems_.push_back(uiText(UiText::StartOfBook));
    chapterPickerSelectedIndex_ = kChapterPickerFallbackIndex;
    Serial.println("[chapter-picker] No chapter markers found; showing start fallback");
  } else {
    for (size_t i = 0; i < chapterMarkers_.size(); ++i) {
      chapterMenuItems_.push_back(chapterMenuLabel(i));
    }

    size_t selectedChapter = 0;
    const size_t currentWordIndex = reader_.currentIndex();
    for (size_t i = 0; i < chapterMarkers_.size(); ++i) {
      if (chapterMarkers_[i].wordIndex <= currentWordIndex) {
        selectedChapter = i;
      }
    }
    chapterPickerSelectedIndex_ = selectedChapter + 1;
  }

  chapterMenuItems_.push_back(uiText(UiText::RestartBook));

  menuScreen_ = MenuScreen::ChapterPicker;
  renderChapterPicker();
}

void App::selectChapterPickerItem(uint32_t nowMs) {
  if (chapterPickerSelectedIndex_ == kChapterPickerBackIndex || chapterMenuItems_.size() <= 1) {
    // Return to book details if we came from there, otherwise main menu
    if (bookDetailsMenuItems_.size() > 0) {
      menuScreen_ = MenuScreen::BookDetails;
      renderBookDetails();
    } else {
      menuScreen_ = MenuScreen::Main;
      renderMainMenu();
    }
    return;
  }

  const size_t restartIndex = chapterMenuItems_.size() - 1;
  if (chapterPickerSelectedIndex_ == restartIndex) {
    openRestartConfirm();
    return;
  }

  if (chapterMarkers_.empty()) {
    reader_.seekTo(0);
    menuScreen_ = MenuScreen::Main;
    setState(AppState::Paused, nowMs);
    saveReadingPosition(true);
    Serial.println("[chapter-picker] jumped to start of book");
    return;
  }

  const size_t chapterIndex = chapterPickerSelectedIndex_ - 1;
  if (chapterIndex >= chapterMarkers_.size()) {
    renderChapterPicker();
    return;
  }

  // Save current position as auto save point before jumping
  saveReadingPosition(true);
  reader_.seekTo(chapterMarkers_[chapterIndex].wordIndex);
  menuScreen_ = MenuScreen::Main;
  setState(AppState::Paused, nowMs);
  saveReadingPosition(true);
  Serial.printf("[chapter-picker] jumped to %s at word %u\n",
                chapterMarkers_[chapterIndex].title.c_str(),
                static_cast<unsigned int>(chapterMarkers_[chapterIndex].wordIndex));
}

// ─── Book Details ────────────────────────────────────────────────────────────

void App::openBookDetails(size_t bookIndex, uint32_t nowMs) {
  (void)nowMs;
  bookDetailsBookIndex_ = bookIndex;
  bookDetailsMenuItems_.clear();
  bookDetailsMenuItems_.push_back(uiText(UiText::Back));

  const String title = storage_.bookDisplayName(bookIndex);
  const String author = storage_.bookAuthorName(bookIndex);
  uint8_t percent = 0;
  bookProgressPercent(bookIndex, percent);

  bookDetailsMenuItems_.push_back(title + (author.isEmpty() ? "" : " - " + author));
  bookDetailsMenuItems_.push_back(String(static_cast<unsigned int>(percent)) + "% " +
                                  polish("ukonczone", "complete"));
  bookDetailsMenuItems_.push_back(polish("Czytaj od miejsca", "Read from place"));
  bookDetailsMenuItems_.push_back(uiText(UiText::Chapters));
  bookDetailsMenuItems_.push_back(uiText(UiText::RestartBook));

  bookDetailsSelectedIndex_ = 3;  // "Czytaj od miejsca"
  menuScreen_ = MenuScreen::BookDetails;
  renderBookDetails();
}

void App::selectBookDetailsItem(uint32_t nowMs) {
  switch (bookDetailsSelectedIndex_) {
    case 0:  // Back
      menuScreen_ = MenuScreen::BookPicker;
      renderBookPicker();
      return;
    case 1:  // Title (info only)
    case 2:  // Progress (info only)
      return;
    case 3: {  // Czytaj od miejsca
      saveReadingPosition(true);
      if (!loadBookAtIndex(bookDetailsBookIndex_, nowMs, true, true, true, true)) {
        display_.renderStatus(polish("Blad", "Error"),
                              storage_.bookDisplayName(bookDetailsBookIndex_), "");
        delay(1400);
        renderBookDetails();
        return;
      }
      bookDetailsMenuItems_.clear();  // signal we left book details
      menuScreen_ = MenuScreen::Main;
      setState(AppState::Paused, nowMs);
      return;
    }
    case 4: {  // Rozdzialy
      // Load the book so we have chapter markers available
      saveReadingPosition(true);
      if (!loadBookAtIndex(bookDetailsBookIndex_, nowMs, true, true, true, true)) {
        display_.renderStatus(polish("Blad", "Error"),
                              storage_.bookDisplayName(bookDetailsBookIndex_), "");
        delay(1400);
        renderBookDetails();
        return;
      }
      openChapterPicker();
      return;
    }
    case 5:  // Restart
      saveReadingPosition(true);
      if (!loadBookAtIndex(bookDetailsBookIndex_, nowMs, true, true, true, true)) {
        display_.renderStatus(polish("Blad", "Error"),
                              storage_.bookDisplayName(bookDetailsBookIndex_), "");
        delay(1400);
        renderBookDetails();
        return;
      }
      openRestartConfirm();
      return;
    default:
      return;
  }
}

void App::renderBookDetails() {
  display_.renderMenu(bookDetailsMenuItems_, bookDetailsSelectedIndex_);
}

// ─── Save Points ─────────────────────────────────────────────────────────────

void App::loadSavePoints() {
  savePoints_.clear();
  // Save points stored in preferences as: sp_count, sp_N_name, sp_N_book,
  // sp_N_title, sp_N_word, sp_N_pct
  const size_t count = preferences_.getUChar("sp_count", 0);
  for (size_t i = 0; i < count && i < kMaxSavePoints; ++i) {
    SavePoint sp;
    const String prefix = "sp_" + String(static_cast<unsigned int>(i)) + "_";
    sp.name = preferences_.getString((prefix + "name").c_str(), "");
    sp.bookPath = preferences_.getString((prefix + "book").c_str(), "");
    sp.bookTitle = preferences_.getString((prefix + "titl").c_str(), "");
    sp.wordIndex = preferences_.getUInt((prefix + "word").c_str(), 0);
    sp.progressPercent = preferences_.getUChar((prefix + "pct").c_str(), 0);
    if (!sp.bookPath.isEmpty()) {
      savePoints_.push_back(sp);
    }
  }
}

void App::persistSavePoints() {
  const size_t count = std::min(savePoints_.size(), kMaxSavePoints);
  preferences_.putUChar("sp_count", static_cast<uint8_t>(count));
  for (size_t i = 0; i < count; ++i) {
    const String prefix = "sp_" + String(static_cast<unsigned int>(i)) + "_";
    preferences_.putString((prefix + "name").c_str(), savePoints_[i].name);
    preferences_.putString((prefix + "book").c_str(), savePoints_[i].bookPath);
    preferences_.putString((prefix + "titl").c_str(), savePoints_[i].bookTitle);
    preferences_.putUInt((prefix + "word").c_str(), static_cast<uint32_t>(savePoints_[i].wordIndex));
    preferences_.putUChar((prefix + "pct").c_str(), savePoints_[i].progressPercent);
  }
  // Clear any leftover entries beyond current count
  for (size_t i = count; i < kMaxSavePoints; ++i) {
    const String prefix = "sp_" + String(static_cast<unsigned int>(i)) + "_";
    preferences_.remove((prefix + "name").c_str());
    preferences_.remove((prefix + "book").c_str());
    preferences_.remove((prefix + "titl").c_str());
    preferences_.remove((prefix + "word").c_str());
    preferences_.remove((prefix + "pct").c_str());
  }
}

void App::createSavePoint(uint32_t nowMs) {
  (void)nowMs;
  if (!usingStorageBook_ || currentBookPath_.isEmpty()) {
    return;
  }

  SavePoint sp;
  sp.bookPath = currentBookPath_;
  sp.bookTitle = currentBookTitle_;
  sp.wordIndex = reader_.currentIndex();
  sp.progressPercent = readingProgressPercent();

  // Build meaningful default name: chapter name + percentage
  const String chapter = currentChapterLabel();
  if (chapter.isEmpty() || chapter == currentBookTitle_) {
    sp.name = currentBookTitle_ + " " + String(static_cast<unsigned int>(sp.progressPercent)) + "%";
  } else {
    sp.name = chapter + " " + String(static_cast<unsigned int>(sp.progressPercent)) + "%";
  }

  // Add to front (newest first)
  savePoints_.insert(savePoints_.begin(), sp);
  if (savePoints_.size() > kMaxSavePoints) {
    savePoints_.pop_back();
  }
  persistSavePoints();
  Serial.printf("[save-point] created: %s word=%u\n", sp.name.c_str(),
                static_cast<unsigned int>(sp.wordIndex));
}

void App::deleteSavePoint(size_t index) {
  if (index >= savePoints_.size()) return;
  savePoints_.erase(savePoints_.begin() + static_cast<int>(index));
  persistSavePoints();
}

void App::openSavePointsList() {
  loadSavePoints();
  savePointMenuItems_.clear();
  savePointMenuItems_.push_back(uiText(UiText::Back));
  savePointMenuItems_.push_back(polish("+ Dodaj punkt zapisu", "+ Add save point"));

  for (size_t i = 0; i < savePoints_.size(); ++i) {
    const auto &sp = savePoints_[i];
    String label = sp.name.isEmpty() ? sp.bookTitle : sp.name;
    savePointMenuItems_.push_back(label);
    savePointMenuItems_.push_back(polish("Usun: ", "Delete: ") + label);
  }

  savePointSelectedIndex_ = savePoints_.empty() ? 1 : 2;
  menuScreen_ = MenuScreen::SavePointsList;
  renderSavePointsList();
}

void App::selectSavePointItem(uint32_t nowMs) {
  if (savePointSelectedIndex_ == 0) {
    // Back
    menuScreen_ = MenuScreen::Main;
    menuSelectedIndex_ = MenuSavePoints;
    renderMainMenu();
    return;
  }

  if (savePointSelectedIndex_ == 1) {
    // Add save point — open name entry
    if (!usingStorageBook_ || currentBookPath_.isEmpty()) {
      display_.renderStatus(uiText(UiText::SavePoints),
                            polish("Najpierw otworz ksiazke", "Open a book first"), "");
      delay(1400);
      renderSavePointsList();
      return;
    }
    // Generate default name
    const String chapter = currentChapterLabel();
    String defaultName;
    if (chapter.isEmpty() || chapter == currentBookTitle_) {
      defaultName = currentBookTitle_.substring(0, 16) + " " +
                    String(static_cast<unsigned int>(readingProgressPercent())) + "%";
    } else {
      defaultName = chapter.substring(0, 20) + " " +
                    String(static_cast<unsigned int>(readingProgressPercent())) + "%";
    }
    openTextEntry(TextEntryPurpose::SavePointName,
                  polish("Nazwij zakladke", "Name bookmark"),
                  polish("Wpisz nazwe:", "Enter name:"),
                  "", defaultName, "", false, 30,
                  MenuScreen::SavePointsList);
    return;
  }

  // Items after index 1: alternating save point / delete
  const size_t itemOffset = savePointSelectedIndex_ - 2;
  const size_t spIndex = itemOffset / 2;
  const bool isDeleteButton = (itemOffset % 2) == 1;

  if (spIndex >= savePoints_.size()) {
    return;
  }

  if (isDeleteButton) {
    // Delete — confirm
    deleteSavePoint(spIndex);
    display_.renderStatus(uiText(UiText::SavePoints),
                          polish("Usunieto", "Deleted"), "");
    delay(800);
    openSavePointsList();
    return;
  }

  // Navigate to save point
  const SavePoint &sp = savePoints_[spIndex];
  saveReadingPosition(true);

  const int bookIdx = findBookIndexByPath(sp.bookPath);
  if (bookIdx < 0) {
    display_.renderStatus(polish("Blad", "Error"),
                          polish("Ksiazka nie znaleziona", "Book not found"), sp.bookTitle);
    delay(1400);
    renderSavePointsList();
    return;
  }

  if (!loadBookAtIndex(static_cast<size_t>(bookIdx), nowMs, true, true, true, true)) {
    display_.renderStatus(polish("Blad", "Error"),
                          polish("Nie mozna otworzyc", "Cannot open"), sp.bookTitle);
    delay(1400);
    renderSavePointsList();
    return;
  }

  reader_.seekTo(sp.wordIndex);
  menuScreen_ = MenuScreen::Main;
  setState(AppState::Paused, nowMs);
  saveReadingPosition(true);
  Serial.printf("[save-point] jumped to %s word=%u\n", sp.name.c_str(),
                static_cast<unsigned int>(sp.wordIndex));
}

void App::renderSavePointsList() {
  display_.renderMenu(savePointMenuItems_, savePointSelectedIndex_);
}

// ─── Plugins ─────────────────────────────────────────────────────────────────

// Indeksy w pluginsMenuItems_:
//   0: Back
//   1: Focus Timer  (built-in, zawsze zainstalowany)
//   2: "Zainstaluj RSS" lub "Usun RSS"  (zależnie od stanu)
//   3: separator "---"
//   4: Biblioteka funkcji (placeholder)

static constexpr size_t kPluginItemBackIndex      = 0;
static constexpr size_t kPluginItemTimerIndex     = 1;
static constexpr size_t kPluginItemRssIndex       = 2;
static constexpr size_t kPluginItemSeparatorIndex = 3;
static constexpr size_t kPluginItemLibraryIndex   = 4;

void App::openPluginsList() {
  pluginsMenuItems_.clear();

  // [0] Back
  pluginsMenuItems_.push_back(uiText(UiText::Back));

  // [1] Focus Timer — wbudowany, zawsze zainstalowany
  {
    const bool installed = pluginManager_.isInstalled(PluginManager::PluginId::Timer);
    String label = tr2(TrKey2::FocusTimer);
    if (installed) {
      label += " " + String(tr2(TrKey2::PluginInstalled));
    }
    pluginsMenuItems_.push_back(label);
  }

  // [2] RSS Feeds — zainstaluj lub usuń
  {
    const bool installed = pluginManager_.isInstalled(PluginManager::PluginId::Rss);
    String label;
    if (installed) {
      label = String(tr2(TrKey2::PluginRemove)) + tr2(TrKey2::RssFeeds);
    } else {
      label = String(tr2(TrKey2::PluginInstall)) + tr2(TrKey2::RssFeeds);
    }
    pluginsMenuItems_.push_back(label);
  }

  // [3] Separator
  pluginsMenuItems_.push_back("---");

  // [4] Biblioteka funkcji
  pluginsMenuItems_.push_back(tr2(TrKey2::PluginLibrary));

  pluginsSelectedIndex_ = kPluginItemTimerIndex;
  menuScreen_ = MenuScreen::PluginsList;
  renderPluginsList();
}

void App::selectPluginsItem(uint32_t nowMs) {
  if (pluginsSelectedIndex_ == kPluginItemBackIndex) {
    menuScreen_ = MenuScreen::Main;
    menuSelectedIndex_ = MenuPlugins;
    renderMainMenu();
    return;
  }

  if (pluginsSelectedIndex_ == kPluginItemTimerIndex) {
    // Focus Timer jest wbudowany — otwieramy sesję timera
    if (pluginManager_.isInstalled(PluginManager::PluginId::Timer)) {
      openFocusTimer();
    } else {
      display_.renderStatus(uiText(UiText::Plugins),
                            tr2(TrKey2::PluginBuiltIn),
                            tr2(TrKey2::PluginCannotRemove));
      delay(1400);
      renderPluginsList();
    }
    return;
  }

  if (pluginsSelectedIndex_ == kPluginItemRssIndex) {
    const bool installed = pluginManager_.isInstalled(PluginManager::PluginId::Rss);
    if (installed) {
      runPluginRemove(PluginManager::PluginId::Rss, nowMs);
    } else {
      runPluginInstall(PluginManager::PluginId::Rss, nowMs);
    }
    return;
  }

  if (pluginsSelectedIndex_ == kPluginItemSeparatorIndex) {
    // Separator — ignoruj
    return;
  }

  if (pluginsSelectedIndex_ == kPluginItemLibraryIndex) {
    display_.renderStatus(uiText(UiText::Plugins),
                          tr2(TrKey2::PluginLibrary),
                          configuredWifiSsid().isEmpty()
                              ? tr2(TrKey2::PluginNoWifi)
                              : tr(TrKey::NotSet));
    delay(1400);
    renderPluginsList();
    return;
  }
}

void App::runPluginInstall(PluginManager::PluginId id, uint32_t nowMs) {
  (void)nowMs;

  if (configuredWifiSsid().isEmpty()) {
    display_.renderStatus(uiText(UiText::Plugins),
                          tr2(TrKey2::PluginNoWifi),
                          tr(TrKey::SettingsWifi));
    delay(1800);
    renderPluginsList();
    return;
  }

  const uint32_t targetMask = pluginManager_.maskAfterInstall(id);
  const String assetName = PluginManager::variantFilename(targetMask);
  const bool isPolish = (uiLanguage_ == UiLanguage::Polish);

  display_.renderStatus(uiText(UiText::Plugins),
                        tr2(TrKey2::PluginInstalling),
                        PluginManager::pluginName(id, isPolish));

  saveReadingPosition(true);

  OtaUpdater::Config config = preferredOtaConfig();
  const OtaUpdater::Result result =
      otaUpdater_.installAsset(config, assetName, "", &App::handleStorageStatus, this);

  Serial.printf("[plugin] install id=%u asset=%s code=%u summary=%s detail=%s\n",
                static_cast<unsigned int>(id), assetName.c_str(),
                static_cast<unsigned int>(result.code),
                result.summary.c_str(), result.detail.c_str());

  if (result.code == OtaUpdater::ResultCode::Success && result.rebootRequired) {
    pluginManager_.setMask(targetMask);
    display_.renderStatus(uiText(UiText::Plugins),
                          tr2(TrKey2::PluginRestartRequired),
                          PluginManager::pluginName(id, isPolish));
    delay(800);
    ESP.restart();
    return;
  }

  if (result.code == OtaUpdater::ResultCode::AssetMissing) {
    display_.renderStatus(uiText(UiText::Plugins),
                          tr2(TrKey2::PluginFetchFailed),
                          assetName);
    delay(2000);
    openPluginsList();
    return;
  }

  if (result.code == OtaUpdater::ResultCode::NoUpdate) {
    // Ten wariant jest już wgrany — wystarczy zaktualizować maskę
    pluginManager_.setMask(targetMask);
    display_.renderStatus(uiText(UiText::Plugins),
                          PluginManager::pluginName(id, isPolish),
                          tr2(TrKey2::PluginInstalled));
    delay(1400);
    openPluginsList();
    return;
  }

  // Inny błąd
  const String detail = result.detail.isEmpty() ? result.summary : result.detail;
  display_.renderStatus(uiText(UiText::Plugins),
                        tr2(TrKey2::PluginInstallFailed),
                        detail);
  delay(2000);
  openPluginsList();
}

void App::runPluginRemove(PluginManager::PluginId id, uint32_t nowMs) {
  (void)nowMs;

  if (configuredWifiSsid().isEmpty()) {
    display_.renderStatus(uiText(UiText::Plugins),
                          tr2(TrKey2::PluginNoWifi),
                          tr(TrKey::SettingsWifi));
    delay(1800);
    renderPluginsList();
    return;
  }

  const uint32_t targetMask = pluginManager_.maskAfterRemove(id);
  const String assetName = PluginManager::variantFilename(targetMask);
  const bool isPolish = (uiLanguage_ == UiLanguage::Polish);

  display_.renderStatus(uiText(UiText::Plugins),
                        tr2(TrKey2::PluginRemoving),
                        PluginManager::pluginName(id, isPolish));

  saveReadingPosition(true);

  OtaUpdater::Config config = preferredOtaConfig();
  const OtaUpdater::Result result =
      otaUpdater_.installAsset(config, assetName, "", &App::handleStorageStatus, this);

  Serial.printf("[plugin] remove id=%u asset=%s code=%u summary=%s detail=%s\n",
                static_cast<unsigned int>(id), assetName.c_str(),
                static_cast<unsigned int>(result.code),
                result.summary.c_str(), result.detail.c_str());

  if (result.code == OtaUpdater::ResultCode::Success && result.rebootRequired) {
    pluginManager_.setMask(targetMask);
    display_.renderStatus(uiText(UiText::Plugins),
                          tr2(TrKey2::PluginRestartRequired),
                          PluginManager::pluginName(id, isPolish));
    delay(800);
    ESP.restart();
    return;
  }

  if (result.code == OtaUpdater::ResultCode::AssetMissing) {
    display_.renderStatus(uiText(UiText::Plugins),
                          tr2(TrKey2::PluginFetchFailed),
                          assetName);
    delay(2000);
    openPluginsList();
    return;
  }

  if (result.code == OtaUpdater::ResultCode::NoUpdate) {
    pluginManager_.setMask(targetMask);
    display_.renderStatus(uiText(UiText::Plugins),
                          PluginManager::pluginName(id, isPolish),
                          tr(TrKey::NotSet));
    delay(1400);
    openPluginsList();
    return;
  }

  const String detail = result.detail.isEmpty() ? result.summary : result.detail;
  display_.renderStatus(uiText(UiText::Plugins),
                        tr2(TrKey2::PluginRemoveFailed),
                        detail);
  delay(2000);
  openPluginsList();
}

void App::renderPluginsList() {
  display_.renderMenu(pluginsMenuItems_, pluginsSelectedIndex_);
}

// ─── Presets ─────────────────────────────────────────────────────────────────

void App::openPresets() {
  menuScreen_ = MenuScreen::Presets;

  auto presets = presetManager_.listPresets();
  presetFilenames_.clear();
  presetFilenames_.reserve(presets.size());

  settingsMenuItems_.clear();
  settingsMenuItems_.reserve(presets.size() + 2);

  // [0] Back
  settingsMenuItems_.push_back(uiText(UiText::Back));

  // [1] Save Current / Limit reached
  if (presets.size() < PresetManager::kMaxPresets) {
    settingsMenuItems_.push_back(polish("+ Zapisz obecne", "+ Save Current"));
  } else {
    settingsMenuItems_.push_back(polish("(Limit 10 osiagniety)", "(Limit 10 reached)"));
  }

  // [2..] Preset names
  for (const auto &p : presets) {
    settingsMenuItems_.push_back(p.name);
    presetFilenames_.push_back(p.filename);
  }

  presetsSelectedIndex_ = 0;
  display_.renderMenu(settingsMenuItems_, presetsSelectedIndex_);
}

void App::selectPresetsItem(uint32_t nowMs) {
  if (presetsSelectedIndex_ == 0) {
    // Back → return to SettingsHome
    openSettings();
    return;
  }

  if (presetsSelectedIndex_ == 1) {
    // Save Current
    if (presetFilenames_.size() < PresetManager::kMaxPresets) {
      openTextEntry(TextEntryPurpose::PresetName,
                    polish("Nazwa presetu", "Preset Name"),
                    "", "", "", "", false,
                    PresetManager::kMaxPresetNameLength,
                    MenuScreen::Presets);
    }
    // If at limit, do nothing (item is informational)
    return;
  }

  // Index >= 2: open preset detail screen (Apply / Delete)
  const size_t presetIndex = presetsSelectedIndex_ - 2;
  confirmDeletePreset(presetIndex, nowMs);
}

void App::executeSavePreset(uint32_t nowMs) {
  (void)nowMs;

  String validatedName = presetManager_.validateName(textEntrySession_.value);
  textEntrySession_ = TextEntrySession();
  textEntryButtons_.clear();

  if (validatedName.isEmpty()) {
    display_.renderStatus(polish("Presety", "Presets"),
                          polish("Nieprawidlowa nazwa", "Invalid name"), "");
    delay(1000);
    openPresets();
    return;
  }

  PresetManager::SaveResult result = presetManager_.savePreset(validatedName, preferences_);

  switch (result) {
    case PresetManager::SaveResult::Ok:
      display_.renderStatus(polish("Presety", "Presets"),
                            polish("Zapisano", "Saved"), validatedName);
      delay(1000);
      break;
    case PresetManager::SaveResult::LimitReached:
      display_.renderStatus(polish("Presety", "Presets"),
                            polish("Limit osiagniety", "Limit reached"), "");
      delay(1000);
      break;
    default:
      display_.renderStatus(polish("Presety", "Presets"),
                            polish("Blad karty SD", "SD card error"), "");
      delay(1000);
      break;
  }

  openPresets();
}

void App::executeRestorePreset(size_t index, uint32_t nowMs) {
  const String &filename = presetFilenames_[index];
  PresetManager::RestoreResult result = presetManager_.restorePreset(filename, preferences_);

  if (result == PresetManager::RestoreResult::Ok) {
    reloadRuntimePreferences(nowMs, true);
    const String &presetName = settingsMenuItems_[index + 2];
    display_.renderStatus(polish("Preset", "Preset"),
                          polish("Wczytano", "Loaded"), presetName);
    delay(1200);
    openPresets();
  } else {
    display_.renderStatus(polish("Blad", "Error"),
                          polish("Blad wczytywania presetu", "Error loading preset"), "");
    delay(1400);
    openPresets();
  }
}

void App::confirmDeletePreset(size_t index, uint32_t nowMs) {
  (void)nowMs;
  presetsDeleteTargetIndex_ = index;
  menuScreen_ = MenuScreen::PresetsDeleteConfirm;

  // Build detail menu: Back + Apply + Delete
  const String presetName = settingsMenuItems_[index + 2];  // offset by Back + Save Current

  settingsMenuItems_.clear();
  settingsMenuItems_.reserve(3);
  settingsMenuItems_.push_back(uiText(UiText::Back));
  settingsMenuItems_.push_back(polish("Zastosuj: ", "Apply: ") + presetName);
  settingsMenuItems_.push_back(polish("Usun: ", "Delete: ") + presetName);

  presetsSelectedIndex_ = 0;
  display_.renderMenu(settingsMenuItems_, presetsSelectedIndex_);
}

void App::executeDeletePreset(uint32_t nowMs) {
  (void)nowMs;

  const String &filename = presetFilenames_[presetsDeleteTargetIndex_];
  PresetManager::DeleteResult result = presetManager_.deletePreset(filename);

  switch (result) {
    case PresetManager::DeleteResult::Ok:
      display_.renderStatus(polish("Presety", "Presets"),
                            polish("Usunieto", "Deleted"), "");
      delay(800);
      break;
    default:
      display_.renderStatus(polish("Presety", "Presets"),
                            polish("Blad usuwania", "Delete failed"), "");
      delay(1000);
      break;
  }

  openPresets();
}

void App::openRestartConfirm() {
  restartConfirmReturnScreen_ = menuScreen_;
  restartConfirmSelectedIndex_ = RestartConfirmNo;
  menuScreen_ = MenuScreen::RestartConfirm;
  renderRestartConfirm();
}

void App::selectRestartConfirmItem(uint32_t nowMs) {
  if (restartConfirmSelectedIndex_ != RestartConfirmYes) {
    menuScreen_ = restartConfirmReturnScreen_;
    renderMenu();
    return;
  }

  reader_.begin(nowMs);
  restartConfirmReturnScreen_ = MenuScreen::Main;
  menuScreen_ = MenuScreen::Main;
  setState(AppState::Paused, nowMs);
  saveReadingPosition(true);
  Serial.println("[restart] book restarted from beginning");
}

void App::openSdCardRepairConfirm() {
  sdCardRepairConfirmSelectedIndex_ = SdCardRepairConfirmNo;
  menuScreen_ = MenuScreen::SdCardRepairConfirm;
  renderSdCardRepairConfirm();
}

void App::selectSdCardRepairConfirmItem(uint32_t nowMs) {
  if (sdCardRepairConfirmSelectedIndex_ != SdCardRepairConfirmYes) {
    Serial.println("[sd-check] folder repair declined");
    menuScreen_ = MenuScreen::Main;
    renderMenu();
    return;
  }

  runSdCardRepair(nowMs);
}

void App::openUpdateConfirm() {
  updateConfirmSelectedIndex_ = UpdateConfirmSkip;
  menuScreen_ = MenuScreen::UpdateConfirm;
  renderUpdateConfirm();
}

void App::selectUpdateConfirmItem(uint32_t nowMs) {
  if (updateConfirmSelectedIndex_ != UpdateConfirmUpdate) {
    Serial.println("[ota] update skipped by user");
    menuScreen_ = MenuScreen::Main;
    setState(AppState::Paused, nowMs);
    return;
  }

  Serial.println("[ota] update confirmed by user");
  runFirmwareUpdate(preferredOtaConfig(), false, nowMs);
}

void App::enterCompanionSync(uint32_t nowMs) {
  if (blockNetworkActionForOtaCheck("Sync", nowMs)) {
    return;
  }

  Serial.println("[app] entering companion sync mode");
  saveReadingPosition(true);
  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  wpmFeedbackVisible_ = false;
  display_.renderStatus("Sync", tr(TrKey::StartingWifi), "");

  // If auto-sync AP is already running, just transition to CompanionSync state.
  if (autoSyncActive_) {
    Serial.println("[app] reusing auto-sync AP for companion sync mode");
    autoSyncActive_ = false;
    lastCompanionSyncRenderMs_ = 0;
    setState(AppState::CompanionSync, nowMs);
    return;
  }

  OtaUpdater::Config wifiConfig = preferredOtaConfig();
  CompanionSyncManager::Config syncConfig;
  syncConfig.wifiSsid = wifiConfig.wifiSsid;
  syncConfig.wifiPassword = wifiConfig.wifiPassword;

  if (!companionSync_.begin(syncConfig)) {
    Serial.println("[app] companion sync failed");
    display_.renderStatus("Sync", tr(TrKey::CouldNotStart),
                          tr(TrKey::Returning));
    delay(1400);
    menuScreen_ = MenuScreen::Main;
    setState(AppState::Menu, nowMs);
    return;
  }

  lastCompanionSyncRenderMs_ = 0;
  setState(AppState::CompanionSync, nowMs);
}

void App::updateCompanionSync(uint32_t nowMs) {
  companionSync_.update();

  if (powerButton_.isHeld() && nowMs - powerButton_.lastEdgeMs() >= kUsbTransferExitHoldMs) {
    powerButtonLongPressHandled_ = true;
    exitCompanionSync(nowMs);
    return;
  }

  // Touch anywhere = exit sync (back button)
  TouchEvent ev;
  if (touch_.poll(ev) && ev.phase == TouchPhase::End) {
    exitCompanionSync(nowMs);
    return;
  }

  if (nowMs - lastCompanionSyncRenderMs_ >= 1000) {
    lastCompanionSyncRenderMs_ = nowMs;
    if (companionSync_.hasQrCode()) {
      display_.renderStatusWithQr(polish("< Wroc | Wi-Fi", "< Back | Wi-Fi"),
                                  companionSync_.statusLine1(),
                                  companionSync_.qrCodeData(), companionSync_.qrCodeSize());
    } else {
      display_.renderStatus(polish("< Wroc | Sync", "< Back | Sync"),
                            companionSync_.statusLine1(), companionSync_.statusLine2());
    }
  }
}

void App::exitCompanionSync(uint32_t nowMs) {
  Serial.println("[app] leaving companion sync mode");
  display_.renderStatus("Sync", tr(TrKey::Stopping), "");
  companionSync_.end();
  preferences_.end();
  preferences_.begin(kPrefsNamespace, false);
  pluginManager_.begin(preferences_);
  reloadRuntimePreferences(nowMs, false);
  storage_.refreshBooks();
  menuScreen_ = MenuScreen::Main;
  setState(AppState::Paused, nowMs);
}

void App::runSdCardCheck(uint32_t nowMs) {
  (void)nowMs;
  Serial.println("[app] running SD card check");
  display_.renderStatus("SD", tr2(TrKey2::Starting), "");
  const StorageManager::DiagnosticResult result = storage_.diagnoseSdCard();

  if (sdCardFolderRepairNeeded(result)) {
    display_.renderStatus("SD", tr2(TrKey2::FoldersMissing), tr2(TrKey2::ConfirmRepair));
    delay(900);
    openSdCardRepairConfirm();
    return;
  }

  String detail = result.detail;
  if (detail.isEmpty() && result.mounted) {
    detail = String(static_cast<unsigned int>(result.sizeMb)) + " MB";
  }
  display_.renderStatus("SD check", result.summary, detail);
  delay(2600);

  menuScreen_ = MenuScreen::Main;
  renderMenu();
}

void App::runSdCardRepair(uint32_t nowMs) {
  (void)nowMs;
  Serial.println("[app] repairing SD card folder layout");
  display_.renderStatus("SD", tr2(TrKey2::RepairingFolders), tr(TrKey::PleaseWait));
  const bool repaired = storage_.repairSdCardFolders();
  if (!repaired) {
    display_.renderStatus("SD", tr2(TrKey2::FolderRepairFailed), tr2(TrKey2::FormatFat32));
    delay(2600);
    menuScreen_ = MenuScreen::Main;
    renderMenu();
    return;
  }

  display_.renderStatus("SD", tr2(TrKey2::FoldersRepaired), tr2(TrKey2::CheckingCard));
  delay(900);

  const StorageManager::DiagnosticResult result = storage_.diagnoseSdCard();
  String detail = result.detail;
  if (detail.isEmpty() && result.mounted) {
    detail = String(static_cast<unsigned int>(result.sizeMb)) + " MB";
  }
  display_.renderStatus("SD check", result.summary, detail);
  delay(2600);

  menuScreen_ = MenuScreen::Main;
  renderMenu();
}

void App::enterUsbTransfer(uint32_t nowMs) {
  Serial.println("[app] entering USB transfer mode");
  saveReadingPosition(true);
  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  wpmFeedbackVisible_ = false;
  const size_t resumeIndex = reader_.currentIndex();
  setState(AppState::UsbTransfer, nowMs);

  activeBookStore_.close();
  storage_.end();
  if (!usbTransfer_.begin(true)) {
    Serial.printf("[app] USB transfer failed: %s\n", usbTransfer_.statusMessage());
    display_.renderStatus("USB", tr2(TrKey2::SdNotReady), tr(TrKey::Returning));
    storageReady_ = storage_.begin();
    if (storageReady_ && usingStorageBook_ && !currentBookPath_.isEmpty()) {
      const int refreshedBookIndex = findBookIndexByPath(currentBookPath_);
      if (refreshedBookIndex >= 0 &&
          loadBookAtIndex(static_cast<size_t>(refreshedBookIndex), nowMs, false, false, false,
                          false)) {
        reader_.seekTo(resumeIndex);
      }
    }
    setState(AppState::Paused, nowMs);
    return;
  }

  const uint64_t sizeMb = usbTransfer_.cardSizeBytes() / (1024ULL * 1024ULL);
  Serial.printf("[app] USB transfer active (%llu MB). Eject from computer when finished.\n",
                sizeMb);
  display_.renderStatus(polish("< Wroc | USB", "< Back | USB"),
                        tr2(TrKey2::CopyBooksNow), tr2(TrKey2::EjectThenHoldPwr));
}

void App::updateUsbTransfer(uint32_t nowMs) {
  if (!usbTransfer_.active()) {
    return;
  }

  // Touch anywhere = exit USB transfer (back button)
  TouchEvent ev;
  if (touch_.poll(ev) && ev.phase == TouchPhase::End) {
    powerButtonLongPressHandled_ = true;
    exitUsbTransfer(nowMs);
    return;
  }

  const bool powerExitRequested =
      powerButton_.isHeld() && nowMs - powerButton_.lastEdgeMs() >= kUsbTransferExitHoldMs;
  if (!usbTransfer_.ejected() && !powerExitRequested) {
    return;
  }

  if (powerExitRequested && !usbTransfer_.ejected()) {
    Serial.println("[app] leaving USB transfer by PWR hold; make sure host was ejected first");
  }

  if (powerExitRequested) {
    powerButtonLongPressHandled_ = true;
  }

  exitUsbTransfer(nowMs);
}

void App::exitUsbTransfer(uint32_t nowMs) {
  Serial.println("[app] USB transfer ejected; remounting SD");
  display_.renderStatus("USB", tr2(TrKey2::RemountingSd), "");
  usbTransfer_.end();

  storageReady_ = storage_.begin();
  if (storageReady_) {
    const int refreshedBookIndex = findBookIndexByPath(currentBookPath_);
    if (refreshedBookIndex >= 0) {
      const size_t resumeIndex = reader_.currentIndex();
      if (loadBookAtIndex(static_cast<size_t>(refreshedBookIndex), nowMs, false, false, false,
                          false)) {
        reader_.seekTo(resumeIndex);
      } else {
        Serial.println("[app] current indexed book unavailable after USB transfer");
        usingStorageBook_ = false;
        currentBookPath_ = "";
        currentBookTitle_ = "Demo";
        reader_.clearLoadedBook(nowMs);
        reader_.begin(nowMs);
      }
    } else if (storage_.bookCount() > 0) {
      loadBookAtIndex(0, nowMs);
    }
  } else {
    Serial.println("[app] SD remount failed after USB transfer");
  }

  menuScreen_ = MenuScreen::Main;
  setState(AppState::Paused, nowMs);
}

void App::enterStandby(uint32_t nowMs) {
  if (state_ == AppState::UsbTransfer || state_ == AppState::CompanionSync ||
      state_ == AppState::Sleeping || powerOffStarted_) {
    return;
  }

  standbyReturnState_ = state_ == AppState::Playing ? AppState::Paused : state_;
  if (standbyReturnState_ == AppState::Booting || standbyReturnState_ == AppState::Standby) {
    standbyReturnState_ = AppState::Paused;
  }

  if (state_ == AppState::Playing) {
    saveReadingPosition(true);
  }

  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  touchPlayHeld_ = false;
  playLocked_ = false;
  pauseAtSentenceEndRequested_ = false;
  contextViewVisible_ = false;
  wpmFeedbackVisible_ = false;
  batteryWarningOverlayVisible_ = false;
  standbyEnteredMs_ = nowMs;
  standbyButtonsReleased_ = false;
  lastStandbyFrameMs_ = 0;
  setState(AppState::Standby, nowMs);
  Serial.println("[app] standby screensaver started");
}

void App::exitStandby(uint32_t nowMs) {
  if (state_ != AppState::Standby) {
    return;
  }

  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  touchPlayHeld_ = false;
  playLocked_ = false;
  pauseAtSentenceEndRequested_ = false;
  batteryWarningOverlayVisible_ = false;
  standbyButtonsReleased_ = false;

  AppState nextState = standbyReturnState_;
  if (nextState == AppState::Booting || nextState == AppState::Playing ||
      nextState == AppState::CompanionSync || nextState == AppState::UsbTransfer ||
      nextState == AppState::Standby || nextState == AppState::Sleeping) {
    nextState = AppState::Paused;
  }

  Serial.println("[app] leaving standby");
  if (standbyScreenOffActive_) {
    display_.wakeFromSleep();
    standbyScreenOffActive_ = false;
  }
  setState(nextState, nowMs);
}

void App::seedStandbyScreensaver(uint32_t nowMs) {
  if (screensaverMode_ != ScreensaverMode::ScreenOff && standbyScreenOffActive_) {
    display_.wakeFromSleep();
    standbyScreenOffActive_ = false;
  }

  switch (screensaverMode_) {
    case ScreensaverMode::Maze:
      seedStandbyMaze(nowMs);
      return;
    case ScreensaverMode::Voronoi:
      seedStandbyVoronoi(nowMs);
      return;
    case ScreensaverMode::Stars:
      seedStandbyStars(nowMs);
      return;
    case ScreensaverMode::Matrix:
      seedStandbyMatrix(nowMs);
      return;
    case ScreensaverMode::ScreenOff:
      seedStandbyScreenOff(nowMs);
      return;
    case ScreensaverMode::Life:
    default:
      seedStandbyLife(nowMs);
      return;
  }
}

void App::stepStandbyScreensaver(uint32_t nowMs) {
  (void)nowMs;
  switch (screensaverMode_) {
    case ScreensaverMode::Maze:
      stepStandbyMaze();
      return;
    case ScreensaverMode::Voronoi:
      stepStandbyVoronoi();
      return;
    case ScreensaverMode::Stars:
      stepStandbyStars();
      return;
    case ScreensaverMode::Matrix:
      stepStandbyMatrix();
      return;
    case ScreensaverMode::ScreenOff:
      return;
    case ScreensaverMode::Life:
    default:
      stepStandbyLife();
      return;
  }
}

void App::seedStandbyLife(uint32_t nowMs) {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  standbyLifeCells_.assign(packedLifeWordCount(cellCount), 0);
  standbyLifeNextCells_.assign(packedLifeWordCount(cellCount), 0);
  standbyScreensaverDimCells_.clear();
  standbyMazeVisited_.clear();
  standbyMazeStack_.clear();
  standbyVoronoiX_.clear();
  standbyVoronoiY_.clear();
  standbyVoronoiDx_.clear();
  standbyVoronoiDy_.clear();
  standbyLifeGeneration_ = 0;

  standbyScreensaverRng_ =
      nowMs ^ micros() ^ (static_cast<uint32_t>(reader_.currentIndex() + 1) * 2654435761UL) ^
      (static_cast<uint32_t>(batteryDisplayedPercent_) << 24);
  for (size_t i = 0; i < cellCount; ++i) {
    setPackedLifeCell(standbyLifeCells_, i, (advanceStandbyRng(standbyScreensaverRng_) >> 24) < 12);
  }

  clearAndStampPackedLifePattern(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                                 kLifeGosperGliderGun,
                                 sizeof(kLifeGosperGliderGun) / sizeof(kLifeGosperGliderGun[0]),
                                 18, 18, 36, 9);
  clearAndStampPackedLifePattern(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                                 kLifeGosperGliderGun,
                                 sizeof(kLifeGosperGliderGun) / sizeof(kLifeGosperGliderGun[0]),
                                 static_cast<int>(kStandbyLifeColumns) - 62,
                                 static_cast<int>(kStandbyLifeRows) - 34, 36, 9);
  clearAndStampPackedLifePattern(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                                 kLifePulsar, sizeof(kLifePulsar) / sizeof(kLifePulsar[0]),
                                 static_cast<int>(kStandbyLifeColumns / 2) - 7,
                                 static_cast<int>(kStandbyLifeRows / 2) - 7, 13, 13);
  clearAndStampPackedLifePattern(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                                 kLifePentadecathlon,
                                 sizeof(kLifePentadecathlon) / sizeof(kLifePentadecathlon[0]),
                                 static_cast<int>(kStandbyLifeColumns / 3),
                                 static_cast<int>(kStandbyLifeRows) - 42, 5, 10);
  clearAndStampPackedLifePattern(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                                 kLifeLightweightSpaceship,
                                 sizeof(kLifeLightweightSpaceship) /
                                     sizeof(kLifeLightweightSpaceship[0]),
                                 static_cast<int>((kStandbyLifeColumns * 2) / 3),
                                 static_cast<int>(kStandbyLifeRows / 3), 5, 4);

  for (uint8_t i = 0; i < 10; ++i) {
    const int x =
        static_cast<int>((advanceStandbyRng(standbyScreensaverRng_) >> 8) %
                         std::max<uint16_t>(1, kStandbyLifeColumns - 6));
    const int y =
        static_cast<int>((advanceStandbyRng(standbyScreensaverRng_) >> 8) %
                         std::max<uint16_t>(1, kStandbyLifeRows - 6));
    clearAndStampPackedLifePattern(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                                   kLifeGlider, sizeof(kLifeGlider) / sizeof(kLifeGlider[0]), x,
                                   y, 3, 3);
  }
}

void App::stepStandbyLife() {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  const size_t wordCount = packedLifeWordCount(cellCount);
  if (standbyLifeCells_.size() != wordCount || standbyLifeNextCells_.size() != wordCount) {
    seedStandbyLife(millis());
    return;
  }

  std::fill(standbyLifeNextCells_.begin(), standbyLifeNextCells_.end(), 0);
  size_t aliveCount = 0;
  for (uint16_t y = 0; y < kStandbyLifeRows; ++y) {
    for (uint16_t x = 0; x < kStandbyLifeColumns; ++x) {
      uint8_t neighbours = 0;
      for (int8_t dy = -1; dy <= 1; ++dy) {
        for (int8_t dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0) {
            continue;
          }
          const uint16_t nx =
              static_cast<uint16_t>((static_cast<int>(x) + dx + kStandbyLifeColumns) %
                                    kStandbyLifeColumns);
          const uint16_t ny =
              static_cast<uint16_t>((static_cast<int>(y) + dy + kStandbyLifeRows) %
                                    kStandbyLifeRows);
          neighbours += packedLifeCellAlive(
              standbyLifeCells_, static_cast<size_t>(ny) * kStandbyLifeColumns + nx)
                            ? 1
                            : 0;
        }
      }

      const size_t index = static_cast<size_t>(y) * kStandbyLifeColumns + x;
      const bool alive = packedLifeCellAlive(standbyLifeCells_, index);
      const bool nextAlive = alive ? (neighbours == 2 || neighbours == 3) : (neighbours == 3);
      setPackedLifeCell(standbyLifeNextCells_, index, nextAlive);
      if (nextAlive) {
        ++aliveCount;
      }
    }
  }

  standbyLifeCells_.swap(standbyLifeNextCells_);
  ++standbyLifeGeneration_;
  if (aliveCount == 0 || aliveCount > (cellCount * 3) / 4) {
    seedStandbyLife(millis());
  }
}

void App::seedStandbyMaze(uint32_t nowMs) {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  const uint16_t mazeColumns = std::max<uint16_t>(1, (kStandbyLifeColumns - 1) / 2);
  const uint16_t mazeRows = std::max<uint16_t>(1, (kStandbyLifeRows - 1) / 2);
  standbyLifeCells_.assign(packedLifeWordCount(cellCount), 0);
  standbyLifeNextCells_.assign(packedLifeWordCount(cellCount), 0);
  standbyScreensaverDimCells_.clear();
  standbyVoronoiX_.clear();
  standbyVoronoiY_.clear();
  standbyVoronoiDx_.clear();
  standbyVoronoiDy_.clear();
  standbyMazeVisited_.assign(static_cast<size_t>(mazeColumns) * mazeRows, 0);
  standbyMazeStack_.clear();
  standbyLifeGeneration_ = 0;
  standbyScreensaverRng_ =
      nowMs ^ micros() ^ (static_cast<uint32_t>(reader_.currentIndex() + 1) * 2246822519UL);

  const uint16_t startX = static_cast<uint16_t>((advanceStandbyRng(standbyScreensaverRng_) >> 8) %
                                               mazeColumns);
  const uint16_t startY = static_cast<uint16_t>((advanceStandbyRng(standbyScreensaverRng_) >> 8) %
                                               mazeRows);
  standbyMazeVisited_[static_cast<size_t>(startY) * mazeColumns + startX] = 1;
  standbyMazeStack_.push_back(static_cast<uint16_t>(startY * mazeColumns + startX));
  setPackedLifeCellAt(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                      static_cast<int>(startX) * 2 + 1, static_cast<int>(startY) * 2 + 1, true);
}

void App::stepStandbyMaze() {
  const uint16_t mazeColumns = std::max<uint16_t>(1, (kStandbyLifeColumns - 1) / 2);
  const uint16_t mazeRows = std::max<uint16_t>(1, (kStandbyLifeRows - 1) / 2);
  const size_t mazeCellCount = static_cast<size_t>(mazeColumns) * mazeRows;
  if (standbyMazeVisited_.size() != mazeCellCount || standbyMazeStack_.empty()) {
    if (standbyMazeStack_.empty() && standbyLifeGeneration_ < 600) {
      ++standbyLifeGeneration_;
      return;
    }
    seedStandbyMaze(millis());
    return;
  }

  constexpr uint8_t kMazeStepsPerFrame = 32;
  for (uint8_t step = 0; step < kMazeStepsPerFrame && !standbyMazeStack_.empty(); ++step) {
    const uint16_t current = standbyMazeStack_.back();
    const uint16_t cx = current % mazeColumns;
    const uint16_t cy = current / mazeColumns;
    uint16_t candidates[4];
    uint8_t candidateCount = 0;

    auto addCandidate = [&](int nx, int ny) {
      if (nx < 0 || ny < 0 || nx >= static_cast<int>(mazeColumns) ||
          ny >= static_cast<int>(mazeRows)) {
        return;
      }
      const uint16_t encoded = static_cast<uint16_t>(ny * mazeColumns + nx);
      if (standbyMazeVisited_[encoded] == 0) {
        candidates[candidateCount++] = encoded;
      }
    };

    addCandidate(static_cast<int>(cx) + 1, cy);
    addCandidate(static_cast<int>(cx) - 1, cy);
    addCandidate(cx, static_cast<int>(cy) + 1);
    addCandidate(cx, static_cast<int>(cy) - 1);

    if (candidateCount == 0) {
      standbyMazeStack_.pop_back();
      continue;
    }

    const uint16_t next = candidates[(advanceStandbyRng(standbyScreensaverRng_) >> 16) %
                                     candidateCount];
    const uint16_t nx = next % mazeColumns;
    const uint16_t ny = next / mazeColumns;
    standbyMazeVisited_[next] = 1;
    standbyMazeStack_.push_back(next);

    const int displayCx = static_cast<int>(cx) * 2 + 1;
    const int displayCy = static_cast<int>(cy) * 2 + 1;
    const int displayNx = static_cast<int>(nx) * 2 + 1;
    const int displayNy = static_cast<int>(ny) * 2 + 1;
    setPackedLifeCellAt(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows, displayNx,
                        displayNy, true);
    setPackedLifeCellAt(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                        (displayCx + displayNx) / 2, (displayCy + displayNy) / 2, true);
  }

  if (standbyMazeStack_.empty()) {
    standbyLifeGeneration_ = 0;
  } else {
    ++standbyLifeGeneration_;
  }
}

void App::seedStandbyVoronoi(uint32_t nowMs) {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  const size_t wordCount = packedLifeWordCount(cellCount);
  standbyLifeCells_.assign(wordCount, 0);
  standbyLifeNextCells_.assign(wordCount, 0);
  standbyScreensaverDimCells_.assign(wordCount, 0);
  standbyMazeVisited_.clear();
  standbyMazeStack_.clear();
  standbyLifeGeneration_ = 0;
  standbyScreensaverRng_ =
      nowMs ^ micros() ^ (static_cast<uint32_t>(reader_.currentIndex() + 1) * 3266489917UL) ^
      0x51a7f00dUL;

  constexpr size_t kVoronoiSiteCount = 15;
  standbyVoronoiX_.assign(kVoronoiSiteCount, 0);
  standbyVoronoiY_.assign(kVoronoiSiteCount, 0);
  standbyVoronoiDx_.assign(kVoronoiSiteCount, 0);
  standbyVoronoiDy_.assign(kVoronoiSiteCount, 0);
  for (size_t i = 0; i < kVoronoiSiteCount; ++i) {
    standbyVoronoiX_[i] = static_cast<int16_t>(
        ((advanceStandbyRng(standbyScreensaverRng_) >> 8) % kStandbyLifeColumns) * 16);
    standbyVoronoiY_[i] = static_cast<int16_t>(
        ((advanceStandbyRng(standbyScreensaverRng_) >> 8) % kStandbyLifeRows) * 16);

    const int16_t dx =
        static_cast<int16_t>(4 + ((advanceStandbyRng(standbyScreensaverRng_) >> 24) % 7));
    const int16_t dy =
        static_cast<int16_t>(3 + ((advanceStandbyRng(standbyScreensaverRng_) >> 24) % 6));
    standbyVoronoiDx_[i] =
        (advanceStandbyRng(standbyScreensaverRng_) & 1U) != 0 ? dx : static_cast<int16_t>(-dx);
    standbyVoronoiDy_[i] =
        (advanceStandbyRng(standbyScreensaverRng_) & 1U) != 0 ? dy : static_cast<int16_t>(-dy);
  }
  renderStandbyVoronoi();
}

void App::renderStandbyVoronoi() {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  const size_t wordCount = packedLifeWordCount(cellCount);
  standbyLifeCells_.assign(wordCount, 0);
  standbyScreensaverDimCells_.assign(wordCount, 0);
  if (standbyVoronoiX_.empty()) {
    return;
  }

  for (uint16_t y = 0; y < kStandbyLifeRows; ++y) {
    const int32_t cellY = static_cast<int32_t>(y) * 16 + 8;
    for (uint16_t x = 0; x < kStandbyLifeColumns; ++x) {
      const int32_t cellX = static_cast<int32_t>(x) * 16 + 8;
      int32_t nearest = INT32_MAX;
      int32_t secondNearest = INT32_MAX;
      for (size_t i = 0; i < standbyVoronoiX_.size(); ++i) {
        const int32_t dx = cellX - standbyVoronoiX_[i];
        const int32_t dy = cellY - standbyVoronoiY_[i];
        const int32_t distance = dx * dx + dy * dy;
        if (distance < nearest) {
          secondNearest = nearest;
          nearest = distance;
        } else if (distance < secondNearest) {
          secondNearest = distance;
        }
      }

      const size_t index = static_cast<size_t>(y) * kStandbyLifeColumns + x;
      const int32_t gap = secondNearest - nearest;
      if (nearest < 1200 || gap < 190) {
        setPackedLifeCell(standbyLifeCells_, index, true);
      } else if (gap < 580 + nearest / 180) {
        setPackedLifeCell(standbyScreensaverDimCells_, index, true);
      }
    }
  }
}

void App::stepStandbyVoronoi() {
  constexpr size_t kVoronoiSiteCount = 15;
  if (standbyVoronoiX_.size() != kVoronoiSiteCount ||
      standbyVoronoiY_.size() != kVoronoiSiteCount ||
      standbyVoronoiDx_.size() != kVoronoiSiteCount ||
      standbyVoronoiDy_.size() != kVoronoiSiteCount) {
    seedStandbyVoronoi(millis());
    return;
  }

  const int16_t maxX = static_cast<int16_t>((kStandbyLifeColumns - 1) * 16);
  const int16_t maxY = static_cast<int16_t>((kStandbyLifeRows - 1) * 16);
  for (size_t i = 0; i < standbyVoronoiX_.size(); ++i) {
    int16_t nextX = static_cast<int16_t>(standbyVoronoiX_[i] + standbyVoronoiDx_[i]);
    int16_t nextY = static_cast<int16_t>(standbyVoronoiY_[i] + standbyVoronoiDy_[i]);
    if (nextX < 0 || nextX > maxX) {
      standbyVoronoiDx_[i] = static_cast<int16_t>(-standbyVoronoiDx_[i]);
      nextX = std::max<int16_t>(0, std::min<int16_t>(maxX, nextX));
    }
    if (nextY < 0 || nextY > maxY) {
      standbyVoronoiDy_[i] = static_cast<int16_t>(-standbyVoronoiDy_[i]);
      nextY = std::max<int16_t>(0, std::min<int16_t>(maxY, nextY));
    }
    standbyVoronoiX_[i] = nextX;
    standbyVoronoiY_[i] = nextY;
  }

  ++standbyLifeGeneration_;
  if (standbyLifeGeneration_ > 2400) {
    seedStandbyVoronoi(millis());
    return;
  }
  renderStandbyVoronoi();
}

void App::seedStandbyScreenOff(uint32_t nowMs) {
  (void)nowMs;
  standbyLifeCells_.clear();
  standbyLifeNextCells_.clear();
  standbyScreensaverDimCells_.clear();
  standbyMazeVisited_.clear();
  standbyMazeStack_.clear();
  standbyVoronoiX_.clear();
  standbyVoronoiY_.clear();
  standbyVoronoiDx_.clear();
  standbyVoronoiDy_.clear();
  standbyStarsX_.clear();
  standbyStarsY_.clear();
  standbyStarsSpeed_.clear();
  standbyStarsBright_.clear();
  standbyMatrixColumns_.clear();
  standbyMatrixHeads_.clear();
  standbyMatrixTrails_.clear();
  standbyLifeGeneration_ = 0;
  standbyScreenOffActive_ = true;
  display_.prepareForSleep();
}

void App::seedStandbyStars(uint32_t nowMs) {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  const size_t wordCount = packedLifeWordCount(cellCount);
  standbyLifeCells_.assign(wordCount, 0);
  standbyLifeNextCells_.assign(wordCount, 0);
  standbyScreensaverDimCells_.assign(wordCount, 0);
  standbyMazeVisited_.clear();
  standbyMazeStack_.clear();
  standbyLifeGeneration_ = 0;

  standbyScreensaverRng_ =
      nowMs ^ micros() ^ (static_cast<uint32_t>(reader_.currentIndex() + 1) * 2246822519UL) ^
      0x57A25EEDUL;

  // Initialize 60 stars at random positions with random speeds and brightness
  constexpr size_t kStarCount = 60;
  standbyStarsX_.resize(kStarCount);
  standbyStarsY_.resize(kStarCount);
  standbyStarsSpeed_.resize(kStarCount);
  standbyStarsBright_.resize(kStarCount);

  for (size_t i = 0; i < kStarCount; ++i) {
    standbyStarsX_[i] = static_cast<int16_t>(
        (advanceStandbyRng(standbyScreensaverRng_) >> 8) % kStandbyLifeColumns);
    standbyStarsY_[i] = static_cast<int16_t>(
        (advanceStandbyRng(standbyScreensaverRng_) >> 8) % kStandbyLifeRows);
    standbyStarsSpeed_[i] = static_cast<int8_t>(
        1 + ((advanceStandbyRng(standbyScreensaverRng_) >> 24) % 3));
    standbyStarsBright_[i] = static_cast<uint8_t>(
        (advanceStandbyRng(standbyScreensaverRng_) >> 16) % 2);
  }

  // Render initial frame
  std::fill(standbyLifeCells_.begin(), standbyLifeCells_.end(), 0);
  for (size_t i = 0; i < kStarCount; ++i) {
    const size_t idx = static_cast<size_t>(standbyStarsY_[i]) * kStandbyLifeColumns +
                       static_cast<size_t>(standbyStarsX_[i]);
    if (idx < cellCount) {
      setPackedLifeCell(standbyLifeCells_, idx, true);
      if (standbyStarsBright_[i]) {
        setPackedLifeCell(standbyScreensaverDimCells_, idx, true);
      }
    }
  }
}

void App::stepStandbyStars() {
  constexpr size_t kStarCount = 60;
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);

  if (standbyStarsX_.size() != kStarCount) return;

  std::fill(standbyLifeCells_.begin(), standbyLifeCells_.end(), 0);
  std::fill(standbyScreensaverDimCells_.begin(), standbyScreensaverDimCells_.end(), 0);

  for (size_t i = 0; i < kStarCount; ++i) {
    // Move star down by its speed (simulating falling stars / starfield)
    standbyStarsY_[i] += standbyStarsSpeed_[i];
    if (standbyStarsY_[i] >= static_cast<int16_t>(kStandbyLifeRows)) {
      // Respawn at top with new random x
      standbyStarsY_[i] = 0;
      standbyStarsX_[i] = static_cast<int16_t>(
          (advanceStandbyRng(standbyScreensaverRng_) >> 8) % kStandbyLifeColumns);
      standbyStarsSpeed_[i] = static_cast<int8_t>(
          1 + ((advanceStandbyRng(standbyScreensaverRng_) >> 24) % 3));
      standbyStarsBright_[i] = static_cast<uint8_t>(
          (advanceStandbyRng(standbyScreensaverRng_) >> 16) % 2);
    }

    const size_t idx = static_cast<size_t>(standbyStarsY_[i]) * kStandbyLifeColumns +
                       static_cast<size_t>(standbyStarsX_[i]);
    if (idx < cellCount) {
      setPackedLifeCell(standbyLifeCells_, idx, true);
      if (standbyStarsBright_[i]) {
        setPackedLifeCell(standbyScreensaverDimCells_, idx, true);
      }
    }

    // Draw a short trail behind
    for (int8_t t = 1; t <= 2; ++t) {
      const int16_t trailY = standbyStarsY_[i] - t * standbyStarsSpeed_[i];
      if (trailY >= 0 && trailY < static_cast<int16_t>(kStandbyLifeRows)) {
        const size_t trailIdx = static_cast<size_t>(trailY) * kStandbyLifeColumns +
                                static_cast<size_t>(standbyStarsX_[i]);
        if (trailIdx < cellCount) {
          setPackedLifeCell(standbyScreensaverDimCells_, trailIdx, true);
        }
      }
    }
  }

  // Occasionally twinkle: randomly toggle a few dim cells
  for (uint8_t t = 0; t < 4; ++t) {
    const size_t twinkleIdx =
        (advanceStandbyRng(standbyScreensaverRng_) >> 8) % cellCount;
    setPackedLifeCell(standbyScreensaverDimCells_, twinkleIdx,
                      !packedLifeCellAlive(standbyScreensaverDimCells_, twinkleIdx));
  }

  standbyLifeGeneration_++;
}

void App::seedStandbyMatrix(uint32_t nowMs) {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  const size_t wordCount = packedLifeWordCount(cellCount);
  standbyLifeCells_.assign(wordCount, 0);
  standbyLifeNextCells_.assign(wordCount, 0);
  standbyScreensaverDimCells_.assign(wordCount, 0);
  standbyMazeVisited_.clear();
  standbyMazeStack_.clear();
  standbyLifeGeneration_ = 0;

  standbyScreensaverRng_ =
      nowMs ^ micros() ^ (static_cast<uint32_t>(reader_.currentIndex() + 1) * 3266489917UL) ^
      0xAA7E1CEDUL;

  // Initialize column raindrop heads
  const uint16_t numCols = kStandbyLifeColumns;
  standbyMatrixColumns_.resize(numCols);
  standbyMatrixHeads_.resize(numCols);
  standbyMatrixTrails_.resize(numCols);

  for (uint16_t c = 0; c < numCols; ++c) {
    standbyMatrixHeads_[c] = static_cast<uint8_t>(
        (advanceStandbyRng(standbyScreensaverRng_) >> 8) % kStandbyLifeRows);
    standbyMatrixTrails_[c] = static_cast<uint8_t>(
        4 + ((advanceStandbyRng(standbyScreensaverRng_) >> 16) % 8));
    // Speed: 0 = slow (skip some frames), 1 = normal, 2 = fast
    standbyMatrixColumns_[c] = static_cast<uint8_t>(
        (advanceStandbyRng(standbyScreensaverRng_) >> 24) % 3);
  }
}

void App::stepStandbyMatrix() {
  const size_t cellCount =
      static_cast<size_t>(kStandbyLifeColumns) * static_cast<size_t>(kStandbyLifeRows);
  const uint16_t numCols = kStandbyLifeColumns;
  const uint16_t numRows = kStandbyLifeRows;

  if (standbyMatrixHeads_.size() != numCols) return;

  std::fill(standbyLifeCells_.begin(), standbyLifeCells_.end(), 0);
  std::fill(standbyScreensaverDimCells_.begin(), standbyScreensaverDimCells_.end(), 0);

  standbyLifeGeneration_++;

  for (uint16_t c = 0; c < numCols; ++c) {
    // Determine if this column should advance this frame based on speed
    const uint8_t speed = standbyMatrixColumns_[c];
    bool shouldAdvance = true;
    if (speed == 0) {
      shouldAdvance = (standbyLifeGeneration_ % 3) == 0;
    } else if (speed == 1) {
      shouldAdvance = (standbyLifeGeneration_ % 2) == 0;
    }

    if (shouldAdvance) {
      standbyMatrixHeads_[c]++;
      if (standbyMatrixHeads_[c] >= numRows + standbyMatrixTrails_[c]) {
        // Respawn from top with new trail length and speed
        standbyMatrixHeads_[c] = 0;
        standbyMatrixTrails_[c] = static_cast<uint8_t>(
            4 + ((advanceStandbyRng(standbyScreensaverRng_) >> 16) % 8));
        standbyMatrixColumns_[c] = static_cast<uint8_t>(
            (advanceStandbyRng(standbyScreensaverRng_) >> 24) % 3);
      }
    }

    // Draw the head (bright pixel)
    const int16_t headY = static_cast<int16_t>(standbyMatrixHeads_[c]);
    if (headY >= 0 && headY < numRows) {
      const size_t headIdx = static_cast<size_t>(headY) * numCols + c;
      setPackedLifeCell(standbyLifeCells_, headIdx, true);
    }

    // Draw the trail (dimmer pixels)
    const uint8_t trail = standbyMatrixTrails_[c];
    for (uint8_t t = 1; t <= trail; ++t) {
      const int16_t trailY = headY - static_cast<int16_t>(t);
      if (trailY >= 0 && trailY < numRows) {
        const size_t trailIdx = static_cast<size_t>(trailY) * numCols + c;
        if (t <= 2) {
          // Near-head trail: bright
          setPackedLifeCell(standbyLifeCells_, trailIdx, true);
        } else {
          // Far trail: dim
          setPackedLifeCell(standbyScreensaverDimCells_, trailIdx, true);
        }
      }
    }
  }

  // Add occasional random bright flickers for the "digital rain" effect
  for (uint8_t f = 0; f < 3; ++f) {
    const size_t flickerIdx =
        (advanceStandbyRng(standbyScreensaverRng_) >> 8) % cellCount;
    setPackedLifeCell(standbyScreensaverDimCells_, flickerIdx, true);
  }
}

void App::openScreensaverSettings() {
  screensaverSettingsSelectedIndex_ = kScreensaverSettingsStyleIndex;
  menuScreen_ = MenuScreen::ScreensaverSettings;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::selectScreensaverSettingsItem(uint32_t nowMs) {
  (void)nowMs;
  switch (settingsSelectedIndex_) {
    case kScreensaverSettingsBackIndex:
      settingsSelectedIndex_ = kSettingsDisplayScreensaverIndex;
      menuScreen_ = MenuScreen::SettingsDisplay;
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kScreensaverSettingsStyleIndex:
      // Cycle through all screensaver modes
      switch (screensaverMode_) {
        case ScreensaverMode::Life:
          screensaverMode_ = ScreensaverMode::Maze;
          break;
        case ScreensaverMode::Maze:
          screensaverMode_ = ScreensaverMode::Voronoi;
          break;
        case ScreensaverMode::Voronoi:
          screensaverMode_ = ScreensaverMode::Stars;
          break;
        case ScreensaverMode::Stars:
          screensaverMode_ = ScreensaverMode::Matrix;
          break;
        case ScreensaverMode::Matrix:
          screensaverMode_ = ScreensaverMode::ScreenOff;
          break;
        case ScreensaverMode::ScreenOff:
        default:
          screensaverMode_ = ScreensaverMode::Life;
          break;
      }
      preferences_.putUChar(kPrefScreensaverMode, static_cast<uint8_t>(screensaverMode_));
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kScreensaverSettingsTimeoutIndex:
      screensaverTimeoutIndex_ =
          (screensaverTimeoutIndex_ + 1) % kScreensaverTimeoutCount;
      preferences_.putUChar(kPrefScreensaverTimeout, screensaverTimeoutIndex_);
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kScreensaverSettingsAutoOffIndex:
      screensaverAutoOffIndex_ =
          (screensaverAutoOffIndex_ + 1) % kScreensaverAutoOffCount;
      preferences_.putUChar(kPrefScreensaverAutoOff, screensaverAutoOffIndex_);
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kScreensaverSettingsSleepGuardIndex:
      screensaverSleepGuardIndex_ =
          (screensaverSleepGuardIndex_ + 1) % kScreensaverSleepGuardCount;
      preferences_.putUChar(kPrefScreensaverSleepGuard, screensaverSleepGuardIndex_);
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    case kScreensaverSettingsPreviewIndex:
      // Enter standby to preview the screensaver
      enterStandby(nowMs);
      return;
    default:
      return;
  }
}

void App::renderScreensaverSettings() {
  renderSettings();
}

String App::screensaverTimeoutLabel() const {
  if (screensaverTimeoutIndex_ >= kScreensaverTimeoutCount) return "5 min";
  const uint16_t minutes = kScreensaverTimeoutMinutes[screensaverTimeoutIndex_];
  return String(minutes) + " min";
}

String App::screensaverAutoOffLabel() const {
  if (screensaverAutoOffIndex_ >= kScreensaverAutoOffCount) return tr(TrKey::Never);
  if (screensaverAutoOffIndex_ == 0) return tr(TrKey::Never);
  const uint16_t minutes = kScreensaverAutoOffMinutes[screensaverAutoOffIndex_];
  if (minutes >= 60) {
    return String(minutes / 60) + "h";
  }
  return String(minutes) + " min";
}

String App::screensaverSleepGuardLabel() const {
  if (screensaverSleepGuardIndex_ >= kScreensaverSleepGuardCount) return tr(TrKey::No);
  if (screensaverSleepGuardIndex_ == 0) return tr(TrKey::No);
  const uint16_t minutes = kScreensaverSleepGuardMinutes[screensaverSleepGuardIndex_];
  if (minutes >= 60) {
    return String(minutes / 60) + "h";
  }
  return String(minutes) + " min";
}

void App::updateStandbyScreensaver(uint32_t nowMs, bool force) {
  if (state_ != AppState::Standby) {
    return;
  }

  // Auto power-off: check if standby has been active long enough
  if (screensaverAutoOffIndex_ > 0) {
    const uint32_t autoOffMs =
        static_cast<uint32_t>(kScreensaverAutoOffMinutes[screensaverAutoOffIndex_]) * 60000UL;
    if (nowMs - standbyEnteredMs_ >= autoOffMs) {
      Serial.println("[app] screensaver auto power-off triggered");
      enterPowerOff(nowMs);
      return;
    }
  }

  if (screensaverMode_ == ScreensaverMode::ScreenOff) {
    if (!standbyScreenOffActive_) {
      seedStandbyScreenOff(nowMs);
    }
    lastStandbyFrameMs_ = nowMs;
    return;
  }

  if (!force && nowMs - lastStandbyFrameMs_ < kStandbyFrameMs) {
    return;
  }

  if (!force) {
    stepStandbyScreensaver(nowMs);
  } else if (standbyLifeCells_.empty()) {
    seedStandbyScreensaver(nowMs);
  }

  lastStandbyFrameMs_ = nowMs;
  
  // Hint text: fade in/out every 10 seconds
  // Cycle: 10s total. First 1s = fade in, 1s-3s = visible, 3s-4s = fade out, 4s-10s = hidden
  const uint32_t standbyElapsed = nowMs - standbyEnteredMs_;
  const uint32_t hintCycleMs = standbyElapsed % 10000UL;
  uint8_t hintAlpha = 0;
  String hintText;
  if (hintCycleMs < 1000) {
    // Fade in: 0→255 over 1 second
    hintAlpha = static_cast<uint8_t>((hintCycleMs * 255UL) / 1000UL);
  } else if (hintCycleMs < 3000) {
    // Fully visible
    hintAlpha = 255;
  } else if (hintCycleMs < 4000) {
    // Fade out: 255→0 over 1 second
    hintAlpha = static_cast<uint8_t>(255 - ((hintCycleMs - 3000UL) * 255UL) / 1000UL);
  }
  if (hintAlpha > 0) {
    hintText = tr(TrKey::ScreensaverHint);
  }

  display_.renderLifeScreensaver(standbyLifeCells_, kStandbyLifeColumns, kStandbyLifeRows,
                                 standbyLifeGeneration_,
                                 standbyScreensaverDimCells_.empty() ? nullptr
                                                                      : &standbyScreensaverDimCells_,
                                 hintText, hintAlpha);
}

void App::enterPowerOff(uint32_t nowMs) {
  if (powerOffStarted_) {
    return;
  }

  powerOffStarted_ = true;
  Serial.println("[app] powering off; hold PWR to start again");
  saveReadingPosition(true);
  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  touchPlayHeld_ = false;
  contextViewVisible_ = false;
  wpmFeedbackVisible_ = false;
  menuScreen_ = MenuScreen::Main;
  state_ = AppState::Sleeping;

  display_.renderStatus("OFF", tr2(TrKey2::ReleasePwr), tr2(TrKey2::HoldPwrToStart));
  delay(300);
  display_.prepareForSleep();

  activeBookStore_.close();
  storage_.end();
  touch_.end();
  touchInitialized_ = false;
  Serial.flush();

  BoardConfig::holdBacklightOffForDeepSleep();
  BoardConfig::releaseBatteryPowerHold();

  const uint32_t waitStartMs = millis();
  while (powerButton_.isHeld() && millis() - waitStartMs < kPowerOffReleaseWaitMs) {
    powerButton_.update(millis());
    delay(10);
  }

  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(BoardConfig::PIN_PWR_BUTTON), 0);
  esp_deep_sleep_start();
}

void App::enterSleep(uint32_t nowMs) {
  Serial.println("[app] entering light sleep; press BOOT to wake");
  saveReadingPosition(true);
  setState(AppState::Sleeping, nowMs);
  Serial.flush();
  delay(200);

  display_.prepareForSleep();
  activeBookStore_.close();
  storage_.end();
  touch_.end();
  touchInitialized_ = false;

  BoardConfig::lightSleepUntilBootButton();
  wakeFromSleep();
}

void App::wakeFromSleep() {
  const uint32_t nowMs = millis();
  Serial.println("[app] woke from light sleep");

  BoardConfig::begin();
  button_.begin();
  powerButton_.begin();
  bootButtonReleasedSinceBoot_ = !button_.isHeld();
  bootButtonLongPressHandled_ = false;
  powerButtonReleasedSinceBoot_ = !powerButton_.isHeld();
  powerButtonLongPressHandled_ = false;
  powerOffStarted_ = false;
  updateBatteryStatus(nowMs, true);
  storage_.setStatusCallback(&App::handleStorageStatus, this);
  pausedTouch_.active = false;
  pausedTouchIntent_ = TouchIntent::None;
  wpmFeedbackVisible_ = false;
  menuScreen_ = MenuScreen::Main;
  lastStateLogMs_ = nowMs;
  state_ = AppState::Paused;

  const bool displayReady = display_.wakeFromSleep();
  touchInitialized_ = touch_.begin();
  storageReady_ = storage_.begin();

  if (storageReady_ && usingStorageBook_ && !currentBookPath_.isEmpty()) {
    const size_t resumeIndex = reader_.currentIndex();
    const int refreshedBookIndex = findBookIndexByPath(currentBookPath_);
    if (refreshedBookIndex >= 0 &&
        loadBookAtIndex(static_cast<size_t>(refreshedBookIndex), nowMs, false, false, false,
                        false)) {
      reader_.seekTo(resumeIndex);
    } else {
      Serial.println("[app] current indexed book unavailable after wake");
      usingStorageBook_ = false;
      currentBookPath_ = "";
      currentBookTitle_ = "Demo";
      reader_.clearLoadedBook(nowMs);
      reader_.begin(nowMs);
    }
  }

  if (displayReady) {
    renderActiveReader(nowMs);
  }
}

bool App::restoreSavedBook(uint32_t nowMs) {
  const String savedPath = preferences_.getString(kPrefBookPath, "");
  if (savedPath.isEmpty()) {
    return false;
  }

  const int bookIndex = findBookIndexByPath(savedPath);
  if (bookIndex < 0) {
    Serial.printf("[app] saved book not found: %s\n", savedPath.c_str());
    return false;
  }

  if (!loadBookAtIndex(static_cast<size_t>(bookIndex), nowMs, true, false, false, false)) {
    return false;
  }

  Serial.printf("[app] restored %s at word %u\n", savedPath.c_str(),
                static_cast<unsigned int>(reader_.currentIndex()));
  return true;
}

bool App::prepareBootBookLoad() {
  pendingBootBookIndex_ = 0;
  pendingBootBookLegacyFallback_ = false;

  if (!storageReady_ || storage_.bookCount() == 0) {
    return false;
  }

  const String savedPath = preferences_.getString(kPrefBookPath, "");
  if (!savedPath.isEmpty()) {
    const int savedBookIndex = findBookIndexByPath(savedPath);
    if (savedBookIndex >= 0) {
      pendingBootBookIndex_ = static_cast<size_t>(savedBookIndex);
      pendingBootBookLegacyFallback_ = true;
      Serial.printf("[app] deferred saved book load: %s\n", savedPath.c_str());
      return true;
    }

    Serial.printf("[app] saved book not found: %s\n", savedPath.c_str());
  }

  pendingBootBookIndex_ = 0;
  pendingBootBookLegacyFallback_ = false;
  Serial.println("[app] deferred first book load");
  return true;
}

void App::loadPendingBootBook(uint32_t nowMs) {
  if (!pendingBootBookLoad_ || state_ != AppState::Paused) {
    return;
  }

  pendingBootBookLoad_ = false;
  display_.renderStatus(tr(TrKey::LoadingBook), currentBookTitle_,
                        tr(TrKey::PleaseWait));
  const uint32_t startedMs = millis();
  const bool allowIndexBuild = pendingBootBookLegacyFallback_;
  const bool loaded = loadBookAtIndex(pendingBootBookIndex_, nowMs,
                                      pendingBootBookLegacyFallback_, allowIndexBuild, false,
                                      false);
  const uint32_t elapsedMs = millis() - startedMs;
  Serial.printf("[app] deferred book load %s in %lu ms\n", loaded ? "ok" : "failed",
                static_cast<unsigned long>(elapsedMs));

  if (loaded) {
    usingStorageBook_ = true;
    renderActiveReader(millis());
    return;
  }

  usingStorageBook_ = false;
  chapterMarkers_.clear();
  paragraphStarts_.clear();
  currentBookPath_ = "";
  currentBookTitle_ = "Demo";
  reader_.begin(millis());
  invalidateContextPreviewWindow();
  Serial.println("[app] using built-in demo text");
  renderActiveReader(millis());
}

void App::saveReadingPosition(bool force) {
  if (!usingStorageBook_ || currentBookPath_.isEmpty()) {
    return;
  }

  const size_t wordIndex = reader_.currentIndex();
  if (!force && wordIndex == lastSavedWordIndex_) {
    return;
  }

  preferences_.putString(kPrefBookPath, currentBookPath_);
  preferences_.putUInt(bookPositionKey(currentBookPath_).c_str(), static_cast<uint32_t>(wordIndex));
  preferences_.putUInt(bookWordCountKey(currentBookPath_).c_str(),
                       static_cast<uint32_t>(reader_.wordCount()));
  preferences_.putUInt(kPrefLegacyWordIndex, static_cast<uint32_t>(wordIndex));
  preferences_.putUShort(kPrefWpm, reader_.wpm());
  markBookRecent(currentBookPath_);
  lastSavedWordIndex_ = wordIndex;
  Serial.printf("[app] saved position word=%u book=%s\n", static_cast<unsigned int>(wordIndex),
                currentBookPath_.c_str());
}

bool App::loadBookAtIndex(size_t index, uint32_t nowMs, bool allowLegacyPositionFallback,
                          bool allowIndexBuild, bool allowEpubConversion,
                          bool rebuildTimeEstimate) {
  BookMetadata book;
  String loadedPath;
  size_t loadedIndex = index;
  const String initialLabel = storage_.bookDisplayName(index);
  renderStorageStatus("Opening book", initialLabel.c_str(),
                      allowIndexBuild ? "Checking index" : "Checking saved index", 5);
  if (!storage_.loadIndexedBook(index, activeBookStore_, book, &loadedPath, &loadedIndex,
                                allowIndexBuild, allowEpubConversion)) {
    return false;
  }

  const String loadedTitle = book.title.isEmpty() ? displayNameForPath(loadedPath) : book.title;
  renderStorageStatus("Opening book", loadedTitle.c_str(), "Loading word cache", 70);

  const bool keepingExistingTimeCache =
      !rebuildTimeEstimate && timeEstimateCacheValid_ && currentBookPath_ == loadedPath;
  reader_.setWordSource(&activeBookStore_, nowMs);
  if (reader_.wordCount() == 0 || reader_.currentWord().isEmpty()) {
    Serial.printf("[app] failed to read first indexed word from %s\n", loadedPath.c_str());
    activeBookStore_.close();
    reader_.clearLoadedBook(nowMs);
    renderStorageStatus("Book open failed", loadedTitle.c_str(), "Word cache unreadable", 100);
    return false;
  }

  chapterMarkers_ = std::move(book.chapters);
  paragraphStarts_ = std::move(book.paragraphStarts);
  invalidateContextPreviewWindow();
  currentBookIndex_ = loadedIndex;
  currentBookPath_ = loadedPath;
  currentBookTitle_ = loadedTitle;
  lastSavedWordIndex_ = static_cast<size_t>(-1);
  usingStorageBook_ = true;
  preferences_.putString(kPrefBookPath, currentBookPath_);
  preferences_.putUInt(bookWordCountKey(currentBookPath_).c_str(),
                       static_cast<uint32_t>(reader_.wordCount()));
  markBookRecent(currentBookPath_);

  const uint32_t savedWordIndex =
      savedWordIndexForBook(currentBookPath_, allowLegacyPositionFallback);
  if (savedWordIndex != kNoSavedWordIndex) {
    renderStorageStatus("Opening book", currentBookTitle_.c_str(), "Restoring position", 78);
    reader_.seekTo(savedWordIndex);
    lastSavedWordIndex_ = reader_.currentIndex();
    Serial.printf("[app] restored book position word=%u key=%s\n",
                  static_cast<unsigned int>(reader_.currentIndex()),
                  bookPositionKey(currentBookPath_).c_str());
  }

  if (rebuildTimeEstimate) {
    rebuildTimeEstimateCache();
  } else if (!keepingExistingTimeCache) {
    invalidateTimeEstimateCache();
  } else {
    renderStorageStatus("Opening book", currentBookTitle_.c_str(), "Using cached estimate", 92);
  }

  lastProgressSaveMs_ = nowMs;
  Serial.printf("[app] loaded SD book[%u/%u]: %s (%u chapters, %u paragraphs)\n",
                static_cast<unsigned int>(loadedIndex + 1),
                static_cast<unsigned int>(storage_.bookCount()), loadedPath.c_str(),
                static_cast<unsigned int>(chapterMarkers_.size()),
                static_cast<unsigned int>(paragraphStarts_.size()));
  return true;
}

String App::bookPositionKey(const String &bookPath) const {
  char key[10];
  std::snprintf(key, sizeof(key), "p%08lx", static_cast<unsigned long>(hashBookPath(bookPath)));
  return String(key);
}

String App::bookWordCountKey(const String &bookPath) const {
  char key[10];
  std::snprintf(key, sizeof(key), "c%08lx", static_cast<unsigned long>(hashBookPath(bookPath)));
  return String(key);
}

String App::bookRecentKey(const String &bookPath) const {
  char key[10];
  std::snprintf(key, sizeof(key), "r%08lx", static_cast<unsigned long>(hashBookPath(bookPath)));
  return String(key);
}

uint32_t App::nextRecentSequence() {
  uint32_t sequence = preferences_.getUInt(kPrefRecentSeq, 0);
  if (sequence == 0xFFFFFFFEUL) {
    sequence = 0;
  }
  ++sequence;
  preferences_.putUInt(kPrefRecentSeq, sequence);
  return sequence;
}

uint32_t App::bookRecentSequence(const String &bookPath) {
  return preferences_.getUInt(bookRecentKey(bookPath).c_str(), 0);
}

void App::markBookRecent(const String &bookPath) {
  if (bookPath.isEmpty()) {
    return;
  }

  preferences_.putUInt(bookRecentKey(bookPath).c_str(), nextRecentSequence());
}

uint32_t App::savedWordIndexForBook(const String &bookPath, bool allowLegacyFallback) {
  const String key = bookPositionKey(bookPath);
  if (preferences_.isKey(key.c_str())) {
    return preferences_.getUInt(key.c_str(), 0);
  }

  if (allowLegacyFallback && preferences_.isKey(kPrefLegacyWordIndex)) {
    const uint32_t legacyWordIndex = preferences_.getUInt(kPrefLegacyWordIndex, 0);
    preferences_.putUInt(key.c_str(), legacyWordIndex);
    Serial.printf("[app] migrated legacy position word=%u to key=%s\n",
                  static_cast<unsigned int>(legacyWordIndex), key.c_str());
    return legacyWordIndex;
  }

  return kNoSavedWordIndex;
}

bool App::bookProgressPercent(size_t bookIndex, uint8_t &percent) {
  size_t wordIndex = 0;
  size_t wordCount = 0;

  if (usingStorageBook_ && bookIndex == currentBookIndex_) {
    wordIndex = reader_.currentIndex();
    wordCount = reader_.wordCount();
  } else {
    const String path = storage_.bookPath(bookIndex);
    const String positionKey = bookPositionKey(path);
    const String countKey = bookWordCountKey(path);
    if (!preferences_.isKey(positionKey.c_str()) || !preferences_.isKey(countKey.c_str())) {
      return false;
    }

    wordIndex = preferences_.getUInt(positionKey.c_str(), 0);
    wordCount = preferences_.getUInt(countKey.c_str(), 0);
  }

  if (wordCount <= 1) {
    return false;
  }

  wordIndex = std::min(wordIndex, wordCount - 1);
  const size_t progress = (wordIndex * static_cast<size_t>(100)) / (wordCount - 1);
  percent = static_cast<uint8_t>(std::min(static_cast<size_t>(100), progress));
  return true;
}

int App::findBookIndexByPath(const String &path) const {
  for (size_t i = 0; i < storage_.bookCount(); ++i) {
    if (storage_.bookPath(i) == path) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void App::renderMenu() {
  if (!isFocusTimerMenuScreen(menuScreen_)) {
    applyReaderUiOrientation();
  }

  if (menuScreen_ == MenuScreen::SettingsHome || menuScreen_ == MenuScreen::SettingsDisplay ||
      menuScreen_ == MenuScreen::SettingsPacing || menuScreen_ == MenuScreen::WifiSettings ||
      menuScreen_ == MenuScreen::SettingsConnectivity ||
      menuScreen_ == MenuScreen::SettingsAbout || menuScreen_ == MenuScreen::ScreensaverSettings ||
      menuScreen_ == MenuScreen::WelcomeLanguage ||
      menuScreen_ == MenuScreen::WelcomeTheme ||
      menuScreen_ == MenuScreen::WelcomeHighlightColor ||
      menuScreen_ == MenuScreen::WelcomePacing ||
      menuScreen_ == MenuScreen::WelcomeConnect) {
    renderSettings();
  } else if (menuScreen_ == MenuScreen::Presets || menuScreen_ == MenuScreen::PresetsDeleteConfirm) {
    display_.renderMenu(settingsMenuItems_, presetsSelectedIndex_);
  } else if (menuScreen_ == MenuScreen::WifiNetworks) {
    renderWifiNetworks();
  } else if (menuScreen_ == MenuScreen::TextEntry) {
    renderTextEntry();
  } else if (menuScreen_ == MenuScreen::TypographyTuning) {
    renderTypographyTuning();
  } else if (menuScreen_ == MenuScreen::BookPicker) {
    renderBookPicker();
  } else if (menuScreen_ == MenuScreen::BookDetails) {
    renderBookDetails();
  } else if (menuScreen_ == MenuScreen::ChapterPicker) {
    renderChapterPicker();
  } else if (menuScreen_ == MenuScreen::SavePointsList) {
    renderSavePointsList();
  } else if (menuScreen_ == MenuScreen::PluginsList) {
    renderPluginsList();
  } else if (menuScreen_ == MenuScreen::RestartConfirm) {
    renderRestartConfirm();
  } else if (menuScreen_ == MenuScreen::SdCardRepairConfirm) {
    renderSdCardRepairConfirm();
  } else if (menuScreen_ == MenuScreen::UpdateConfirm) {
    renderUpdateConfirm();
  } else if (menuScreen_ == MenuScreen::FocusTimerGenres) {
    renderFocusTimerGenres();
  } else if (menuScreen_ == MenuScreen::FocusTimerSession) {
    renderFocusTimerSession();
  } else if (menuScreen_ == MenuScreen::TutorialStep1 ||
             menuScreen_ == MenuScreen::TutorialStep2 ||
             menuScreen_ == MenuScreen::TutorialStep3 ||
             menuScreen_ == MenuScreen::TutorialStep4 ||
             menuScreen_ == MenuScreen::TutorialStep5) {
    renderTutorialStep();
  } else {
    renderMainMenu();
  }
}

void App::renderMainMenu() {
  std::vector<String> items;
  items.reserve(MenuItemCount);
  items.push_back(uiText(UiText::Read));
  items.push_back(uiText(UiText::Library));
  items.push_back(uiText(UiText::SavePoints));
  items.push_back(uiText(UiText::Settings));
  items.push_back(uiText(UiText::Plugins));
  items.push_back(uiText(UiText::PowerOff));
  display_.renderMenu(items, menuSelectedIndex_);
}

void App::renderSettings() {
  if (settingsMenuItems_.empty()) {
    rebuildSettingsMenuItems();
  }

  // Show "?" indicator on selected item only if help is available
  std::vector<String> renderItems = settingsMenuItems_;
  if (showHelpHints_ && settingsSelectedIndex_ < renderItems.size() && settingsSelectedIndex_ > 0) {
    bool hasHelp = false;
    if (menuScreen_ == MenuScreen::SettingsDisplay) {
      hasHelp = HelpTexts::getDisplayHelp(settingsSelectedIndex_ - 1) != nullptr;
    } else if (menuScreen_ == MenuScreen::SettingsPacing) {
      hasHelp = (readerMode_ == ReaderMode::Scroll)
        ? HelpTexts::getPacingScrollHelp(settingsSelectedIndex_ - 1) != nullptr
        : HelpTexts::getPacingHelp(settingsSelectedIndex_ - 1) != nullptr;
    }
    if (hasHelp) {
      renderItems[settingsSelectedIndex_] += " ?";
    }
  }

  display_.renderMenu(renderItems, settingsSelectedIndex_);
}

void App::renderTypographyTuning() {
  if (kTypographyPreviewWordCount == 0) {
    display_.renderStatus(uiText(UiText::Typography), uiText(UiText::NoSamples), "");
    return;
  }

  if (typographyPreviewSampleIndex_ >= kTypographyPreviewWordCount) {
    typographyPreviewSampleIndex_ = 0;
  }
  if (typographyTuningSelectedIndex_ >= TypographyTuningItemCount) {
    typographyTuningSelectedIndex_ = TypographyTuningFontSize;
  }

  const size_t index = typographyPreviewSampleIndex_;
  const size_t beforeIndex =
      index == 0 ? kTypographyPreviewWordCount - 1 : index - 1;
  const size_t afterIndex =
      (index + 1 >= kTypographyPreviewWordCount) ? 0 : index + 1;
  const String beforeText = phantomWordsEnabled_ ? kTypographyPreviewWords[beforeIndex] : "";
  const String afterText = phantomWordsEnabled_ ? kTypographyPreviewWords[afterIndex] : "";
  const String line1 = typographyTuningLabel() + ": " + typographyTuningValueLabel();
  const String title =
      uiText(UiText::Typography) + " " + String(static_cast<unsigned int>(index + 1)) + "/" +
      String(static_cast<unsigned int>(kTypographyPreviewWordCount));
  String line2 = uiText(UiText::TapChangeSample);
  if (typographyTuningSelectedIndex_ == TypographyTuningBack) {
    line2 = uiText(UiText::TapExitSample);
  } else if (typographyTuningSelectedIndex_ == TypographyTuningPhantomWords ||
             typographyTuningSelectedIndex_ == TypographyTuningFocusHighlight) {
    line2 = uiText(UiText::TapToggleSample);
  } else if (typographyTuningSelectedIndex_ == TypographyTuningFontSize ||
             typographyTuningSelectedIndex_ == TypographyTuningTypeface) {
    line2 = uiText(UiText::TapCycleSample);
  } else if (typographyTuningSelectedIndex_ == TypographyTuningReset) {
    line2 = uiText(UiText::TapToReset);
  }

  display_.renderTypographyPreview(beforeText,
                                   kTypographyPreviewWords[index],
                                   afterText,
                                   readerFontSizeIndex_, title, line1, line2);
}

void App::renderBookPicker() {
  display_.renderLibrary(bookMenuItems_, bookPickerSelectedIndex_);
}

void App::renderChapterPicker() {
  display_.renderMenu(chapterMenuItems_, chapterPickerSelectedIndex_);
}

void App::renderRestartConfirm() {
  std::vector<String> items;
  items.reserve(RestartConfirmItemCount);
  items.push_back(uiText(UiText::AreYouSure));
  items.push_back(uiText(UiText::NoKeepPlace));
  items.push_back(uiText(UiText::YesRestart));

  display_.renderMenu(items, restartConfirmSelectedIndex_ + kRestartConfirmHeaderRows);
}

void App::renderSdCardRepairConfirm() {
  std::vector<String> items;
  items.reserve(SdCardRepairConfirmItemCount + kSdCardRepairConfirmHeaderRows);
  items.push_back(tr2(TrKey2::RepairFolders));
  items.push_back(tr2(TrKey2::NotNow));
  items.push_back(tr2(TrKey2::CreateFolders));

  display_.renderMenu(items, sdCardRepairConfirmSelectedIndex_ + kSdCardRepairConfirmHeaderRows);
}

void App::renderUpdateConfirm() {
  std::vector<String> items;
  items.reserve(UpdateConfirmItemCount + kUpdateConfirmHeaderRows);
  items.push_back(tr2(TrKey2::UpdateAvailable));
  items.push_back(pendingUpdateCurrentVersion_ + " -> " + pendingUpdateNewVersion_);
  items.push_back(tr2(TrKey2::SkipForNow));
  items.push_back(tr2(TrKey2::Update));

  display_.renderMenu(items, updateConfirmSelectedIndex_ + kUpdateConfirmHeaderRows);
}

void App::renderFocusTimerGenres() {
  applyReaderUiOrientation();
  if (focusTimerGenreMenuItems_.empty()) {
    rebuildFocusTimerGenreMenuItems();
  }
  display_.renderMenu(focusTimerGenreMenuItems_, focusTimerGenreSelectedIndex_);
}

void App::renderFocusTimerSession() {
  applyUiOrientation(focusTimer_.uiOrientation());
  const String remainingLabel = formatFocusTimerRemaining(millis());

  switch (focusTimer_.state()) {
    case FocusTimer::State::Unavailable:
      display_.renderFocusTimerScreen("TIMER", "", "", "IMU unavailable");
      return;
    case FocusTimer::State::GenreSelect:
      renderFocusTimerGenres();
      return;
    case FocusTimer::State::WaitForTouchStart:
      display_.renderFocusTimerScreen("BEGIN", "", "", "Place on short side");
      return;
    case FocusTimer::State::TouchRunning:
      display_.renderFocusTimerScreen("BEGIN", "", remainingLabel, "",
                                      "", focusTimer_.progressPercent(millis()));
      return;
    case FocusTimer::State::WaitAfterTouch:
      display_.renderFocusTimerScreen("WORK", "", "", "Flip to continue");
      return;
    case FocusTimer::State::WorkRunning:
      display_.renderFocusTimerScreen("WORK", "", remainingLabel, "",
                                      "", focusTimer_.progressPercent(millis()));
      return;
    case FocusTimer::State::BreakRunning:
      display_.renderFocusTimerScreen("BREAK", "", remainingLabel, "",
                                      "", focusTimer_.progressPercent(millis()), true);
      return;
    case FocusTimer::State::WaitAfterWork:
      display_.renderFocusTimerScreen("BREAK", "", "", "Turn for break", "",
                                      -1, true);
      return;
    case FocusTimer::State::WaitAfterBreak:
      display_.renderFocusTimerScreen("WORK", "", "", "Flip to begin");
      return;
    case FocusTimer::State::Cancelled:
      display_.renderFocusTimerScreen("BEGIN", "", "", "Place to begin again");
      return;
    case FocusTimer::State::Complete:
      display_.renderFocusTimerScreen("DONE", "", "", "Session complete");
      return;
  }
}

bool App::updateChapterTransition(uint32_t nowMs) {
  if (!chapterTransitionVisible_) {
    return false;
  }

  if (nowMs < chapterTransitionUntilMs_) {
    return true;
  }

  chapterTransitionVisible_ = false;
  reader_.start(nowMs);
  renderActiveReader(nowMs);
  return true;
}

bool App::maybeStartChapterTransition(size_t previousWordIndex, size_t currentWordIndex,
                                      uint32_t nowMs) {
  if (chapterMarkers_.empty() || currentWordIndex <= previousWordIndex) {
    return false;
  }

  for (size_t i = 0; i < chapterMarkers_.size(); ++i) {
    const size_t chapterWordIndex = chapterMarkers_[i].wordIndex;
    if (chapterWordIndex == 0 || chapterWordIndex <= previousWordIndex ||
        chapterWordIndex > currentWordIndex) {
      continue;
    }

    chapterTransitionIndex_ = i;
    chapterTransitionVisible_ = true;
    chapterTransitionUntilMs_ = nowMs + kChapterTransitionMs;
    contextViewVisible_ = false;
    wpmFeedbackVisible_ = false;
    reader_.seekTo(chapterWordIndex);
    renderChapterTransition();
    Serial.printf("[chapter] transition %u/%u word=%u title=%s\n",
                  static_cast<unsigned int>(i + 1),
                  static_cast<unsigned int>(chapterMarkers_.size()),
                  static_cast<unsigned int>(chapterWordIndex),
                  chapterMarkers_[i].title.c_str());
    return true;
  }

  return false;
}

void App::renderChapterTransition() {
  if (!chapterTransitionVisible_ || chapterTransitionIndex_ >= chapterMarkers_.size()) {
    return;
  }

  applyReaderUiOrientation();
  const String title = String("CHAPTER ") + String(chapterTransitionIndex_ + 1);
  String subtitle = chapterMarkers_[chapterTransitionIndex_].title;
  if (subtitle.length() > 42) {
    subtitle = subtitle.substring(0, 42) + "...";
  }
  display_.renderStatus(title, subtitle, "");
}

DisplayManager::LibraryItem App::libraryItemForBook(size_t bookIndex) {
  DisplayManager::LibraryItem item;
  item.title = storage_.bookDisplayName(bookIndex);
  item.subtitle = storage_.bookAuthorName(bookIndex);

  uint8_t percent = 0;
  const bool hasProgress = bookProgressPercent(bookIndex, percent);
  if (hasProgress) {
    if (!item.subtitle.isEmpty()) {
      item.subtitle += " - ";
    }
    item.subtitle += String(percent) + "%";
  }

  if (item.subtitle.isEmpty() && usingStorageBook_ && bookIndex == currentBookIndex_) {
    item.subtitle = uiText(UiText::CurrentBook);
  }

  return item;
}

String App::chapterMenuLabel(size_t chapterIndex) const {
  if (chapterIndex >= chapterMarkers_.size()) {
    return "";
  }

  String label = String(chapterIndex + 1) + " " + chapterMarkers_[chapterIndex].title;
  if (label.length() > 36) {
    label = label.substring(0, 36) + "...";
  }

  const size_t currentIndex = reader_.currentIndex();
  const size_t startIndex = chapterMarkers_[chapterIndex].wordIndex;
  const size_t endIndex = (chapterIndex + 1 < chapterMarkers_.size())
                              ? chapterMarkers_[chapterIndex + 1].wordIndex
                              : reader_.wordCount();
  if (currentIndex >= startIndex && currentIndex < endIndex) {
    label += " *";
  }
  return label;
}

size_t App::currentChapterIndex() const {
  if (chapterMarkers_.empty()) {
    return static_cast<size_t>(-1);
  }

  size_t currentChapter = 0;
  const size_t currentIndex = reader_.currentIndex();
  for (size_t i = 0; i < chapterMarkers_.size(); ++i) {
    if (chapterMarkers_[i].wordIndex <= currentIndex) {
      currentChapter = i;
    }
  }

  return currentChapter;
}

String App::currentChapterLabel() const {
  const size_t chapterIndex = currentChapterIndex();
  if (chapterIndex >= chapterMarkers_.size()) {
    return currentBookTitle_.isEmpty() ? uiText(UiText::Start) : currentBookTitle_;
  }

  return chapterMarkers_[chapterIndex].title;
}

String App::currentFooterMetricLabel() const {
  if (footerMetricMode_ == FooterMetricMode::Percentage) {
    return String(readingProgressPercent()) + "%";
  }

  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0) {
    return "0%";
  }

  const size_t currentIndex = std::min(reader_.currentIndex(), wordCount - 1);
  size_t endIndex = wordCount;
  const bool generatingEstimate = accurateTimeEstimateEnabled_ && timeEstimateBuildInProgress_ &&
                                  timeEstimateBuildMatchesCurrentBook();
  const int generatingPercent =
      generatingEstimate
          ? static_cast<int>((timeEstimateBuildNextBlock_ * 100UL) /
                             std::max<size_t>(1, timeEstimateBuildBlockCount_))
          : 0;

  if (footerMetricMode_ == FooterMetricMode::ChapterTime) {
    const size_t chapterIndex = currentChapterIndex();
    if (chapterIndex < chapterMarkers_.size() && chapterIndex + 1 < chapterMarkers_.size()) {
      endIndex = chapterMarkers_[chapterIndex + 1].wordIndex;
    }
    if (generatingEstimate) {
      return String("CH ") + String(generatingPercent) + "% gen";
    }
    return String("CH ") +
           formatReadingTimeRemaining(estimatedReadingTimeRemainingMs(currentIndex, endIndex));
  }

  if (generatingEstimate) {
    return String("BOOK ") + String(generatingPercent) + "% gen";
  }
  return String("BOOK ") +
         formatReadingTimeRemaining(estimatedReadingTimeRemainingMs(currentIndex, endIndex));
}

String App::currentBatteryLabel() const {
  if (!batteryPresent_ || !batterySampleInitialized_) {
    return "";
  }

  if (batteryLabelMode_ == BatteryLabelMode::TimeRemaining) {
    return batteryTimeRemainingLabel();
  }

  if (batteryLabelMode_ == BatteryLabelMode::Voltage) {
    return batteryVoltageLabel();
  }

  return String(static_cast<unsigned int>(batteryDisplayedPercent_)) + "%";
}

String App::footerMetricModeLabel() const {
  switch (footerMetricMode_) {
    case FooterMetricMode::ChapterTime:
      return tr(TrKey::ChapterTime);
    case FooterMetricMode::BookTime:
      return tr(TrKey::BookTime);
    case FooterMetricMode::Percentage:
    default:
      return tr(TrKey::PercentRead);
  }
}

String App::batteryLabelModeLabel() const {
  switch (batteryLabelMode_) {
    case BatteryLabelMode::TimeRemaining:
      return tr(TrKey::TimeRemaining);
    case BatteryLabelMode::Voltage:
      return tr(TrKey::Voltage);
    case BatteryLabelMode::Percent:
    default:
      return tr(TrKey::Percentage);
  }
}

String App::screensaverModeLabel() const {
  switch (screensaverMode_) {
    case ScreensaverMode::Maze:
      return tr(TrKey::Maze);
    case ScreensaverMode::Voronoi:
      return "Voronoi";
    case ScreensaverMode::Stars:
      return tr(TrKey::Stars);
    case ScreensaverMode::Matrix:
      return tr(TrKey::MatrixRain);
    case ScreensaverMode::ScreenOff:
      return tr(TrKey::ScreenOff);
    case ScreensaverMode::Life:
    default:
      return tr(TrKey::Life);
  }
}

String App::batteryTimeRemainingLabel() const {
  if (batteryRuntimeEstimateReady_) {
    return formatBatteryTimeRemaining(batteryRuntimeMinutesRemaining_);
  }

  const uint32_t estimatedMinutes =
      (static_cast<uint32_t>(batteryDisplayedPercent_) * kNominalBatteryRuntimeMinutes) / 100UL;
  return formatBatteryTimeRemaining(estimatedMinutes);
}

String App::batteryVoltageLabel() const { return String(batteryFilteredVoltage_, 2) + "V"; }

String App::formatBatteryTimeRemaining(uint32_t minutes) const {
  if (minutes < 1) {
    return "0m";
  }

  if (minutes < 60) {
    return String(minutes) + "m";
  }

  const uint32_t hours = minutes / 60;
  const uint32_t remainder = minutes % 60;
  if (hours >= 10 || remainder < 10) {
    return String(hours) + "h";
  }

  return String(hours) + "h" + String(remainder / 10) + "0";
}

uint32_t App::estimatedReadingTimeRemainingMs(size_t startIndex, size_t endIndex) const {
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0 || reader_.wpm() == 0) {
    return 0;
  }

  startIndex = std::min(startIndex, wordCount);
  endIndex = std::min(endIndex, wordCount);
  if (endIndex <= startIndex) {
    return 0;
  }

  const uint32_t baseMs = static_cast<uint32_t>(
      (static_cast<uint64_t>(endIndex - startIndex) * 60000ULL) /
      static_cast<uint64_t>(reader_.wpm()));

  if (!accurateTimeEstimateEnabled_ || !timeEstimateCacheValid_) {
    return baseMs;
  }

  return baseMs + estimatedPacingBonusMs(startIndex, endIndex);
}

uint32_t App::estimatedPacingBonusMs(size_t startIndex, size_t endIndex) const {
  if (!timeEstimateCacheValid_ || wordBonusBlockPrefixSumMs_.empty() ||
      endIndex <= startIndex) {
    return 0;
  }

  const size_t wordCount = reader_.wordCount();
  startIndex = std::min(startIndex, wordCount);
  endIndex = std::min(endIndex, wordCount);
  if (endIndex <= startIndex) {
    return 0;
  }

  const size_t firstFullBlock = (startIndex + kTimeEstimateBlockWords - 1) /
                                kTimeEstimateBlockWords;
  const size_t lastFullBlockEnd = endIndex / kTimeEstimateBlockWords;
  uint32_t bonusMs = 0;

  if (firstFullBlock < lastFullBlockEnd &&
      lastFullBlockEnd < wordBonusBlockPrefixSumMs_.size()) {
    const size_t startPartialEnd =
        std::min(endIndex, firstFullBlock * kTimeEstimateBlockWords);
    for (size_t i = startIndex; i < startPartialEnd; ++i) {
      bonusMs += reader_.wordPacingBonusMsAt(i);
    }

    bonusMs += wordBonusBlockPrefixSumMs_[lastFullBlockEnd] -
               wordBonusBlockPrefixSumMs_[firstFullBlock];

    const size_t endPartialStart = lastFullBlockEnd * kTimeEstimateBlockWords;
    for (size_t i = endPartialStart; i < endIndex; ++i) {
      bonusMs += reader_.wordPacingBonusMsAt(i);
    }
    return bonusMs;
  }

  for (size_t i = startIndex; i < endIndex; ++i) {
    bonusMs += reader_.wordPacingBonusMsAt(i);
  }
  return bonusMs;
}

void App::invalidateTimeEstimateCache() {
  cancelTimeEstimateBuild();
  timeEstimateCacheValid_ = false;
  std::vector<uint32_t>().swap(wordBonusBlockPrefixSumMs_);
}

void App::rebuildTimeEstimateCache() {
  invalidateTimeEstimateCache();
  pacingCacheDirty_ = false;
  if (!accurateTimeEstimateEnabled_) {
    if (!currentBookTitle_.isEmpty()) {
      renderStorageStatus("Reading time", currentBookTitle_.c_str(), "Fast estimate enabled",
                          100);
    }
    return;
  }

  const size_t n = reader_.wordCount();
  if (n == 0) {
    return;
  }

  const String label = currentBookTitle_.isEmpty() ? String("Current book") : currentBookTitle_;
  timeEstimateBuildWordCount_ = n;
  timeEstimateBuildBlockCount_ =
      (timeEstimateBuildWordCount_ + kTimeEstimateBlockWords - 1) / kTimeEstimateBlockWords;
  if (timeEstimateBuildBlockCount_ == 0) {
    return;
  }

  wordBonusBlockPrefixSumMs_.assign(timeEstimateBuildBlockCount_ + 1, 0);
  timeEstimateBuildBookPath_ = currentBookPath_;
  timeEstimateBuildNextBlock_ = 0;
  timeEstimateBuildRunningMs_ = 0;
  timeEstimateBuildStartedMs_ = millis();
  timeEstimateBuildLastLogMs_ = timeEstimateBuildStartedMs_;
  timeEstimateBuildInProgress_ = true;

  const String detail = String(static_cast<unsigned int>(n)) + " words in background";
  renderStorageStatus("Reading time", label.c_str(), detail.c_str(), 0);
  Serial.printf("[time-est] background build started words=%u blocks=%u book=%s\n",
                static_cast<unsigned int>(timeEstimateBuildWordCount_),
                static_cast<unsigned int>(timeEstimateBuildBlockCount_),
                currentBookPath_.c_str());
}

void App::cancelTimeEstimateBuild() {
  timeEstimateBuildInProgress_ = false;
  timeEstimateBuildBookPath_ = "";
  timeEstimateBuildWordCount_ = 0;
  timeEstimateBuildBlockCount_ = 0;
  timeEstimateBuildNextBlock_ = 0;
  timeEstimateBuildRunningMs_ = 0;
  timeEstimateBuildStartedMs_ = 0;
  timeEstimateBuildLastLogMs_ = 0;
}

bool App::timeEstimateBuildMatchesCurrentBook() const {
  return timeEstimateBuildInProgress_ && timeEstimateBuildBookPath_ == currentBookPath_ &&
         timeEstimateBuildWordCount_ == reader_.wordCount();
}

void App::updateTimeEstimateBuild(uint32_t nowMs) {
  if (!timeEstimateBuildInProgress_) {
    return;
  }

  if (!accurateTimeEstimateEnabled_ || !timeEstimateBuildMatchesCurrentBook()) {
    Serial.println("[time-est] background build cancelled");
    invalidateTimeEstimateCache();
    return;
  }

  if (state_ == AppState::Playing || state_ == AppState::CompanionSync ||
      state_ == AppState::UsbTransfer || state_ == AppState::Standby ||
      state_ == AppState::Sleeping) {
    return;
  }

  size_t processedBlocks = 0;
  while (timeEstimateBuildNextBlock_ < timeEstimateBuildBlockCount_ &&
         processedBlocks < kTimeEstimateBlocksPerUpdate) {
    const size_t block = timeEstimateBuildNextBlock_;
    wordBonusBlockPrefixSumMs_[block] = timeEstimateBuildRunningMs_;
    const size_t blockStart = block * kTimeEstimateBlockWords;
    const size_t blockEnd =
        std::min(timeEstimateBuildWordCount_, blockStart + kTimeEstimateBlockWords);
    for (size_t i = blockStart; i < blockEnd; ++i) {
      timeEstimateBuildRunningMs_ += reader_.wordPacingBonusMsAt(i);
    }
    ++timeEstimateBuildNextBlock_;
    ++processedBlocks;
    delay(0);
  }

  if (timeEstimateBuildNextBlock_ >= timeEstimateBuildBlockCount_) {
    wordBonusBlockPrefixSumMs_[timeEstimateBuildBlockCount_] = timeEstimateBuildRunningMs_;
    timeEstimateCacheValid_ = true;
    const uint32_t elapsedMs = millis() - timeEstimateBuildStartedMs_;
    Serial.printf("[time-est] background cached %u words in %u blocks bonus=%lums took=%lums\n",
                  static_cast<unsigned int>(timeEstimateBuildWordCount_),
                  static_cast<unsigned int>(timeEstimateBuildBlockCount_),
                  static_cast<unsigned long>(timeEstimateBuildRunningMs_),
                  static_cast<unsigned long>(elapsedMs));
    cancelTimeEstimateBuild();
    if (state_ == AppState::Paused || state_ == AppState::Playing) {
      renderActiveReader(nowMs);
    } else if (state_ == AppState::Menu) {
      renderMenu();
    }
    return;
  }

  if (nowMs - timeEstimateBuildLastLogMs_ >= kTimeEstimateProgressLogMs) {
    const int progress =
        static_cast<int>((timeEstimateBuildNextBlock_ * 100UL) /
                         std::max<size_t>(1, timeEstimateBuildBlockCount_));
    Serial.printf("[time-est] background progress %u/%u blocks (%d%%)\n",
                  static_cast<unsigned int>(timeEstimateBuildNextBlock_),
                  static_cast<unsigned int>(timeEstimateBuildBlockCount_), progress);
    timeEstimateBuildLastLogMs_ = nowMs;
    if (state_ == AppState::Paused) {
      renderActiveReader(nowMs);
    }
  }
}

String App::timeEstimateModeLabel() const {
  return uiText(accurateTimeEstimateEnabled_ ? UiText::TimeEstimateAccurate
                                             : UiText::TimeEstimateFast);
}

String App::formatReadingTimeRemaining(uint32_t remainingMs) const {
  const uint32_t totalSeconds = remainingMs / 1000UL;
  if (totalSeconds < 60UL) {
    return "0m";
  }

  const uint32_t totalMinutes = totalSeconds / 60UL;
  if (totalMinutes < 60UL) {
    return String(totalMinutes) + "m";
  }

  const uint32_t totalHours = totalMinutes / 60UL;
  const uint32_t minutes = totalMinutes % 60UL;
  if (totalHours < 24UL) {
    if (minutes == 0) {
      return String(totalHours) + "h";
    }
    return String(totalHours) + "h" + String(minutes) + "m";
  }

  const uint32_t days = totalHours / 24UL;
  const uint32_t hours = totalHours % 24UL;
  if (hours == 0) {
    return String(days) + "d";
  }
  return String(days) + "d" + String(hours) + "h";
}

uint8_t App::readingProgressPercent() const {
  const size_t count = reader_.wordCount();
  if (count <= 1) {
    return 0;
  }

  const size_t index = std::min(reader_.currentIndex(), count - 1);
  const size_t percent = (index * 100UL) / (count - 1);
  return static_cast<uint8_t>(std::min(static_cast<size_t>(100), percent));
}

bool App::isFocusTimerMenuScreen(MenuScreen screen) const {
  return screen == MenuScreen::FocusTimerGenres || screen == MenuScreen::FocusTimerSession;
}

void App::applyUiOrientation(BoardConfig::UiOrientation orientation) {
  touch_.setUiOrientation(orientation);
  display_.setUiOrientation(orientation);
}

void App::applyReaderUiOrientation() {
  applyUiOrientation(readerUiOrientation());
}

BoardConfig::UiOrientation App::readerUiOrientation() const {
  return uiRotated180() ? BoardConfig::UiOrientation::LandscapeFlipped
                        : BoardConfig::UiOrientation::Landscape;
}

String App::formatFocusTimerRemaining(uint32_t nowMs) const {
  const uint32_t remainingMs = focusTimer_.remainingMs(nowMs);
  const uint32_t totalSeconds = remainingMs / 1000UL;
  const uint32_t minutes = totalSeconds / 60UL;
  const uint32_t seconds = totalSeconds % 60UL;
  char buffer[8];
  std::snprintf(buffer, sizeof(buffer), "%02lu:%02lu",
                static_cast<unsigned long>(minutes),
                static_cast<unsigned long>(seconds));
  return String(buffer);
}

String App::focusTimerCountsLabel() const {
  return "T" + String(focusTimer_.completedTouchBlocks()) + " W" +
         String(focusTimer_.completedWorkBlocks()) + " B" +
         String(focusTimer_.completedBreakBlocks());
}

void App::playFocusTimerCompletionCue() {
  if (audio_.beep()) {
    return;
  }

  for (int i = 0; i < 3; ++i) {
    digitalWrite(BoardConfig::PIN_LCD_BACKLIGHT, HIGH);
    delay(55);
    digitalWrite(BoardConfig::PIN_LCD_BACKLIGHT, LOW);
    delay(45);
  }
}

bool App::scrollModeEnabled() const { return readerMode_ == ReaderMode::Scroll; }

bool App::uiRotated180() const {
  // Always keep the same screen orientation regardless of handedness.
  // Left/right hand setting only affects touch zones and anchor offset,
  // not physical screen rotation.
  return BoardConfig::UI_ROTATED_180;
}

uint8_t App::effectiveAnchorPercent() const {
  return handednessMode_ == HandednessMode::Left
             ? static_cast<uint8_t>(typographyConfig_.anchorPercent + kLeftHandAnchorOffset)
             : typographyConfig_.anchorPercent;
}

DisplayManager::TypographyConfig App::effectiveTypographyConfig() const {
  DisplayManager::TypographyConfig config = typographyConfig_;
  config.anchorPercent = effectiveAnchorPercent();
  return config;
}

uint32_t App::currentReaderContentToken() const {
  return hashBookPath(currentBookPath_.isEmpty() ? String("__demo__") : currentBookPath_);
}

size_t App::phantomBeforeCharTarget() const {
  uint8_t levelIndex = readerFontSizeIndex_;
  if (levelIndex >= kReaderFontSizeCount) {
    levelIndex = 0;
  }
  return kPhantomBeforeCharTargets[levelIndex];
}

size_t App::phantomAfterCharTarget() const {
  uint8_t levelIndex = readerFontSizeIndex_;
  if (levelIndex >= kReaderFontSizeCount) {
    levelIndex = 0;
  }
  return kPhantomAfterCharTargets[levelIndex];
}

String App::collectPhantomBeforeText(size_t currentIndex, size_t charTarget) const {
  if (currentIndex == 0 || charTarget == 0) {
    return "";
  }

  size_t startIndex = currentIndex;
  size_t totalChars = 0;
  while (startIndex > 0 && totalChars < charTarget) {
    --startIndex;
    const String word = reader_.wordAt(startIndex);
    totalChars += word.length();
    if (startIndex + 1 < currentIndex) {
      ++totalChars;
    }
  }

  String text;
  for (size_t index = startIndex; index < currentIndex; ++index) {
    if (!text.isEmpty()) {
      text += ' ';
    }
    text += reader_.wordAt(index);
  }
  return text;
}

String App::collectPhantomAfterText(size_t currentIndex, size_t charTarget) const {
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0 || currentIndex + 1 >= wordCount || charTarget == 0) {
    return "";
  }

  size_t endIndex = currentIndex + 1;
  size_t totalChars = 0;
  while (endIndex < wordCount && totalChars < charTarget) {
    const String word = reader_.wordAt(endIndex);
    totalChars += word.length();
    if (endIndex > currentIndex + 1) {
      ++totalChars;
    }
    ++endIndex;
  }

  String text;
  for (size_t index = currentIndex + 1; index < endIndex; ++index) {
    if (!text.isEmpty()) {
      text += ' ';
    }
    text += reader_.wordAt(index);
  }
  return text;
}

String App::phantomBeforeText() const {
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0) {
    return "";
  }

  const size_t currentIndex = std::min(reader_.currentIndex(), wordCount - 1);
  return collectPhantomBeforeText(currentIndex, phantomBeforeCharTarget());
}

String App::phantomAfterText() const {
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0) {
    return "";
  }

  const size_t currentIndex = std::min(reader_.currentIndex(), wordCount - 1);
  return collectPhantomAfterText(currentIndex, phantomAfterCharTarget());
}

void App::renderActiveReader(uint32_t nowMs) {
  if (pendingBootBookLoad_) {
    display_.renderStatus(tr(TrKey::LoadingBook), currentBookTitle_,
                        tr(TrKey::PleaseWait));
    return;
  }

  if (!ensureCurrentBookWordAvailable(nowMs)) {
    return;
  }

  if (chapterTransitionVisible_) {
    renderChapterTransition();
    return;
  }

  applyReaderUiOrientation();
  if (scrollModeEnabled()) {
    if (wpmFeedbackVisible_) {
      renderScrollReader(nowMs, String(reader_.wpm()) + " WPM");
    } else {
      renderScrollReader(nowMs);
    }
    return;
  }

  if (contextViewVisible_) {
    renderContextPreview();
  } else if (wpmFeedbackVisible_) {
    renderWpmFeedback(nowMs);
  } else {
    renderReaderWord();
  }
}

bool App::ensureCurrentBookWordAvailable(uint32_t nowMs) {
  if (!usingStorageBook_ || reader_.wordCount() == 0 || !reader_.currentWord().isEmpty()) {
    return true;
  }

  handleCurrentBookReadFailure(nowMs, "Word cache unreadable");
  return false;
}

void App::handleCurrentBookReadFailure(uint32_t nowMs, const char *detail) {
  const String failedTitle = currentBookTitle_.isEmpty() ? String("Current book") : currentBookTitle_;
  const String failedPath = currentBookPath_;
  const bool articlesOnly =
      currentBookIndex_ < storage_.bookCount() && storage_.bookIsArticle(currentBookIndex_);

  Serial.printf("[app] active book read failed word=%u book=%s detail=%s\n",
                static_cast<unsigned int>(reader_.currentIndex()), failedPath.c_str(),
                detail == nullptr ? "" : detail);

  saveReadingPosition(true);
  activeBookStore_.close();
  reader_.clearLoadedBook(nowMs);
  chapterMarkers_.clear();
  paragraphStarts_.clear();
  currentBookPath_ = "";
  currentBookTitle_ = "Demo";
  usingStorageBook_ = false;
  contextViewVisible_ = false;
  wpmFeedbackVisible_ = false;
  invalidateContextPreviewWindow();
  invalidateTimeEstimateCache();

  setState(AppState::Menu, nowMs);
  display_.renderStatus("Book read failed", failedTitle,
                        detail == nullptr ? "Reopen from library" : detail);
  delay(1800);
  openBookPicker(articlesOnly);
}

void App::renderReaderWord() {
  applyReaderUiOrientation();
  contextViewVisible_ = false;
  const String beforeText = phantomWordsEnabled_ ? phantomBeforeText() : "";
  const String afterText = phantomWordsEnabled_ ? phantomAfterText() : "";
  const DisplayManager::ReaderChrome chrome = readerChrome();
  const bool showReaderFooter = readerFooterVisible();
  const String footerMetricLabel = readerFooterStatusLabel();
  display_.renderPhantomRsvpWord(beforeText, reader_.currentWord(), afterText,
                                 readerFontSizeIndex_, currentChapterLabel(),
                                 readingProgressPercent(), showReaderFooter, footerMetricLabel,
                                 chrome);
}

bool App::isParagraphStart(size_t wordIndex) const {
  if (wordIndex == 0) {
    return true;
  }

  return std::binary_search(paragraphStarts_.begin(), paragraphStarts_.end(), wordIndex);
}

size_t App::paragraphStartAtOrBefore(size_t wordIndex) const {
  if (wordIndex == 0 || paragraphStarts_.empty()) {
    return 0;
  }

  const auto it = std::upper_bound(paragraphStarts_.begin(), paragraphStarts_.end(), wordIndex);
  if (it == paragraphStarts_.begin()) {
    return 0;
  }

  return *std::prev(it);
}

size_t App::contextPreviewAnchorIndex(size_t currentIndex) const {
  if (currentIndex <= kContextPreviewAnchorLeadWords) {
    return 0;
  }

  const size_t anchorTarget = currentIndex - kContextPreviewAnchorLeadWords;
  const size_t paragraphStart = paragraphStartAtOrBefore(anchorTarget);
  if (anchorTarget - paragraphStart <= kContextPreviewMaxParagraphSnapWords) {
    return paragraphStart;
  }

  return anchorTarget;
}

void App::updateContextPreviewWindow(size_t currentIndex) {
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0) {
    contextPreviewWords_.clear();
    contextPreviewWindowValid_ = false;
    contextPreviewCurrentLocalIndex_ = static_cast<size_t>(-1);
    return;
  }

  size_t startIndex = contextPreviewStartIndex_;
  size_t endIndex = 0;
  bool rebuildWindow = !contextPreviewWindowValid_ || contextPreviewWords_.empty();
  if (!rebuildWindow) {
    endIndex = std::min(wordCount, startIndex + kContextPreviewWindowWords);
    rebuildWindow = currentIndex < startIndex || currentIndex >= endIndex ||
                    (currentIndex + 1 >= endIndex && endIndex < wordCount);
  }

  if (rebuildWindow) {
    startIndex = contextPreviewAnchorIndex(currentIndex);
    endIndex = std::min(wordCount, startIndex + kContextPreviewWindowWords);
    contextPreviewStartIndex_ = startIndex;
    contextPreviewWindowValid_ = true;
    contextPreviewWords_.clear();
    contextPreviewWords_.reserve(endIndex - startIndex);
    for (size_t index = startIndex; index < endIndex; ++index) {
      DisplayManager::ContextWord word;
      word.text = reader_.wordAt(index);
      word.paragraphStart = isParagraphStart(index);
      word.current = index == currentIndex;
      contextPreviewWords_.push_back(word);
    }
    contextPreviewCurrentLocalIndex_ =
        currentIndex >= startIndex ? currentIndex - startIndex : static_cast<size_t>(-1);
    return;
  }

  const size_t nextLocalIndex = currentIndex - startIndex;
  if (contextPreviewCurrentLocalIndex_ < contextPreviewWords_.size()) {
    contextPreviewWords_[contextPreviewCurrentLocalIndex_].current = false;
  }
  if (nextLocalIndex < contextPreviewWords_.size()) {
    contextPreviewWords_[nextLocalIndex].current = true;
    contextPreviewCurrentLocalIndex_ = nextLocalIndex;
  } else {
    contextPreviewCurrentLocalIndex_ = static_cast<size_t>(-1);
  }
}

void App::invalidateContextPreviewWindow() {
  contextPreviewWindowValid_ = false;
  contextPreviewWords_.clear();
  contextPreviewCurrentLocalIndex_ = static_cast<size_t>(-1);
}

void App::renderContextPreview() {
  applyReaderUiOrientation();
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0) {
    renderReaderWord();
    return;
  }

  const size_t currentIndex = std::min(reader_.currentIndex(), wordCount - 1);
  updateContextPreviewWindow(currentIndex);

  contextViewVisible_ = true;
  const DisplayManager::ReaderChrome chrome = readerChrome();
  display_.renderScrollView(contextPreviewWords_, currentReaderContentToken(),
                            contextPreviewStartIndex_, currentIndex, 0,
                            currentChapterLabel(), readingProgressPercent(), "",
                            readerFooterStatusLabel(), chrome);
}

void App::renderScrollReader(uint32_t nowMs, const String &overlayText) {
  applyReaderUiOrientation();
  contextViewVisible_ = false;
  const size_t wordCount = reader_.wordCount();
  if (wordCount == 0) {
    renderReaderWord();
    return;
  }

  const size_t currentIndex = std::min(reader_.currentIndex(), wordCount - 1);
  updateContextPreviewWindow(currentIndex);

  uint16_t scrollProgressPermille = 0;
  if (state_ == AppState::Playing && currentIndex + 1 < wordCount) {
    const uint32_t durationMs = reader_.currentWordDurationMs();
    if (durationMs > 0) {
      const uint32_t elapsedMs = reader_.elapsedInCurrentWordMs(nowMs);
      scrollProgressPermille = static_cast<uint16_t>(
          std::min<uint32_t>(1000UL, (elapsedMs * 1000UL) / durationMs));
    }
  }

  const DisplayManager::ReaderChrome chrome = readerChrome();
  display_.renderScrollView(contextPreviewWords_, currentReaderContentToken(),
                            contextPreviewStartIndex_, currentIndex, scrollProgressPermille,
                            currentChapterLabel(), readingProgressPercent(), overlayText,
                            readerFooterStatusLabel(), chrome);
}

void App::renderWpmFeedback(uint32_t nowMs) {
  if (!ensureCurrentBookWordAvailable(nowMs)) {
    return;
  }

  applyReaderUiOrientation();
  wpmFeedbackVisible_ = true;
  wpmFeedbackUntilMs_ = nowMs + kWpmFeedbackMs;
  if (scrollModeEnabled()) {
    renderScrollReader(nowMs, String(reader_.wpm()) + " WPM");
    return;
  }

  contextViewVisible_ = false;
  const String beforeText = phantomWordsEnabled_ ? phantomBeforeText() : "";
  const String afterText = phantomWordsEnabled_ ? phantomAfterText() : "";
  const DisplayManager::ReaderChrome chrome = readerChrome();
  const String footerMetricLabel = readerFooterStatusLabel();
  display_.renderPhantomRsvpWordWithWpm(beforeText, reader_.currentWord(), afterText,
                                        readerFontSizeIndex_, reader_.wpm(),
                                        currentChapterLabel(), readingProgressPercent(),
                                        readerFooterVisible(), footerMetricLabel, chrome);
}

void App::renderStorageStatus(const char *title, const char *line1, const char *line2,
                              int progressPercent) {
  applyReaderUiOrientation();
  display_.renderProgress(title == nullptr ? "SD" : title, line1 == nullptr ? "" : line1,
                          line2 == nullptr ? "" : line2, progressPercent);
}

void App::handleStorageStatus(void *context, const char *title, const char *line1,
                              const char *line2, int progressPercent) {
  if (context == nullptr) {
    return;
  }

  static_cast<App *>(context)->renderStorageStatus(title, line1, line2, progressPercent);
  delay(0);
}
