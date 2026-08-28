#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <vector>

#include "app/AppState.h"
#include "app/HelpTexts.h"
#include "app/Localization.h"
#include "app/Translations.h"
#include "audio/AudioManager.h"
#include "audio/AudioRecorder.h"
#include "display/DisplayManager.h"
#include "input/ButtonHandler.h"
#include "input/TouchHandler.h"
#include "plugins/PluginLibrary.h"
#include "plugins/PluginLoader.h"
#include "reader/ReadingLoop.h"
#include "storage/PresetManager.h"
#include "storage/StorageManager.h"
#include "ble/BleApi.h"
#include "sync/CompanionSyncManager.h"
#include "ui/TouchGesture.h"
#include "ui/UiGrid.h"
#include "update/OtaUpdater.h"
#include "usb/UsbMassStorageManager.h"

class App {
  friend class BleApi;

 public:
  enum class ReaderMode : uint8_t {
    Rsvp = 0,
    Scroll = 1,
  };

  enum class HandednessMode : uint8_t {
    Right = 0,
    Left = 1,
  };

  enum class NavMode : uint8_t {
    Swipe = 0,
    DPad = 1,
    Buttons = 2,
  };

  App();

  void begin();
  void update(uint32_t nowMs);

 private:
  static constexpr size_t kOtaVersionLabelMax = 32;
  static constexpr size_t kOtaSummaryLabelMax = 40;
  static constexpr size_t kOtaDetailLabelMax = 96;

  struct OtaCheckResult {
    OtaUpdater::ResultCode code = OtaUpdater::ResultCode::MetadataFailed;
    char currentVersion[kOtaVersionLabelMax] = {};
    char latestVersion[kOtaVersionLabelMax] = {};
    char summary[kOtaSummaryLabelMax] = {};
    char detail[kOtaDetailLabelMax] = {};
  };

  struct OtaCheckTaskParams {
    OtaUpdater::Config config;
    QueueHandle_t resultQueue = nullptr;
  };

  struct PausedTouchSession {
    bool active = false;
    uint16_t startX = 0;
    uint16_t startY = 0;
    uint16_t lastX = 0;
    uint16_t lastY = 0;
    uint32_t startMs = 0;
    uint32_t lastMs = 0;
    size_t startWordIndex = 0;
    int gestureStepsApplied = 0;
    int32_t browseOffsetPermille = 0;
  };

  enum class TouchIntent {
    None,
    PlayHold,
    Scrub,
    BrowseScroll,
    Wpm,
  };

  enum class MenuScreen {
    Main,
    SettingsHome,
    SettingsDisplay,
    SettingsPacing,
    SettingsConnectivity,
    SettingsAbout,
    ScreensaverSettings,
    WifiSettings,
    WifiNetworks,
    TextEntry,
    TypographyTuning,
    BookPicker,
    BookDetails,
    BookDeleteConfirm,
    ChapterPicker,
    SavePointsList,
    SavePointDeleteConfirm,
    SavePointNameEntry,
    PluginsList,
    PluginLibraryScreen,
    PluginDetail,
    RestartConfirm,
    SdCardRepairConfirm,
    UpdateConfirm,
    WelcomeInstallApp,
    WelcomeLanguage,
    WelcomeTheme,
    WelcomeHighlightColor,
    WelcomePacing,
    WelcomeConnect,
    TutorialStep1,
    TutorialStep2,
    TutorialStep3,
    TutorialStep4,
    TutorialStep5,
    Presets,
    PresetsDeleteConfirm,
    PacingDelayEditor,
    WpmEditor,
  };

  // Which of the three pacing delays the PacingDelayEditor screen is
  // currently showing — one shared screen/handler for all three instead of
  // three near-identical copies.
  enum class PacingDelayTarget : uint8_t {
    LongWords,
    Complexity,
    Punctuation,
  };

  enum class FooterMetricMode : uint8_t {
    Percentage = 0,
    ChapterTime = 1,
    BookTime = 2,
  };

  enum class BatteryLabelMode : uint8_t {
    Percent = 0,
    TimeRemaining = 1,
    Voltage = 2,
  };

  enum class ScreensaverMode : uint8_t {
    Life = 0,
    Maze = 2,
    Voronoi = 3,
    Stars = 4,
    Matrix = 5,
    ScreenOff = 6,
  };

  enum class PauseMode : uint8_t {
    SentenceEnd = 0,
    Instant = 1,
  };

  enum class TextEntryPurpose : uint8_t {
    None,
    WifiPassword,
    OtaOwner,
    SavePointName,
    PresetName,
  };

  enum class KeyboardMode : uint8_t {
    Lower,
    Upper,
    Symbols,
  };

  enum class TextEntryAction : uint8_t {
    Insert,
    SetLower,
    SetUpper,
    SetSymbols,
    Space,
    Backspace,
    Clear,
    ToggleMask,
    Save,
    Cancel,
  };

  struct WifiNetworkInfo {
    String ssid;
    int32_t rssi = 0;
    uint8_t authMode = 0;
  };

  struct TextEntryButton {
    DisplayManager::Button view;
    TextEntryAction action = TextEntryAction::Insert;
    String payload;
  };

  struct TextEntrySession {
    bool active = false;
    TextEntryPurpose purpose = TextEntryPurpose::None;
    KeyboardMode mode = KeyboardMode::Lower;
    MenuScreen returnScreen = MenuScreen::Main;
    String title;
    String prompt;
    String helperText;
    String value;
    String contextValue;
    size_t maxLength = 63;
    bool masked = false;
    bool revealValue = false;
  };

  struct SavePoint {
    String name;
    String bookPath;
    String bookTitle;
    size_t wordIndex = 0;
    size_t chapterIndex = 0;
    uint8_t progressPercent = 0;
  };

  static constexpr size_t kMaxSavePoints = 20;

  void setState(AppState nextState, uint32_t nowMs);
  void updateState(uint32_t nowMs);
  void updateReader(uint32_t nowMs);
  void updateWpmFeedback(uint32_t nowMs);
  void maybeSaveReadingPosition(uint32_t nowMs);
  void handleBootButton(uint32_t nowMs);
  void handlePowerButton(uint32_t nowMs);
  bool handleStandbyCombo(uint32_t nowMs);
  void toggleMenuFromPowerButton(uint32_t nowMs);
  void restartFromPowerButtonDoubleTap();
  void openMainMenu(uint32_t nowMs);
  void cycleBrightness();
  void cycleThemeMode(uint32_t nowMs);
  void cycleUiLanguage(uint32_t nowMs);
  void cycleReaderMode(uint32_t nowMs);
  void cycleHandednessMode(uint32_t nowMs);
  void togglePhantomWords(uint32_t nowMs);
  void cycleReaderFontSize(uint32_t nowMs);
  void applyDisplayPreferences(uint32_t nowMs, bool rerender = true);
  void applyHandednessSettings(uint32_t nowMs, bool rerender = true);
  void applyTypographySettings(uint32_t nowMs, bool rerender = true);
  uint8_t currentBrightnessPercent() const;
  bool updateBatteryStatus(uint32_t nowMs, bool force = false);
  void handleBatteryProtection(uint32_t nowMs);
  void showLowBatteryWarning(uint32_t nowMs);
  void updateBatteryWarningOverlay(uint32_t nowMs);
  void handleTouch(uint32_t nowMs);
  void applyPausedTouchGesture(const TouchEvent &event, uint32_t nowMs);
  void handleReaderTap(uint16_t x, uint16_t y, uint32_t nowMs);
  bool handleFooterMetricTap(uint16_t x, uint16_t y, uint32_t nowMs);
  bool handleBatteryBadgeTap(uint16_t x, uint16_t y, uint32_t nowMs);
  bool handlePreviousSentenceTap(uint16_t x, uint16_t y, uint32_t nowMs);
  void requestReaderPauseAtSentenceEnd(uint32_t nowMs);
  void finalizeReaderPause(uint32_t nowMs);
  bool shouldFinalizeReaderPause(uint32_t nowMs) const;
  void resetReaderTapTracking();
  void flushStaleTouch();
  bool isFooterMetricTap(uint16_t x, uint16_t y) const;
  bool isBatteryBadgeTap(uint16_t x, uint16_t y) const;
  bool isPreviousSentenceTap(uint16_t x, uint16_t y) const;
  bool isSavePointButtonTap(uint16_t x, uint16_t y) const;
  bool isActivelyReading() const;
  bool readerFooterVisible() const;
  DisplayManager::ReaderChrome readerChrome() const;
  String readerFooterStatusLabel() const;
  String onOffLabel(bool enabled) const;
  int scrubStepsForDrag(int deltaX) const;
  void applyScrubTarget(int targetSteps, uint32_t nowMs);
  int browseScrollRatePermille(uint16_t y) const;
  void applyBrowseHoldScroll(uint16_t y, uint32_t elapsedMs, uint32_t nowMs);
  void renderContextBrowsePreview(size_t currentIndex, uint16_t scrollProgressPermille);
  void applyMenuTouchGesture(const TouchEvent &event, uint32_t nowMs);
  void moveMenuSelection(int direction);
  void selectMenuItem(uint32_t nowMs);
  void openSettings();
  void selectSettingsItem(uint32_t nowMs);
  void openWifiSettings();
  void selectWifiSettingsItem(uint32_t nowMs);
  void openTypographyTuning();
  void selectTypographyTuningItem(uint32_t nowMs);
  void cycleTypographyPreviewSample(int direction);
  void rebuildSettingsMenuItems();
  void applyPacingSettings();
  void maybeAutoCheckForUpdates(uint32_t nowMs);
  bool startBackgroundOtaCheck(const OtaUpdater::Config &config);
  static void otaCheckTask(void *params);
  void pollOtaCheckResult(uint32_t nowMs);
  void maybeOpenUpdateConfirm(uint32_t nowMs);
  bool updateConfirmCanOpen() const;
  bool blockNetworkActionForOtaCheck(const String &title, uint32_t nowMs);
  void runFirmwareUpdate(const OtaUpdater::Config &config, bool automatic, uint32_t nowMs);
  void runRssFeedCheck(uint32_t nowMs);
  OtaUpdater::Config preferredOtaConfig();
  void scanWifiNetworks();
  void renderWifiNetworks();
  void selectWifiNetworkItem(uint32_t nowMs);
  void openTextEntry(TextEntryPurpose purpose, const String &title, const String &prompt,
                     const String &helperText, const String &initialValue,
                     const String &contextValue, bool masked, size_t maxLength,
                     MenuScreen returnScreen);
  void rebuildTextEntryButtons();
  void renderTextEntry();
  bool handleTextEntryTap(uint16_t x, uint16_t y, uint32_t nowMs);
  void firePendingTextEntryFlash(uint32_t nowMs);
  void activateTextEntryButton(size_t buttonIndex, uint32_t nowMs);
  void commitTextEntry(uint32_t nowMs);
  String configuredWifiSsid();
  String findSavedWifiPassword(const String &ssid);
  void rememberWifiNetwork(const String &ssid, const String &pass);
  void forgetSavedWifiNetwork(const String &ssid);
  bool otaAutoCheckEnabled();
  String otaOwnerLabel();
  /// Tryb developera — domyślnie wyłączony. Włączany tylko z poziomu
  /// aplikacji-towarzysza (ukryty 10-tap w PWA → settings sync API).
  /// Kiedy wyłączony, advanced ustawienia (OTA owner, auto-OTA itp.)
  /// nie pojawiają się w menu urządzenia — UX dla klienta jest czystsze.
  bool devModeEnabled();
  void setDevModeEnabled(bool enabled);

  /// Etykieta wersji firmware (z `RSVP_FIRMWARE_VERSION` build flag).
  /// Używane przez BleApi do odpowiedzi na `get-version`.
  String firmwareVersionLabel() const;

  /// Zwraca true dla każdego ekranu który renderuje się jako list-menu
  /// settings (display_.renderMenu z settingsMenuItems_). Touch handlery
  /// i nawigacja musi traktować je wszystkie tak samo — bez tego helpera
  /// trzeba by powielać warunek w 6 miejscach.
  bool isSettingsListScreen() const;

  /// Lekka lokalizacja PL/EN bez rozbudowy UiText. Wszystkie nowe etykiety
  /// w settings/welcome przechodzą przez ten helper — w polskim zwraca `pl`,
  /// w pozostałych językach `en` (które dla rsvpnano i tak było defaultem).
  /// Bez diakrytyków, bo embedded fonty ich nie obsługują (patrz istniejące
  /// tłumaczenia w Localization.h: "Wroc", "Jasnosc", itd.).
  const char *polish(const char *pl, const char *en) const;

  /// Pełna lokalizacja 6-językowa przez TrKey (Translations.h).
  const char *tr(TrKey key) const;
  const char *tr2(TrKey2 key) const;

  /// Nowe ekrany ustawień zorganizowane wokół codziennego użycia (a nie
  /// odziedziczonej hierarchii rsvpnano). Otwieranie + handlery wyboru.
  void openSettingsConnectivity();
  void selectSettingsConnectivityItem(uint32_t nowMs);
  void openSettingsAbout();
  void selectSettingsAboutItem(uint32_t nowMs);

  /// First-run welcome wizard — pyta o język i motyw zanim klient zobaczy
  /// główne menu. Pokazywany tylko jeśli `kPrefSetupDone == false`.
  /// Krok 0 kreatora: kod QR do PWA. Cały ekran jest przyciskiem — dowolny
  /// tap idzie dalej, do wyboru języka. Ekran włącza się w updateState()
  /// przy pierwszym boocie (`kPrefSetupDone == false`).
  void renderWelcomeInstallApp();
  void openWelcomeLanguage();
  void selectWelcomeLanguageItem(uint32_t nowMs);
  void openWelcomeTheme();
  void selectWelcomeThemeItem(uint32_t nowMs);
  void openWelcomeHighlightColor();
  void selectWelcomeHighlightColorItem(uint32_t nowMs);
  void openWelcomePacing();
  void selectWelcomePacingItem(uint32_t nowMs);
  void openWelcomeConnect(uint32_t nowMs);
  void selectWelcomeConnectItem(uint32_t nowMs);
  void finishWelcomeWizard(uint32_t nowMs);
  void openTutorialStep1();
  void openTutorialStep2();
  void openTutorialStep3();
  void openTutorialStep4();
  void openTutorialStep5();
  void finishTutorial(uint32_t nowMs);
  void renderTutorialStep();
  void handleTutorialTap(uint32_t nowMs);
  void previousTutorialStep(uint32_t nowMs);
  String pacingDelayLabel(uint16_t delayMs) const;
  // Full-screen drag-slider editor for the three pacing delays (long words /
  // complexity / punctuation). Replaces the old "tap to cycle by 50ms with
  // no way back down" behaviour on those three settings — see the tap
  // handler in selectSettingsItem()'s SettingsPacing switch.
  void openPacingDelayEditor(PacingDelayTarget target, uint32_t nowMs);
  void renderPacingDelayEditor();
  void handlePacingDelayEditorTouch(const TouchEvent &event, uint32_t nowMs);
  void applyPacingDelayEditorTouchX(uint16_t x);
  uint16_t *pacingDelayEditorValuePtr();
  const char *pacingDelayEditorPrefKey() const;
  String pacingDelayEditorLabel() const;

  void openWpmEditor(uint32_t nowMs);
  void renderWpmEditor();
  void handleWpmEditorTouch(const TouchEvent &event, uint32_t nowMs);
  void applyWpmEditorTouchX(uint16_t x);
  String wpmEditorLabel() const;
  String firmwareUpdateMenuLabel() const;
  String themeModeLabel() const;
  String phantomWordsLabel() const;
  String focusHighlightLabel() const;
  String focusColorLabel() const;
  void cycleFocusColor(uint32_t nowMs);
  String uiLanguageLabel() const;
  String readerModeLabel() const;
  String scrollFontSizeLabel() const;
  String scrollLineSpacingLabel() const;
  String scrollMarginLabel() const;
  String pauseModeLabel() const;
  String handednessLabel() const;
  String navModeLabel() const;
  String readerFontSizeLabel() const;
  String readerTypefaceLabel() const;
  String typographyTuningLabel() const;
  String typographyTuningValueLabel() const;
  String uiText(UiText key) const;
  void openBookPicker(bool articlesOnly = false);
  void selectBookPickerItem(uint32_t nowMs);
  void openBookDetails(size_t bookIndex, uint32_t nowMs);
  void selectBookDetailsItem(uint32_t nowMs);
  void openBookDeleteConfirm(uint32_t nowMs);
  void selectBookDeleteConfirmItem(uint32_t nowMs);
  void executeDeleteBook(uint32_t nowMs);
  void openChapterPicker();
  void openChapterPickerForBook(size_t bookIndex);
  void selectChapterPickerItem(uint32_t nowMs);
  void openSavePointsList();
  void selectSavePointItem(uint32_t nowMs);
  void openSavePointDeleteConfirm(size_t index, uint32_t nowMs);
  void selectSavePointDeleteConfirmItem(uint32_t nowMs);
  void executeDeleteSavePoint(uint32_t nowMs);
  void createSavePoint(uint32_t nowMs);
  void deleteSavePoint(size_t index);
  void loadSavePoints();
  void persistSavePoints();
  void openPluginsList();
  void selectPluginsItem(uint32_t nowMs);
  void autoSyncPlugins();

  void openPluginLibraryScreen(uint32_t nowMs);
  void selectPluginLibraryItem(uint32_t nowMs);
  void renderPluginLibraryScreen();
  void openPluginDetail(size_t registryIndex);
  void selectPluginDetailItem(uint32_t nowMs);
  void renderPluginDetail();
  void openPresets();
  void selectPresetsItem(uint32_t nowMs);
  void confirmDeletePreset(size_t index, uint32_t nowMs);
  void executeDeletePreset(uint32_t nowMs);
  void executeSavePreset(uint32_t nowMs);
  void executeRestorePreset(size_t index, uint32_t nowMs);
  void openRestartConfirm();
  void selectRestartConfirmItem(uint32_t nowMs);
  void openSdCardRepairConfirm();
  void selectSdCardRepairConfirmItem(uint32_t nowMs);
  void runSdCardRepair(uint32_t nowMs);
  void runSdCardCheck(uint32_t nowMs);
  void openUpdateConfirm();
  void selectUpdateConfirmItem(uint32_t nowMs);
  void enterCompanionSync(uint32_t nowMs);
  void updateCompanionSync(uint32_t nowMs);
  void exitCompanionSync(uint32_t nowMs);
  void enterUsbTransfer(uint32_t nowMs);
  void updateUsbTransfer(uint32_t nowMs);
  void exitUsbTransfer(uint32_t nowMs);
  void enterStandby(uint32_t nowMs);
  void exitStandby(uint32_t nowMs);
  void seedStandbyScreensaver(uint32_t nowMs);
  void stepStandbyScreensaver(uint32_t nowMs);
  void seedStandbyLife(uint32_t nowMs);
  void stepStandbyLife();
  void seedStandbyMaze(uint32_t nowMs);
  void stepStandbyMaze();
  void seedStandbyVoronoi(uint32_t nowMs);
  void stepStandbyVoronoi();
  void renderStandbyVoronoi();
  void seedStandbyScreenOff(uint32_t nowMs);
  void updateStandbyScreensaver(uint32_t nowMs, bool force = false);
  void seedStandbyStars(uint32_t nowMs);
  void stepStandbyStars();
  void seedStandbyMatrix(uint32_t nowMs);
  void stepStandbyMatrix();
  void openScreensaverSettings();
  void selectScreensaverSettingsItem(uint32_t nowMs);
  void renderScreensaverSettings();
  void showHelpForCurrentItem();
  void dismissHelpPopup(uint32_t nowMs);
  String screensaverTimeoutLabel() const;
  String screensaverAutoOffLabel() const;
  String screensaverSleepGuardLabel() const;
  void enterPowerOff(uint32_t nowMs);
  void enterSleep(uint32_t nowMs);
  void wakeFromSleep();
  bool restoreSavedBook(uint32_t nowMs);
  bool prepareBootBookLoad();
  void loadPendingBootBook(uint32_t nowMs);
  void saveReadingPosition(bool force = false);
  bool loadBookAtIndex(size_t index, uint32_t nowMs, bool allowLegacyPositionFallback = false,
                       bool allowIndexBuild = true, bool allowEpubConversion = true,
                       bool rebuildTimeEstimate = true);
  String bookPositionKey(const String &bookPath) const;
  String bookWordCountKey(const String &bookPath) const;
  String bookRecentKey(const String &bookPath) const;
  uint32_t nextRecentSequence();
  uint32_t bookRecentSequence(const String &bookPath);
  void markBookRecent(const String &bookPath);
  uint32_t savedWordIndexForBook(const String &bookPath, bool allowLegacyFallback = false);
  bool bookProgressPercent(size_t bookIndex, uint8_t &percent);
  int findBookIndexByPath(const String &path) const;
  /// Immediate-mode button grid shared by every menu/list screen: builds
  /// the same ui::Rect layout used for drawing AND touch hit-testing, so
  /// the two can never drift apart (see firmware/src/ui/UiGrid.h).
  /// `headerRows` items at the front of `items` are rendered as a
  /// non-tappable title above the grid (used by confirm dialogs whose
  /// index 0 is an informational header, e.g. "Na pewno?").
  void renderItemGrid(const String &title, const std::vector<String> &items, size_t selectedIndex,
                      size_t headerRows = 0, bool showBatteryBadge = true);
  void renderItemGridLibrary(const std::vector<DisplayManager::LibraryItem> &items,
                             size_t selectedIndex);
  /// Dispatches to whichever render function matches navMode_ — the button
  /// grid (Buttons), the cursor list (DPad, via DisplayManager::
  /// renderMenuWithDPad), or the bare scrollable list (Swipe, via
  /// DisplayManager::renderMenuScroll). Every menu screen that is a plain
  /// list of choices should render through this instead of calling
  /// renderItemGrid() directly, so it behaves consistently in all three nav
  /// modes. Small confirm dialogs (2-4 items, e.g. RestartConfirm) are left
  /// calling renderItemGrid() directly — a full-screen grid dialog reads
  /// fine regardless of nav mode, and DPad/Swipe's list chrome would be
  /// wasted on them.
  void renderMenuAnyMode(const String &title, const std::vector<String> &items,
                         size_t selectedIndex, size_t headerRows = 0);
  /// Same dispatch as renderMenuAnyMode(), for library-style (title +
  /// subtitle) lists. DPad/Swipe show titles only — no room for a subtitle
  /// in a single compact list row.
  void renderMenuAnyModeLibrary(const std::vector<DisplayManager::LibraryItem> &items,
                                size_t selectedIndex);
  /// Returns the pointer/count pair describing the currently active menu
  /// screen's selection state — the single place that knows how each
  /// MenuScreen maps to its backing vector, reused by moveMenuSelection()
  /// (D-Pad/legacy swipe) and by the grid tap hit-test.
  size_t *currentMenuSelectedIndexPtr(size_t &itemCountOut);
  /// Direct hit-test against the Rects built by the last renderItemGrid*()
  /// call. Returns true if a button was hit (and dispatches the screen's
  /// existing selectXItem() handler with the tapped index applied).
  ///
  /// Two-step tap confirm: the first tap on a button arms it (highlighted,
  /// no action yet); a second tap on the SAME button within
  /// kArmedConfirmWindowMs confirms and runs the action. Tapping a
  /// different button re-arms it instead of acting on the old one. "Wróć"/
  /// Back buttons are exempt — a confirm step on a pure navigation action
  /// is friction without a safety payoff, so Back always fires immediately.
  bool handleGridTap(uint16_t x, uint16_t y, uint32_t nowMs);
  /// True if `canonicalIndex` on the current menu screen is the armed
  /// (first-tapped, awaiting confirm) grid button and the confirm window
  /// hasn't expired. Shared by handleGridTap() (to decide confirm vs. arm)
  /// and by renderItemGrid()/renderItemGridLibrary() (to draw the highlight).
  bool isGridItemArmed(size_t canonicalIndex, uint32_t nowMs) const;
  /// True if `canonicalIndex` on the current menu screen is mid press-flash
  /// (tapped, action not fired yet) — see pendingFlashItemIndex_.
  bool isGridItemFlashing(size_t canonicalIndex, uint32_t nowMs) const;
  /// If a press-flash is pending and its window elapsed, runs the deferred
  /// selectMenuItem() now. Called every tick from handleTouch().
  void firePendingGridFlash(uint32_t nowMs);
  /// Horizontal swipe changes page for grid screens with more items than
  /// fit on one page. Returns true if it consumed the gesture.
  bool handleGridPageSwipe(int deltaX, int deltaY, uint32_t nowMs);
  /// Shrinks whichever button in currentGridButtons_ is the Back control
  /// (Button::icon == IconId::Back) down to a small fixed rect pinned in
  /// the top-left corner, icon-only — instead of taking a full grid cell
  /// like every other button. Same Rect drives drawing and hit-testing, so
  /// there is exactly one place that can get the two out of sync.
  void applyBackButtonCornerLayout();
  /// SettingsDisplay-only: gives specific rows (booleans, 3-way cycles) a
  /// widget that shows their current value at a glance — a toggle track or
  /// a row of state dots — instead of every setting looking like the same
  /// plain rectangle regardless of what it controls.
  void annotateSettingsDisplayButton(DisplayManager::Button &button, size_t canonicalIndex) const;
  /// SavePointsList-only: gives the "+ Add save point" row and each named
  /// save point the floppy-disk icon (ui::IconId::SavePoint) — drawn via
  /// drawButtons()'s icon+label combo mode, since (unlike Back) these
  /// labels are real, variable text that can't be blanked to icon-only.
  void annotateSavePointsButton(DisplayManager::Button &button, size_t canonicalIndex) const;
  /// Main menu only: gives each top-level tile (Read/Library/Save
  /// points/Settings/Plugins/Power off) an icon so the menu reads as an app
  /// grid, matched by label text like the Back icon above.
  void annotateMainMenuButton(DisplayManager::Button &button) const;
  /// Shows toastText for kGridToastVisibleMs on top of the current button
  /// grid — the uncut version of whatever a Toggle/Cycle button's tap just
  /// changed. Same shape as showLowBatteryWarning()/
  /// updateBatteryWarningOverlay(): set-and-timer here, timer checked and
  /// cleared every tick by updateGridToastOverlay().
  void showGridToast(const String &text, uint32_t nowMs);
  /// Clears the grid toast once its visible window elapses and redraws the
  /// menu so the toast bar disappears. Called every tick from update().
  void updateGridToastOverlay(uint32_t nowMs);
  /// Toast text to pass into renderItemGrid()'s display_.renderButtonGrid()
  /// call — empty once expired or when nothing is pending.
  String activeGridToastText(uint32_t nowMs) const;
  /// Hit-tests a tap against the row layout shared by renderMenuWithDPad()
  /// and renderMenuScroll() (same centered-window math, see
  /// kCompactMenuRowHeight in DisplayManager.cpp). Returns true and fills
  /// outIndex if the tap landed on a visible row.
  bool hitTestMenuListRow(uint16_t tapX, uint16_t tapY, size_t itemCount, size_t selectedIndex,
                          size_t &outIndex) const;
  /// Swipe/scroll nav mode's touch handling: tap picks the row under the
  /// finger (via hitTestMenuListRow), a vertical drag scrolls the list by
  /// shifting the selected index proportionally to drag distance. Mirrors
  /// the D-Pad panel's tap handling in applyMenuTouchGesture() but without
  /// any button chrome. Returns true (gesture always consumed in Swipe
  /// mode, matching the "ignore taps that hit nothing" DPad behavior).
  bool handleSwipeListGesture(const TouchEvent &event, int deltaX, int deltaY, uint32_t nowMs);

  void renderMenu();
  void renderMainMenu();
  /// Liczba pozycji, które renderMainMenu() naprawdę rysuje (Pluginy tylko w
  /// trybie zaawansowanym, +1 gdy widać przycisk aktualizacji).
  size_t mainMenuItemCount();
  void renderSettings();
  void showScrollSettingsPreview();
  void renderTypographyTuning();
  void renderBookPicker();
  void renderBookDetails();
  void renderChapterPicker();
  void renderSavePointsList();
  void renderPluginsList();
  void renderRestartConfirm();
  void renderSdCardRepairConfirm();
  void renderUpdateConfirm();
  void renderActiveReader(uint32_t nowMs);
  bool updateChapterTransition(uint32_t nowMs);
  bool maybeStartChapterTransition(size_t previousWordIndex, size_t currentWordIndex,
                                   uint32_t nowMs);
  void renderChapterTransition();
  void renderScrollReader(uint32_t nowMs, const String &overlayText = "");
  DisplayManager::LibraryItem libraryItemForBook(size_t bookIndex);
  String chapterMenuLabel(size_t chapterIndex) const;
  size_t currentChapterIndex() const;
  String currentChapterLabel() const;
  String currentFooterMetricLabel() const;
  String currentBatteryLabel() const;
  String footerMetricModeLabel() const;
  String batteryLabelModeLabel() const;
  String screensaverModeLabel() const;
  String batteryTimeRemainingLabel() const;
  String batteryVoltageLabel() const;
  String formatBatteryTimeRemaining(uint32_t minutes) const;
  uint32_t estimatedReadingTimeRemainingMs(size_t startIndex, size_t endIndex) const;
  uint32_t estimatedPacingBonusMs(size_t startIndex, size_t endIndex) const;
  void rebuildTimeEstimateCache();
  void invalidateTimeEstimateCache();
  void flushPendingTimeEstimateRebuild();
  void cancelTimeEstimateBuild();
  void updateTimeEstimateBuild(uint32_t nowMs);
  bool timeEstimateBuildMatchesCurrentBook() const;
  String formatReadingTimeRemaining(uint32_t remainingMs) const;
  String timeEstimateModeLabel() const;
  uint8_t readingProgressPercent() const;
  // Default name suggested when naming a new save point: "<percent>%
  // <title/chapter>", one decimal place so two bookmarks made moments
  // apart (same rounded whole-percent progress) still read as different
  // entries instead of colliding on an identical suggested name — see the
  // three call sites in App.cpp that used to duplicate this.
  String savePointDefaultName() const;
  bool ensureCurrentBookWordAvailable(uint32_t nowMs);
  void handleCurrentBookReadFailure(uint32_t nowMs, const char *detail);
  void renderReaderWord();
  void renderContextPreview();
  void renderWpmFeedback(uint32_t nowMs);
  size_t phantomBeforeCharTarget() const;
  size_t phantomAfterCharTarget() const;
  String collectPhantomBeforeText(size_t currentIndex, size_t charTarget) const;
  String collectPhantomAfterText(size_t currentIndex, size_t charTarget) const;
  String phantomBeforeText() const;
  String phantomAfterText() const;
  bool isParagraphStart(size_t wordIndex) const;
  size_t paragraphStartAtOrBefore(size_t wordIndex) const;
  size_t contextPreviewAnchorIndex(size_t currentIndex) const;
  void updateContextPreviewWindow(size_t currentIndex);
  void invalidateContextPreviewWindow();
  void renderStorageStatus(const char *title, const char *line1, const char *line2,
                           int progressPercent);
  static void handleStorageStatus(void *context, const char *title, const char *line1,
                                  const char *line2, int progressPercent);
  const char *stateName(AppState state) const;
  const char *touchPhaseName(TouchPhase phase) const;
  bool scrollModeEnabled() const;
  void applyUiOrientation(BoardConfig::UiOrientation orientation);
  void applyReaderUiOrientation();
  void reloadRuntimePreferences(uint32_t nowMs, bool rerender);
  BoardConfig::UiOrientation readerUiOrientation() const;
  bool uiRotated180() const;
  uint8_t effectiveAnchorPercent() const;
  DisplayManager::TypographyConfig effectiveTypographyConfig() const;
  uint32_t currentReaderContentToken() const;

  AppState state_ = AppState::Booting;
  AppState standbyReturnState_ = AppState::Paused;
  DisplayManager display_;
  AudioManager audio_;
  AudioRecorder recorder_;
  ReadingLoop reader_;
  ButtonHandler button_;
  ButtonHandler powerButton_;
  TouchHandler touch_;
  StorageManager storage_;
  IndexedBookStore activeBookStore_;
  OtaUpdater otaUpdater_;
  CompanionSyncManager companionSync_;
  BleApi ble_;
  UsbMassStorageManager usbTransfer_;
  PluginLibrary pluginLibrary_;
  PluginLoader pluginLoader_;
  PresetManager presetManager_;
  Preferences preferences_;
  PausedTouchSession pausedTouch_;
  TouchIntent pausedTouchIntent_ = TouchIntent::None;

  uint32_t bootStartedMs_ = 0;
  uint32_t lastStateLogMs_ = 0;
  uint32_t wpmFeedbackUntilMs_ = 0;
  uint32_t lastProgressSaveMs_ = 0;
  uint32_t lastBatterySampleMs_ = 0;
  uint32_t batteryRuntimeAnchorMs_ = 0;
  uint32_t lastScrollAnimationRenderMs_ = 0;
  uint32_t lastCompanionSyncRenderMs_ = 0;
  uint32_t autoSyncStartedMs_ = 0;
  bool autoSyncActive_ = false;
  bool autoSyncClientConnected_ = false;
  bool pluginSyncDone_ = false;
  uint32_t lastReaderTapMs_ = 0;
  uint32_t standbyComboStartedMs_ = 0;
  uint32_t standbyEnteredMs_ = 0;
  uint32_t lastActivityMs_ = 0;
  uint32_t lastStandbyFrameMs_ = 0;
  uint32_t standbyLifeGeneration_ = 0;
  uint32_t standbyScreensaverRng_ = 1;
  uint32_t chapterTransitionUntilMs_ = 0;
  uint32_t lastLowBatteryWarningMs_ = 0;
  uint32_t batteryWarningRestoreAtMs_ = 0;
  size_t lastSavedWordIndex_ = static_cast<size_t>(-1);
  size_t contextPreviewStartIndex_ = 0;
  size_t contextPreviewCurrentLocalIndex_ = static_cast<size_t>(-1);
  size_t currentBookIndex_ = 0;
  size_t pendingBootBookIndex_ = 0;
  size_t menuSelectedIndex_ = 0;
  // Mapa pozycja-na-ekranie -> MenuItem, budowana przez renderMainMenu()
  // w tej samej kolejności co przyciski na ekranie. selectMenuItem() czyta
  // stąd zamiast zakładać stały porządek enuma — kolejność kafelków w
  // trybie zaawansowanym różni się od podstawowego (Wyłącz jest ostatnie,
  // za Pluginami, tylko gdy Pluginy w ogóle są widoczne).
  std::vector<size_t> mainMenuOrder_;
  size_t settingsSelectedIndex_ = 0;
  // Counter dla 10-tap na "Wersji" w SettingsAbout — odblokowuje dev mode
  // bezpośrednio z urządzenia (oprócz odblokowania z PWA).
  uint8_t aboutTapCount_ = 0;
  uint32_t aboutLastTapMs_ = 0;
  size_t wifiNetworkSelectedIndex_ = 0;
  size_t bookPickerSelectedIndex_ = 0;
  size_t chapterPickerSelectedIndex_ = 0;
  size_t chapterTransitionIndex_ = static_cast<size_t>(-1);
  size_t restartConfirmSelectedIndex_ = 0;
  size_t sdCardRepairConfirmSelectedIndex_ = 0;
  size_t updateConfirmSelectedIndex_ = 0;
  uint8_t brightnessLevelIndex_ = 4;
  uint8_t readerFontSizeIndex_ = 0;
  uint8_t scrollFontSize_ = 4;
  uint8_t scrollLineSpacing_ = 1;
  uint8_t scrollMargin_ = 1;
  uint16_t pacingLongWordDelayMs_ = 200;
  uint16_t pacingComplexWordDelayMs_ = 200;
  uint16_t pacingPunctuationDelayMs_ = 200;
  PacingDelayTarget pacingDelayEditorTarget_ = PacingDelayTarget::LongWords;
  // True while the touch that is currently down started on the editor's
  // corner Back icon — set on TouchPhase::Start, consumed on End, so a
  // press-and-drag off the icon doesn't accidentally exit the screen.
  bool pacingDelayEditorTouchOnBack_ = false;
  bool wpmEditorTouchOnBack_ = false;
  size_t typographyTuningSelectedIndex_ = 1;
  size_t typographyPreviewSampleIndex_ = 0;
  // Rects for the currently-rendered button grid, aligned with
  // currentGridItemIndices_ (same index i => same underlying item).
  // Rebuilt every renderItemGrid*() call; consumed by handleGridTap().
  std::vector<DisplayManager::Button> currentGridButtons_;
  std::vector<size_t> currentGridItemIndices_;
  // Pagination bookkeeping from the same renderItemGrid*() call that built
  // currentGridButtons_ above, cached so handleGridPageSwipe() can page
  // using the exact same tileCount/itemsPerPage/pageCount math as what was
  // actually drawn — recomputing it separately from the raw item count
  // (which includes the Back tile the grid excludes) used to drift by one
  // and mispage on any screen with a Back button and 2+ pages.
  size_t gridHeaderRows_ = 0;
  bool gridHasBack_ = false;
  size_t gridItemsPerPage_ = 1;
  size_t gridPageCount_ = 1;
  size_t gridPage_ = 0;
  // SavePointsList overrides the grid to one full-width row per savepoint
  // (name row + its Delete row) instead of the usual multi-column tile
  // grid — long "42.3% Book Title" names were unreadable packed 4-up, and
  // multiple bookmarks saved close together used to look identical at a
  // glance. Paging becomes vertical (one bookmark per "page") with the
  // page dots moved to the left edge instead of centered at the bottom —
  // see renderSavePointsList()/handleGridPageSwipe().
  bool gridPagesVertically_ = false;
  // Two-step tap confirm state for the grid (see handleGridTap()/
  // isGridItemArmed()). -1 means nothing is armed. armedGridScreen_ scopes
  // the armed index to the screen it was armed on, so navigating away and
  // later landing on the same numeric index elsewhere never confirms it.
  int armedGridItemIndex_ = -1;
  uint32_t armedGridArmedAtMs_ = 0;
  MenuScreen armedGridScreen_ = MenuScreen::Main;
  // Press-flash: every grid tap (Back included) highlights the button in
  // focusColor for kPressFlashMs before the action actually runs, so a tap
  // always gets a visible "yes, I felt that" before the screen changes —
  // see handleGridTap()/isGridItemFlashing() and the fire check in
  // handleTouch(). -1 means nothing is pending.
  int pendingFlashItemIndex_ = -1;
  uint32_t pendingFlashFireAtMs_ = 0;
  MenuScreen pendingFlashScreen_ = MenuScreen::Main;
  // Debounces contact-bounce on cheap capacitive touch: a finger landing can
  // register as a real release-then-recontact a few ms apart (two Start/End
  // cycles from one physical tap), which used to fire a Cycle button (e.g.
  // screensaver style) twice per tap. Tracks the last button actually fired
  // so a second tap on the exact same button, on the exact same screen,
  // within kGridTapDebounceMs is swallowed instead of firing again.
  int lastFiredGridItemIndex_ = -1;
  uint32_t lastFiredGridAtMs_ = 0;
  MenuScreen lastFiredGridScreen_ = MenuScreen::Main;
  // Same press-flash idea, but for the on-screen keyboard (see
  // handleTextEntryTap()/firePendingTextEntryFlash()) — every key tap
  // highlights briefly before the character/action actually lands, so
  // typing feels as responsive/acknowledged as every other button.
  int pendingTextEntryFlashIndex_ = -1;
  uint32_t pendingTextEntryFlashFireAtMs_ = 0;
  // true: the key's action already ran (typing keys — see
  // handleTextEntryTap()), this pending entry only clears the highlight.
  // false: the action hasn't run yet and firePendingTextEntryFlash() must
  // fire it (mode switches / clear / save / cancel).
  bool pendingTextEntryFlashIsPostActionOnly_ = false;
  // Grid toast (see showGridToast()/updateGridToastOverlay()): full text of
  // a Toggle/Cycle button's new value, shown briefly on top of the grid
  // because the button widget itself is too small to show it uncut. Empty
  // string means nothing is pending.
  String pendingToastText_;
  uint32_t toastVisibleUntilMs_ = 0;
  MenuScreen menuScreen_ = MenuScreen::Main;
  MenuScreen restartConfirmReturnScreen_ = MenuScreen::Main;
  QueueHandle_t otaCheckQueue_ = nullptr;
  std::vector<String> settingsMenuItems_;
  std::vector<DisplayManager::LibraryItem> wifiNetworkMenuItems_;
  std::vector<DisplayManager::LibraryItem> bookMenuItems_;
  std::vector<size_t> bookPickerBookIndices_;
  std::vector<String> chapterMenuItems_;
  std::vector<ChapterMarker> chapterMarkers_;
  std::vector<size_t> paragraphStarts_;
  std::vector<SavePoint> savePoints_;
  std::vector<String> savePointMenuItems_;
  size_t savePointSelectedIndex_ = 0;
  std::vector<String> savePointDeleteConfirmMenuItems_;
  size_t savePointDeleteConfirmSelectedIndex_ = 0;
  size_t savePointDeleteTargetIndex_ = 0;
  std::vector<String> presetFilenames_;
  size_t presetsDeleteTargetIndex_ = 0;
  size_t presetsSelectedIndex_ = 0;
  std::vector<String> bookDetailsMenuItems_;
  size_t bookDetailsSelectedIndex_ = 0;
  size_t bookDetailsBookIndex_ = 0;
  std::vector<String> bookDeleteConfirmMenuItems_;
  size_t bookDeleteConfirmSelectedIndex_ = 0;
  std::vector<String> pluginsMenuItems_;
  size_t pluginsSelectedIndex_ = 0;
  std::vector<String> pluginLibraryMenuItems_;
  size_t pluginLibrarySelectedIndex_ = 0;
  std::vector<String> pluginDetailMenuItems_;
  size_t pluginDetailSelectedIndex_ = 0;
  size_t pluginDetailIndex_ = 0;
  MenuScreen wifiReturnScreen_ = MenuScreen::SettingsHome;
  std::vector<uint32_t> wordBonusBlockPrefixSumMs_;
  String timeEstimateBuildBookPath_;
  size_t timeEstimateBuildWordCount_ = 0;
  size_t timeEstimateBuildBlockCount_ = 0;
  size_t timeEstimateBuildNextBlock_ = 0;
  uint32_t timeEstimateBuildRunningMs_ = 0;
  uint32_t timeEstimateBuildStartedMs_ = 0;
  uint32_t timeEstimateBuildLastLogMs_ = 0;
  bool timeEstimateCacheValid_ = false;
  bool timeEstimateBuildInProgress_ = false;
  bool accurateTimeEstimateEnabled_ = true;
  bool pacingCacheDirty_ = false;
  std::vector<DisplayManager::ContextWord> contextPreviewWords_;
  std::vector<WifiNetworkInfo> wifiNetworks_;
  std::vector<TextEntryButton> textEntryButtons_;
  std::vector<uint32_t> standbyLifeCells_;
  std::vector<uint32_t> standbyLifeNextCells_;
  std::vector<uint32_t> standbyScreensaverDimCells_;
  std::vector<uint8_t> standbyMazeVisited_;
  std::vector<uint16_t> standbyMazeStack_;
  std::vector<int16_t> standbyVoronoiX_;
  std::vector<int16_t> standbyVoronoiY_;
  std::vector<int16_t> standbyVoronoiDx_;
  std::vector<int16_t> standbyVoronoiDy_;
  std::vector<int16_t> standbyStarsX_;
  std::vector<int16_t> standbyStarsY_;
  std::vector<int8_t> standbyStarsSpeed_;
  std::vector<uint8_t> standbyStarsBright_;
  std::vector<uint8_t> standbyMatrixColumns_;
  std::vector<uint8_t> standbyMatrixHeads_;
  std::vector<uint8_t> standbyMatrixTrails_;
  String currentBookPath_;
  String currentBookTitle_;
  String pendingUpdateCurrentVersion_;
  String pendingUpdateNewVersion_;
  String batteryLabel_;
  float batteryFilteredVoltage_ = 0.0f;
  float batteryFilteredPercent_ = 0.0f;
  uint8_t batteryDisplayedPercent_ = 0;
  uint8_t batteryRuntimeAnchorPercent_ = 0;
  uint32_t batteryRuntimeMinutesRemaining_ = 0;
  TextEntrySession textEntrySession_;
  uint16_t lastReaderTapX_ = 0;
  uint16_t lastReaderTapY_ = 0;
  bool touchInitialized_ = false;
  bool touchPlayHeld_ = false;
  bool playLocked_ = false;
  bool pauseAtSentenceEndRequested_ = false;
  bool lastReaderTapValid_ = false;
  bool bootButtonReleasedSinceBoot_ = false;
  bool bootButtonLongPressHandled_ = false;
  bool powerButtonReleasedSinceBoot_ = false;
  bool powerButtonLongPressHandled_ = false;
  bool powerTapPending_ = false;
  uint32_t powerTapPendingMs_ = 0;
  bool powerOffStarted_ = false;
  bool standbyComboActive_ = false;
  bool standbyComboHandled_ = false;
  bool standbyButtonsReleased_ = false;
  bool standbyScreenOffActive_ = false;
  bool chapterTransitionVisible_ = false;
  bool batteryWarningOverlayVisible_ = false;
  bool otaCheckInProgress_ = false;
  bool otaUpdatePromptPending_ = false;
  uint8_t pwrTapCount_ = 0;
  uint32_t pwrFirstTapMs_ = 0;
  bool contextViewVisible_ = false;
  bool contextPreviewWindowValid_ = false;
  bool wpmFeedbackVisible_ = false;
  bool usingStorageBook_ = false;
  bool storageReady_ = false;
  bool pendingBootBookLoad_ = false;
  bool pendingBootBookLegacyFallback_ = false;
  bool batteryPresent_ = false;
  bool batterySampleInitialized_ = false;
  bool batteryRuntimeEstimateReady_ = false;
  uint8_t batteryCriticalSampleCount_ = 0;
  bool phantomWordsEnabled_ = true;
  bool readerBatteryVisibleWhilePlaying_ = true;
  bool readerChapterVisibleWhilePlaying_ = false;
  bool readerProgressVisibleWhilePlaying_ = false;
  bool savePointButtonVisible_ = true;
  // True while naming a save point created via the in-reader quick-save
  // button, so committing/cancelling that name entry resumes reading
  // instead of landing on the SavePointsList menu (which is where naming
  // one from that list itself should still end up).
  bool savePointQuickSaveFromReader_ = false;
  bool showHelpHints_ = true;
  bool showingHelpPopup_ = false;
  bool tutorialCompleted_ = false;
  const char* helpPopupTitle_ = nullptr;
  const char* helpPopupDesc_ = nullptr;
  FooterMetricMode footerMetricMode_ = FooterMetricMode::Percentage;
  BatteryLabelMode batteryLabelMode_ = BatteryLabelMode::Percent;
  ScreensaverMode screensaverMode_ = ScreensaverMode::Life;
  uint8_t screensaverTimeoutIndex_ = 2;   // default: 5 min
  uint8_t screensaverAutoOffIndex_ = 0;   // default: off (never)
  uint8_t screensaverSleepGuardIndex_ = 0; // default: off (never)
  size_t screensaverSettingsSelectedIndex_ = 1;
  PauseMode pauseMode_ = PauseMode::SentenceEnd;
  bool darkMode_ = true;
  bool nightMode_ = false;
  UiLanguage uiLanguage_ = UiLanguage::English;
  ReaderMode readerMode_ = ReaderMode::Rsvp;
  HandednessMode handednessMode_ = HandednessMode::Right;
  NavMode navMode_ = NavMode::Buttons;
  DisplayManager::TypographyConfig typographyConfig_;
};
