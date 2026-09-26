// Nano UI — NavMode::Modern's port of rsvpnano's "regular" presentation
// (their src/ui/screens/regular/*.cpp, the layout they ship for this same
// 640x172 Waveshare panel).
//
// Included at the bottom of App.cpp instead of compiled on its own: it
// shares that file's anonymous-namespace index constants (kSettingsHome*,
// kPressFlashMs, storedOrFallbackLabel(), ...). Screens keep running on the
// MenuScreen state machine and the existing select*Item() handlers; this
// file only decides layout and painting, and adds the interactions rsvpnano
// has and the Buttons grid doesn't (tab rail, header paging, bookshelf drag,
// chapter wheel). DisplayManager's Nano skin (display/NanoSkin.inl) paints.

#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
#include <tusb.h>
#endif
#include "text/LatinText.h"

namespace {

using NanoRole = DisplayManager::NanoRole;
using NanoAlign = DisplayManager::NanoAlign;
using NanoIcon = DisplayManager::NanoIcon;

// Offsets from kNanoActionBase (App.cpp) — canonical indices at or above
// the base are these actions; smaller ones are the screen's own item
// indices, handled by its select*Item() as usual.
enum NanoAction : int {
  kNanoTabRead = 1,
  kNanoTabSettings,
  kNanoTabDevice,
  kNanoTabPlugins,
  kNanoPowerOff,
  kNanoPagePrev,
  kNanoPageNext,
  kNanoReadResume,
  kNanoReadChapters,
  kNanoReadSavePoints,
  kNanoReadLibrary,
  kNanoReadFonts,
  kNanoSettingsScreensaver,
  kNanoPluginLibrary,
  kNanoLaunchPlugin = 100,  // + index into pluginLibrary_.enabledEntries()
};

// DeviceHome rows (App::deviceHomeActions_).
enum DeviceHomeAction : uint8_t {
  kDeviceNone,
  kDeviceSdCard,
  kDeviceVersion,
  kDeviceUpdateNow,
  kDeviceUsb,
  kDeviceSync,
  kDeviceBluetooth,
  kDeviceWifi,
  kDeviceFirmware,
  kDeviceTutorial,
};

constexpr int kNanoScreenW = BoardConfig::DISPLAY_WIDTH;
constexpr int kNanoScreenH = BoardConfig::DISPLAY_HEIGHT;
// rsvpnano uses 136; 160 lets the Polish tab names ("Ustawienia",
// "Urzadzenie") stay at 2x next to their icon instead of dropping to 1x.
constexpr int kNanoRailWidth = 160;
constexpr int kNanoContentGap = 12;
constexpr int kNanoRightInset = 48;
constexpr int kNanoHeaderHeight = 32;
constexpr int kNanoBackWidth = 56;
constexpr int kNanoPageButtonWidth = 36;
constexpr int kNanoPageLabelWidth = 44;
constexpr uint32_t kNanoDragFrameMs = 40;

ui::Rect nanoRect(int x, int y, int w, int h) {
  ui::Rect rect;
  rect.x = static_cast<uint16_t>(std::max(0, x));
  rect.y = static_cast<uint16_t>(std::max(0, y));
  rect.w = static_cast<uint16_t>(std::max(0, w));
  rect.h = static_cast<uint16_t>(std::max(0, h));
  return rect;
}

// Content area next to the rail (rsvpnano screens::detail::tabContent()).
ui::Rect nanoTabContent() {
  const int x = kNanoRailWidth + kNanoContentGap;
  return nanoRect(x, 8, kNanoScreenW - x - kNanoRightInset, kNanoScreenH - 16);
}

// Full-width content for nested screens (screens::detail::content()).
ui::Rect nanoFullContent() { return nanoRect(8, 8, kNanoScreenW - 16, kNanoScreenH - 16); }

// "Tryb zaawansowany: " -> "Tryb zaawansowany".
String nanoStripColon(const String &text) {
  String out = text;
  out.trim();
  while (out.endsWith(":")) {
    out.remove(out.length() - 1);
    out.trim();
  }
  return out;
}

// Settings rows are built as "<Name>: <value>" — split at the last ": ".
bool nanoSplitSetting(const String &item, String &label, String &value) {
  const int sep = item.lastIndexOf(": ");
  if (sep <= 0) {
    return false;
  }
  label = item.substring(0, sep);
  value = item.substring(sep + 2);
  value.trim();
  if (value.endsWith(" >")) {
    value.remove(value.length() - 2);
  }
  return true;
}

// ── Bookshelf geometry (rsvpnano screens::LibraryScreen, regular) ──
constexpr int kShelfDetailHeight = 43;
constexpr int kShelfDetailGap = 2;
constexpr int kShelfGap = 5;
constexpr int kShelfSpineBaseWidth = 29;
constexpr int kShelfSpineWidthStep = 2;
constexpr int kShelfSpinePeriod = 4;
constexpr int kShelfDragThreshold = 20;
constexpr int kShelfCycleWidth = kShelfSpinePeriod * (kShelfSpineBaseWidth + kShelfGap) +
                                 kShelfSpineWidthStep * kShelfSpinePeriod * (kShelfSpinePeriod - 1) / 2;

constexpr int shelfSpineWidth(size_t index) {
  return kShelfSpineBaseWidth + kShelfSpineWidthStep * static_cast<int>(index % kShelfSpinePeriod);
}

// Single return statement: the build is C++11, where constexpr bodies can't
// hold local variables.
constexpr int32_t shelfSpineLeft(size_t index) {
  return static_cast<int32_t>(index / kShelfSpinePeriod) * kShelfCycleWidth +
         static_cast<int32_t>(index % kShelfSpinePeriod) * (kShelfSpineBaseWidth + kShelfGap) +
         kShelfSpineWidthStep * static_cast<int32_t>(index % kShelfSpinePeriod) *
             (static_cast<int32_t>(index % kShelfSpinePeriod) - 1) / 2;
}

static_assert(shelfSpineLeft(4) == kShelfCycleWidth, "shelf spine cycle");

size_t shelfSpineIndexAt(int32_t contentX, size_t count) {
  if (count == 0 || contentX <= 0) {
    return 0;
  }
  const size_t cycle = static_cast<size_t>(contentX / kShelfCycleWidth);
  const int32_t within = contentX % kShelfCycleWidth;
  const size_t offset = within >= shelfSpineLeft(3)   ? 3
                        : within >= shelfSpineLeft(2) ? 2
                        : within >= shelfSpineLeft(1) ? 1
                                                      : 0;
  return std::min(count - 1, cycle * kShelfSpinePeriod + offset);
}

uint16_t shelfSpineColor(size_t index) {
  constexpr uint16_t kColors[] = {0x99E3, 0x1AF5, 0x0B6A, 0x7B98, 0x4490, 0xB4CD, 0x9A49, 0x32FA};
  return kColors[index % 8];
}

int shelfSpineHeight(const String &title, size_t index) {
  return std::min(110, 84 + static_cast<int>(std::min<size_t>(title.length(), 24)) / 2 +
                           static_cast<int>((index * 5) % 17));
}

struct ShelfGeometry {
  ui::Rect viewport;
  ui::Rect detail;
  int marker = 0;
};

ShelfGeometry shelfGeometry() {
  const ui::Rect content = nanoTabContent();
  ShelfGeometry g;
  const int detailY = content.y + content.h - kShelfDetailHeight;
  g.viewport = nanoRect(content.x, content.y, content.w, detailY - kShelfDetailGap - content.y);
  g.detail = nanoRect(content.x, detailY, content.w, kShelfDetailHeight);
  g.marker = g.viewport.x + g.viewport.w / 2;
  return g;
}

int32_t shelfClampOffset(size_t count, int32_t offset, int viewportWidth) {
  if (count == 0) {
    return 0;
  }
  const size_t last = count - 1;
  const int32_t lastCenter = shelfSpineLeft(last) + shelfSpineWidth(last) / 2;
  const int32_t firstCenter = shelfSpineWidth(0) / 2;
  return std::max(viewportWidth / 2 - lastCenter, std::min(offset, viewportWidth / 2 - firstCenter));
}

int32_t shelfCenteredOffset(size_t count, size_t index, int viewportWidth) {
  if (count == 0) {
    return 0;
  }
  index = std::min(index, count - 1);
  return shelfClampOffset(count, viewportWidth / 2 - shelfSpineLeft(index) - shelfSpineWidth(index) / 2,
                          viewportWidth);
}

size_t shelfNearest(size_t count, int32_t offset, int markerX, int viewportX) {
  if (count == 0) {
    return 0;
  }
  const int32_t contentX = markerX - viewportX - offset;
  const size_t candidate = shelfSpineIndexAt(contentX, count);
  const size_t first = candidate == 0 ? 0 : candidate - 1;
  const size_t last = std::min(count - 1, candidate + 1);
  size_t best = first;
  int32_t bestDistance = INT32_MAX;
  for (size_t i = first; i <= last; ++i) {
    const int32_t center = viewportX + shelfSpineLeft(i) + shelfSpineWidth(i) / 2 + offset;
    const int32_t distance = std::abs(center - markerX);
    if (distance < bestDistance) {
      bestDistance = distance;
      best = i;
    }
  }
  return best;
}

// ── Chapter wheel geometry (rsvpnano screens::ChaptersScreen, regular) ──
constexpr int kWheelRowStep = 30;
constexpr int kWheelDragThreshold = 6;

int wheelRowCenter(const ui::Rect &viewport, int row, int offset) {
  const int raw = row * kWheelRowStep + offset;
  const int magnitude = std::min(std::abs(raw), static_cast<int>(viewport.h));
  return viewport.y + viewport.h / 2 + raw * (2 * viewport.h - magnitude) / (2 * viewport.h);
}

constexpr int wheelRowHeight(bool centered) { return centered ? 28 : 18; }

bool wheelRowVisible(const ui::Rect &viewport, int y, int height) {
  return y - height / 2 >= viewport.y && y + height / 2 <= viewport.y + viewport.h;
}

ui::Rect wheelViewport() {
  const ui::Rect content = nanoTabContent();
  return nanoRect(content.x, content.y + kNanoHeaderHeight + 4, content.w,
                  content.h - kNanoHeaderHeight - 4);
}

}  // namespace

// ─── Mode / screen classification ───────────────────────────────────────────

bool App::nanoUiActive() const {
  if (navMode_ != NavMode::Modern) {
    return false;
  }
  switch (menuScreen_) {
    case MenuScreen::WelcomeLanguage:
    case MenuScreen::WelcomeTheme:
    case MenuScreen::WelcomeHighlightColor:
    case MenuScreen::WelcomeLoading:
    case MenuScreen::WelcomeSuper:
    case MenuScreen::WelcomeConfigureIntro:
    case MenuScreen::WelcomeReadingMode:
    case MenuScreen::WelcomeReadingModePreview:
    case MenuScreen::WelcomeConnect:
    case MenuScreen::WelcomeAppPairing:
    case MenuScreen::WelcomeConfigureInApp:
    case MenuScreen::TutorialStep1:
    case MenuScreen::TutorialStep2:
    case MenuScreen::TutorialStep3:
    case MenuScreen::TutorialStep4:
    case MenuScreen::TutorialStep5:
      return false;
    case MenuScreen::TypographyFontPicker:
      return !wizardFontPickerActive_;
    case MenuScreen::BookPicker:
      return !wizardBookPickerActive_;
    case MenuScreen::WifiNetworks:
    case MenuScreen::WifiSettings:
    case MenuScreen::TextEntry:
      return !wifiFlowFromWizard_;
    default:
      return true;
  }
}

bool App::nanoRailScreen() const {
  switch (menuScreen_) {
    case MenuScreen::Main:
    case MenuScreen::BookPicker:
    case MenuScreen::ChapterPicker:
    case MenuScreen::SavePointsList:
    case MenuScreen::SettingsHome:
    case MenuScreen::DeviceHome:
    case MenuScreen::PluginsHome:
      return true;
    default:
      return false;
  }
}

App::NanoTab App::nanoActiveTab() const {
  switch (menuScreen_) {
    case MenuScreen::SettingsHome:
    case MenuScreen::SettingsDisplay:
    case MenuScreen::SettingsPacing:
    case MenuScreen::ScreensaverSettings:
    case MenuScreen::Presets:
    case MenuScreen::PresetsDeleteConfirm:
    case MenuScreen::TypographyTuning:
    case MenuScreen::TypographyResetConfirm:
      return NanoTab::Settings;
    case MenuScreen::DeviceHome:
    case MenuScreen::WifiSettings:
    case MenuScreen::WifiNetworks:
    case MenuScreen::SettingsConnectivity:
    case MenuScreen::SettingsAbout:
    case MenuScreen::SdCardRepairConfirm:
    case MenuScreen::UpdateConfirm:
      return NanoTab::Device;
    case MenuScreen::PluginsHome:
    case MenuScreen::PluginsActive:
    case MenuScreen::PluginLibraryScreen:
    case MenuScreen::PluginDetail:
      return NanoTab::Plugins;
    case MenuScreen::TypographyFontPicker:
      return nanoFontPickerFromRead_ ? NanoTab::Read : NanoTab::Settings;
    default:
      return NanoTab::Read;
  }
}

String App::nanoScreenTitle() const {
  switch (menuScreen_) {
    case MenuScreen::SettingsDisplay:
      return uiText(UiText::Display);
    case MenuScreen::SettingsPacing:
      return tr3(TrKey3::ReadingSettings);
    case MenuScreen::ScreensaverSettings:
      return nanoStripColon(tr(TrKey::Screensaver));
    case MenuScreen::WifiSettings:
      return "Wi-Fi";
    case MenuScreen::WifiNetworks:
      return nanoStripColon(tr(TrKey::ChooseNetwork));
    case MenuScreen::SettingsConnectivity:
      return tr(TrKey::Connectivity);
    case MenuScreen::SettingsAbout:
      return tr(TrKey::AboutHelp);
    case MenuScreen::Presets:
      return tr3(TrKey3::PresetsLabel);
    case MenuScreen::PluginsActive:
      return tr3(TrKey3::ActivePlugins);
    case MenuScreen::PluginLibraryScreen:
      return tr2(TrKey2::PluginLibrary);
    case MenuScreen::PluginDetail: {
      const auto &all = pluginLibrary_.all();
      return pluginDetailIndex_ < all.size() ? all[pluginDetailIndex_].name : uiText(UiText::Plugins);
    }
    case MenuScreen::TypographyFontPicker:
      return uiText(UiText::Typeface);
    case MenuScreen::BookDetails:
      return storage_.bookDisplayName(bookDetailsBookIndex_);
    case MenuScreen::ChapterPicker:
      return uiText(UiText::Chapters);
    case MenuScreen::SavePointsList:
      return uiText(UiText::SavePoints);
    default:
      return "";
  }
}

// ─── Tap targets ────────────────────────────────────────────────────────────

void App::nanoAddTarget(const ui::Rect &rect, int canonicalIndex, const String &label, ui::IconId icon) {
  DisplayManager::Button target;
  target.x = rect.x;
  target.y = rect.y;
  target.width = rect.w;
  target.height = rect.h;
  target.label = label;
  target.icon = icon;
  currentGridButtons_.push_back(target);
  currentGridItemIndices_.push_back(static_cast<size_t>(canonicalIndex));
}

bool App::nanoPressed(int canonicalIndex) const {
  return canonicalIndex >= 0 && isGridItemFlashing(static_cast<size_t>(canonicalIndex), millis());
}

bool App::nanoArmed(int canonicalIndex) const {
  return canonicalIndex >= 0 && isGridItemArmed(static_cast<size_t>(canonicalIndex), millis());
}

// ─── Entry points (called by the generic menu renderers) ────────────────────

void App::renderNanoScreen(const String &title, const std::vector<String> &items, size_t selectedIndex,
                           size_t headerRows) {
  (void)selectedIndex;
  applyReaderUiOrientation();
  display_.setModernCardStyle(true);
  currentGridButtons_.clear();
  currentGridItemIndices_.clear();
  // Paging is Nano's own (header arrows / nanoChangePage()); keep the
  // Buttons-grid swipe pager from acting on these screens.
  gridHeaderRows_ = headerRows;
  gridHasBack_ = false;
  gridItemsPerPage_ = 1;
  gridPageCount_ = 1;
  gridPage_ = 0;
  gridPagesVertically_ = false;
  nanoPage_ = 0;
  nanoPageFirstIndex_.clear();

  display_.nanoBeginFrame();
  switch (menuScreen_) {
    case MenuScreen::Main:
      renderNanoRead();
      break;
    case MenuScreen::SettingsHome:
      renderNanoSettingsHome();
      break;
    case MenuScreen::DeviceHome:
      renderNanoDeviceHome();
      break;
    case MenuScreen::PluginsHome:
      renderNanoPluginsHome();
      break;
    case MenuScreen::BookPicker:
      renderNanoShelf();
      break;
    case MenuScreen::ChapterPicker:
      renderNanoChapters();
      break;
    case MenuScreen::SavePointsList:
      renderNanoSavePoints();
      break;
    case MenuScreen::BookDetails:
      renderNanoBookDetails();
      break;
    case MenuScreen::RestartConfirm:
    case MenuScreen::TypographyResetConfirm:
    case MenuScreen::SdCardRepairConfirm:
    case MenuScreen::UpdateConfirm:
    case MenuScreen::BookDeleteConfirm:
    case MenuScreen::SavePointDeleteConfirm:
    case MenuScreen::PresetsDeleteConfirm:
      renderNanoConfirm(title, items, headerRows);
      break;
    default:
      renderNanoList(title, items, headerRows);
      break;
  }
  if (nanoRailScreen()) {
    renderNanoRail();
  }
  display_.nanoEndFrame();
}

void App::renderNanoLibraryList(const std::vector<DisplayManager::LibraryItem> &items, size_t selectedIndex,
                                const String &title) {
  if (menuScreen_ == MenuScreen::BookPicker) {
    renderNanoScreen(title, {}, selectedIndex, 0);
    return;
  }
  // Wi-Fi network list: SSID left, signal/security right.
  std::vector<String> titles;
  std::vector<String> subtitles;
  titles.reserve(items.size());
  subtitles.reserve(items.size());
  for (const DisplayManager::LibraryItem &item : items) {
    titles.push_back(item.title);
    subtitles.push_back(item.subtitle);
  }
  applyReaderUiOrientation();
  display_.setModernCardStyle(true);
  currentGridButtons_.clear();
  currentGridItemIndices_.clear();
  gridHeaderRows_ = 0;
  gridHasBack_ = false;
  gridItemsPerPage_ = 1;
  gridPageCount_ = 1;
  gridPage_ = 0;
  gridPagesVertically_ = false;
  nanoPage_ = 0;
  nanoPageFirstIndex_.clear();
  display_.nanoBeginFrame();
  renderNanoList(title, titles, 0, subtitles);
  display_.nanoEndFrame();
}

// ─── Rail (rsvpnano screens::detail::navigation) ────────────────────────────

void App::renderNanoRail() {
  struct TabSpec {
    int action;
    String label;
    NanoIcon icon;
    NanoTab tab;
  };
  std::vector<TabSpec> tabs;
  tabs.reserve(4);
  tabs.push_back({kNanoTabRead, uiText(UiText::Read), NanoIcon::Books, NanoTab::Read});
  tabs.push_back({kNanoTabSettings, uiText(UiText::Settings), NanoIcon::Edit, NanoTab::Settings});
  tabs.push_back({kNanoTabDevice, tr3(TrKey3::NanoDeviceTab), NanoIcon::Device, NanoTab::Device});
  // Plugins are an advanced-mode feature everywhere else in the UI too.
  if (devModeEnabled()) {
    tabs.push_back({kNanoTabPlugins, uiText(UiText::Plugins), NanoIcon::Apps, NanoTab::Plugins});
  }

  const NanoTab active = nanoActiveTab();
  const int tabHeight = kNanoScreenH / static_cast<int>(tabs.size());
  int y = 0;
  for (size_t i = 0; i < tabs.size(); ++i) {
    const int h = (i + 1 == tabs.size()) ? kNanoScreenH - y : tabHeight;
    const ui::Rect rect = nanoRect(0, y, kNanoRailWidth, h);
    const int action = kNanoActionBase + tabs[i].action;
    const bool badge = tabs[i].tab == NanoTab::Device && otaUpdatePromptPending_;
    display_.nanoTab(rect, tabs[i].label, tabs[i].tab == active, tabs[i].icon, nanoPressed(action), badge);
    nanoAddTarget(rect, action, tabs[i].label);
    y += h;
  }

  const ui::Rect power = nanoRect(kNanoScreenW - 46, 4, 36, 36);
  display_.nanoIconButton(power, NanoIcon::Power, nanoPressed(kNanoActionBase + kNanoPowerOff));
  nanoAddTarget(power, kNanoActionBase + kNanoPowerOff);
  display_.nanoBatteryStack(nanoRect(kNanoScreenW - 46, 48, 36, 28));
}

// ─── Czytaj (rsvpnano screens::read) ────────────────────────────────────────

void App::renderNanoRead() {
  nanoFontPickerFromRead_ = false;
  const ui::Rect area = nanoTabContent();
  constexpr int kHeaderHeight = 64;
  constexpr int kHeaderActionSize = 40;
  constexpr int kHeaderGap = 8;
  constexpr int kRowGap = 18;

  String title = tr3(TrKey3::NanoNoBook);
  String author;
  String progress;
  if (usingStorageBook_) {
    title = storage_.bookDisplayName(currentBookIndex_);
    author = storage_.bookAuthorName(currentBookIndex_);
    if (author.isEmpty()) {
      author = tr3(TrKey3::NanoUnknownAuthor);
    }
    progress = String(static_cast<unsigned>(readingProgressPercent())) + "%";
  }

  const ui::Rect resume =
      nanoRect(area.x, area.y, area.w - kHeaderActionSize - kHeaderGap, kHeaderHeight);
  const int resumeAction = kNanoActionBase + kNanoReadResume;
  display_.nanoButton(resume, title, true, NanoIcon::Bookmark, 2, author, progress,
                      nanoPressed(resumeAction));
  nanoAddTarget(resume, resumeAction, title);

  const ui::Rect fonts = nanoRect(area.x + area.w - kHeaderActionSize,
                                  area.y + (kHeaderHeight - kHeaderActionSize) / 2, kHeaderActionSize,
                                  kHeaderActionSize);
  const int fontsAction = kNanoActionBase + kNanoReadFonts;
  display_.nanoIconButton(fonts, NanoIcon::Language, nanoPressed(fontsAction));
  nanoAddTarget(fonts, fontsAction);

  // rsvpnano has Chapters + Library here; save points are ours and sit
  // between them.
  const int rowY = area.y + kHeaderHeight + kRowGap;
  constexpr int kGap = 10;
  const int buttonWidth = (area.w - kGap * 2) / 3;
  struct ActionSpec {
    int action;
    String label;
  };
  const ActionSpec actions[] = {
      {kNanoReadChapters, uiText(UiText::Chapters)},
      {kNanoReadSavePoints, uiText(UiText::SavePoints)},
      {kNanoReadLibrary, uiText(UiText::Library)},
  };
  for (size_t i = 0; i < 3; ++i) {
    const int x = area.x + static_cast<int>(i) * (buttonWidth + kGap);
    const int w = (i == 2) ? area.x + area.w - x : buttonWidth;
    const ui::Rect rect = nanoRect(x, rowY, w, kHeaderHeight);
    const int action = kNanoActionBase + actions[i].action;
    display_.nanoButton(rect, actions[i].label, true, NanoIcon::None, 2, "", "", nanoPressed(action));
    nanoAddTarget(rect, action, actions[i].label);
  }
}

// ─── Ustawienia (rsvpnano screens::settings) ────────────────────────────────

void App::renderNanoSettingsHome() {
  nanoScreensaverFromSettingsHome_ = false;
  const ui::Rect content = nanoTabContent();
  constexpr int kRowHeight = 36;
  constexpr int kGap = 6;
  const int half = (content.w - kGap) / 2;
  const int rightX = content.x + half + kGap;
  const int rightW = content.x + content.w - rightX;

  auto settingButton = [&](const ui::Rect &rect, int index, const String &label) {
    display_.nanoButton(rect, label, true, NanoIcon::None, 1, "", "", nanoPressed(index));
    nanoAddTarget(rect, index, label);
  };

  int y = content.y;
  display_.nanoSeparator(nanoRect(content.x, y, content.w, 12), tr3(TrKey3::NanoReadingSection));
  y += 18;
  settingButton(nanoRect(content.x, y, half, kRowHeight), kSettingsHomeReadingIndex,
                tr3(TrKey3::ReadingSettings));
  settingButton(nanoRect(rightX, y, rightW, kRowHeight), kSettingsHomeTypographyIndex,
                uiText(UiText::TypographyTune));
  y += kRowHeight + kGap;

  display_.nanoSeparator(nanoRect(content.x, y, content.w, 12), tr3(TrKey3::NanoSystemSection));
  y += 18;
  settingButton(nanoRect(content.x, y, half, kRowHeight), kSettingsHomeDisplayIndex,
                uiText(UiText::Display));
  const int screensaverAction = kNanoActionBase + kNanoSettingsScreensaver;
  const String screensaverLabel = nanoStripColon(tr(TrKey::Screensaver));
  const ui::Rect screensaver = nanoRect(rightX, y, rightW, kRowHeight);
  display_.nanoButton(screensaver, screensaverLabel, true, NanoIcon::None, 1, "", "",
                      nanoPressed(screensaverAction));
  nanoAddTarget(screensaver, screensaverAction, screensaverLabel);
  y += kRowHeight + kGap;

  const bool advanced = devModeEnabled();
  const String advancedLabel = nanoStripColon(tr3(TrKey3::AdvancedModeColon));
  const ui::Rect toggle = nanoRect(content.x, y, advanced ? half : content.w, kRowHeight);
  display_.nanoToggle(toggle, advancedLabel, advanced, nanoPressed(kSettingsHomeAdvancedIndex));
  nanoAddTarget(toggle, kSettingsHomeAdvancedIndex, advancedLabel);
  if (advanced) {
    settingButton(nanoRect(rightX, y, rightW, kRowHeight), kSettingsHomePresetsIndex,
                  tr3(TrKey3::PresetsLabel));
  }
}

// ─── Urzadzenie (rsvpnano screens::device) ──────────────────────────────────

void App::openDeviceHome() {
  menuScreen_ = MenuScreen::DeviceHome;
  settingsSelectedIndex_ = 1;
  rebuildSettingsMenuItems();
  renderSettings();
}

void App::rebuildDeviceHomeItems() {
  deviceHomeActions_.clear();
  settingsMenuItems_.push_back(uiText(UiText::Back));
  deviceHomeActions_.push_back(kDeviceNone);
  auto add = [this](uint8_t action, const String &label) {
    settingsMenuItems_.push_back(label);
    deviceHomeActions_.push_back(action);
  };
  add(kDeviceSdCard, String(tr3(TrKey3::NanoSdCard)) + ": " +
                         String(static_cast<unsigned>(storage_.bookCount())) + " " +
                         tr3(TrKey3::NanoBooksCount));
  add(kDeviceVersion, String(tr(TrKey::Version)) + otaUpdater_.currentVersion());
  if (otaUpdatePromptPending_) {
    add(kDeviceUpdateNow, String(tr2(TrKey2::Update)) + ": " + pendingUpdateNewVersion_);
  }
#if RSVP_USB_TRANSFER_ENABLED
  add(kDeviceUsb, uiText(UiText::UsbTransfer));
#endif
  add(kDeviceSync, String(tr(TrKey::PhoneSync)) +
                       (state_ == AppState::CompanionSync ? tr(TrKey::Yes) : tr(TrKey::No)));
#if FLOWER_BLE_ENABLED
  add(kDeviceBluetooth,
      String("Bluetooth: ") +
          (ble_.isActive() ? (ble_.isConnected() ? tr(TrKey::Connected) : tr(TrKey::Yes)) : tr(TrKey::No)));
#endif
  add(kDeviceWifi, String(tr(TrKey::HomeWifi)) + storedOrFallbackLabel(configuredWifiSsid(), tr(TrKey::NotSet)));
  if (devModeEnabled()) {
    add(kDeviceFirmware, firmwareUpdateMenuLabel());
  }
  add(kDeviceTutorial, tr3(TrKey3::TutorialLabel));
}

void App::renderNanoDeviceHome() {
  const ui::Rect content = nanoTabContent();
  constexpr int kGap = 4;
  constexpr int kStatusHeight = 44;
  const int statusWidth = (content.w - kGap) / 2;

  // Row 1: the two status tiles (rsvpnano: Storage / Encryption).
  size_t index = 1;
  for (int column = 0; column < 2 && index < settingsMenuItems_.size(); ++column, ++index) {
    const int x = content.x + column * (statusWidth + kGap);
    const ui::Rect rect = nanoRect(x, content.y, column == 0 ? statusWidth : content.x + content.w - x,
                                   kStatusHeight);
    String label;
    String value;
    if (!nanoSplitSetting(settingsMenuItems_[index], label, value)) {
      label = settingsMenuItems_[index];
    }
    display_.nanoSetting(rect, label, value, false, nanoPressed(static_cast<int>(index)));
    nanoAddTarget(rect, static_cast<int>(index), settingsMenuItems_[index]);
  }

  // Rows 2-3: actions (rsvpnano: 2x2 USB / Sync / RSS / OTA; ours has a
  // few more, so 3 columns, 4 once an update is also waiting).
  const size_t first = index;
  const size_t count = settingsMenuItems_.size() > first ? settingsMenuItems_.size() - first : 0;
  const int columns = count > 6 ? 4 : 3;
  const int actionsY = content.y + kStatusHeight + kGap;
  const int actionsHeight = content.y + content.h - actionsY;
  const int rowHeight = (actionsHeight - kGap) / 2;
  const int cellWidth = (content.w - kGap * (columns - 1)) / columns;
  for (size_t i = 0; i < count && i < static_cast<size_t>(columns * 2); ++i) {
    const size_t item = first + i;
    const int column = static_cast<int>(i) % columns;
    const int row = static_cast<int>(i) / columns;
    const int x = content.x + column * (cellWidth + kGap);
    const int w = column == columns - 1 ? content.x + content.w - x : cellWidth;
    const ui::Rect rect = nanoRect(x, actionsY + row * (rowHeight + kGap), w, rowHeight);
    const int canonical = static_cast<int>(item);
    String label;
    String value;
    if (nanoSplitSetting(settingsMenuItems_[item], label, value)) {
      display_.nanoSetting(rect, label, value, false, nanoPressed(canonical));
    } else {
      display_.nanoButton(rect, settingsMenuItems_[item], true, NanoIcon::None, 2, "", "",
                          nanoPressed(canonical));
    }
    nanoAddTarget(rect, canonical, settingsMenuItems_[item]);
  }
}

void App::selectDeviceHomeItem(uint32_t nowMs) {
  if (settingsSelectedIndex_ == 0 || settingsSelectedIndex_ >= deviceHomeActions_.size()) {
    menuScreen_ = MenuScreen::Main;
    renderMainMenu();
    return;
  }
  switch (deviceHomeActions_[settingsSelectedIndex_]) {
    case kDeviceSdCard:
      runSdCardCheck(nowMs);
      return;
    case kDeviceVersion: {
      // Same 10-tap developer unlock as Informacje > Wersja.
      constexpr uint32_t kTapWindowMs = 1500;
      constexpr uint8_t kTapsToUnlock = 10;
      if (aboutLastTapMs_ != 0 && nowMs - aboutLastTapMs_ > kTapWindowMs) {
        aboutTapCount_ = 0;
      }
      aboutLastTapMs_ = nowMs;
      ++aboutTapCount_;
      if (!devModeEnabled() && aboutTapCount_ >= kTapsToUnlock) {
        setDevModeEnabled(true);
        aboutTapCount_ = 0;
      }
      rebuildSettingsMenuItems();
      renderSettings();
      return;
    }
    case kDeviceUpdateNow:
      otaUpdatePromptPending_ = false;
      runFirmwareUpdate(preferredOtaConfig(), false, nowMs);
      return;
    case kDeviceUsb:
#if RSVP_USB_TRANSFER_ENABLED
      enterUsbTransfer(nowMs);
#endif
      return;
    case kDeviceSync:
      if (state_ == AppState::CompanionSync) {
        exitCompanionSync(nowMs);
      } else {
        enterCompanionSync(nowMs);
      }
      return;
    case kDeviceBluetooth:
#if FLOWER_BLE_ENABLED
      if (ble_.isActive()) {
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
#endif
      return;
    case kDeviceWifi:
      openWifiSettings();
      return;
    case kDeviceFirmware:
      runFirmwareUpdate(preferredOtaConfig(), false, nowMs);
      return;
    case kDeviceTutorial:
      openTutorialStep1();
      return;
    default:
      return;
  }
}

// ─── Pluginy (rsvpnano's Focus tab slot) ────────────────────────────────────

void App::renderNanoPluginsHome() {
  const ui::Rect content = nanoTabContent();
  constexpr int kGap = 6;
  constexpr int kRows = 3;
  const auto enabled = pluginLibrary_.enabledEntries();
  const size_t count = enabled.size() + 1;  // + library button
  const int columns = count > 6 ? 3 : 2;
  const int rowHeight = (content.h - kGap * (kRows - 1)) / kRows;
  const int cellWidth = (content.w - kGap * (columns - 1)) / columns;

  size_t cell = 0;
  if (enabled.empty()) {
    display_.nanoLabel(nanoRect(content.x, content.y, content.w, rowHeight * 2 + kGap),
                       tr3(TrKey3::NoActivePlugins), 2, NanoRole::Muted, NanoAlign::Center, 2);
    cell = static_cast<size_t>(columns) * 2;
  }
  auto cellRect = [&](size_t index, bool fullRow) {
    const int column = static_cast<int>(index) % columns;
    const int row = static_cast<int>(index) / columns;
    const int x = content.x + column * (cellWidth + kGap);
    const int w = fullRow ? content.w : (column == columns - 1 ? content.x + content.w - x : cellWidth);
    return nanoRect(fullRow ? content.x : x, content.y + row * (rowHeight + kGap), w, rowHeight);
  };
  const size_t maxCells = static_cast<size_t>(columns * kRows);
  for (size_t i = 0; i < enabled.size() && cell + 1 < maxCells; ++i, ++cell) {
    const int action = kNanoActionBase + kNanoLaunchPlugin + static_cast<int>(i);
    const ui::Rect rect = cellRect(cell, false);
    display_.nanoButton(rect, enabled[i].name, true, NanoIcon::None, 2, "", "", nanoPressed(action));
    nanoAddTarget(rect, action, enabled[i].name);
  }
  // Library: last cell, or a full-width bottom row when nothing is enabled.
  const bool fullRow = enabled.empty();
  const ui::Rect library = fullRow ? cellRect(static_cast<size_t>(columns) * 2, true) : cellRect(cell, false);
  const int libraryAction = kNanoActionBase + kNanoPluginLibrary;
  display_.nanoButton(library, tr2(TrKey2::PluginLibrary), true, NanoIcon::Books, 2, "", "",
                      nanoPressed(libraryAction));
  nanoAddTarget(library, libraryAction, tr2(TrKey2::PluginLibrary));
}

// ─── Biblioteka: bookshelf (rsvpnano screens::LibraryScreen, regular) ───────

void App::renderNanoShelf() {
  const ShelfGeometry g = shelfGeometry();
  const size_t count = bookMenuItems_.size() > 1 ? bookMenuItems_.size() - 1 : 0;
  if (count == 0) {
    display_.nanoLabel(g.viewport, tr3(TrKey3::NanoNoLibraryItems), 2, NanoRole::Muted, NanoAlign::Center, 2);
    return;
  }
  const size_t selected = bookPickerSelectedIndex_ > 0 ? std::min(bookPickerSelectedIndex_ - 1, count - 1) : 0;
  if (!nanoShelfDragging_) {
    nanoShelfOffset_ = shelfCenteredOffset(count, selected, g.viewport.w);
  }

  const uint16_t foreground = display_.nanoColor(NanoRole::Foreground);
  const uint16_t accent = display_.nanoColor(NanoRole::Accent);
  const uint16_t outline = display_.nanoColor(NanoRole::Outline);
  // Spines may scroll under the rail — clip to the shelf column.
  display_.nanoSetClip(g.viewport.x, 0, g.viewport.w, g.viewport.y + g.viewport.h + 2);
  display_.nanoFillRect(g.marker, g.viewport.y, 1, g.viewport.h, display_.nanoColor(NanoRole::ProgressTrack));

  const int32_t contentLeft = -nanoShelfOffset_;
  const size_t firstVisible = shelfSpineIndexAt(contentLeft, count);
  const size_t lastVisible = shelfSpineIndexAt(contentLeft + g.viewport.w, count);
  for (size_t i = firstVisible; i <= lastVisible && i < count; ++i) {
    const DisplayManager::LibraryItem &item = bookMenuItems_[i + 1];
    const int width = shelfSpineWidth(i);
    const int height = shelfSpineHeight(item.title, i);
    const int x = g.viewport.x + static_cast<int>(shelfSpineLeft(i) + nanoShelfOffset_);
    const bool active = i == selected;
    const int y = g.viewport.y + g.viewport.h - height - (active ? 8 : 0);
    const uint16_t fill = shelfSpineColor(i);
    display_.nanoFillRect(x, y, width, height, fill);
    display_.nanoDrawRect(x, y, width, height, foreground);
    if (active) {
      display_.nanoFillRect(x, y - 2, width, 2, accent);
    }
    if (item.progressPercent > 0) {
      // Bookmark ribbon hanging from the top, as long as the progress.
      const int ribbonX = x + width - 9;
      const int ribbonHeight = std::max(8, height * item.progressPercent / 100);
      display_.nanoFillRect(ribbonX, y, 5, ribbonHeight, 0xDACA);
      for (int row = 0; row < 3; ++row) {
        display_.nanoFillRect(ribbonX + 2 - row, y + ribbonHeight - 3 + row, row * 2 + 1, 1, fill);
      }
    }
    // Spine lettering: up to 7 upper-case letters/digits, top to bottom,
    // leading English article dropped, Polish letters folded to ASCII.
    String title = item.title;
    String lower = title;
    lower.toLowerCase();
    if (lower.startsWith("the ")) {
      title = title.substring(4);
    } else if (lower.startsWith("an ")) {
      title = title.substring(3);
    } else if (lower.startsWith("a ")) {
      title = title.substring(2);
    }
    int letterY = y + 6;
    size_t written = 0;
    auto drawLetter = [&](char c) {
      display_.nanoSmallGlyph(x + width / 2 - 3, letterY, c, 0xFF9C);
      letterY += 11;
      ++written;
    };
    for (size_t c = 0; c < title.length() && written < 7 && letterY + 8 < y + height; ++c) {
      uint8_t value = LatinText::byteValue(title[c]);
      if (value >= 0x80 || value < 0x20) {
        value = LatinText::fallbackAsciiByte(value);
      }
      char letter = static_cast<char>(value);
      if (letter >= 'a' && letter <= 'z') {
        letter = static_cast<char>(letter - 'a' + 'A');
      }
      if ((letter >= 'A' && letter <= 'Z') || (letter >= '0' && letter <= '9')) {
        drawLetter(letter);
      }
    }
    if (written == 0) {
      for (const char letter : {'B', 'O', 'O', 'K'}) {
        if (letterY + 8 >= y + height) break;
        drawLetter(letter);
      }
    }
  }
  display_.nanoFillRect(g.viewport.x, g.viewport.y + g.viewport.h, g.viewport.w, 2, outline);
  display_.nanoResetClip();

  // Detail strip: title, author, status, progress percent.
  constexpr int kProgressWidth = 76;
  constexpr int kDetailGap = 12;
  const int textWidth = g.detail.w - kProgressWidth - kDetailGap;
  const size_t bookIndex = selected < bookPickerBookIndices_.size() ? bookPickerBookIndices_[selected] : 0;
  const String author = storage_.bookAuthorName(bookIndex);
  uint8_t percent = 0;
  const bool hasProgress = bookProgressPercent(bookIndex, percent);
  display_.nanoText(nanoRect(g.detail.x, g.detail.y, textWidth, 18), bookMenuItems_[selected + 1].title, 2,
                    foreground);
  display_.nanoText(nanoRect(g.detail.x, g.detail.y + 20, textWidth, 10),
                    author.isEmpty() ? String(tr3(TrKey3::NanoUnknownAuthor)) : author, 1,
                    display_.nanoColor(NanoRole::Muted));
  if (usingStorageBook_ && bookIndex == currentBookIndex_) {
    display_.nanoText(nanoRect(g.detail.x, g.detail.y + 33, textWidth, 10), uiText(UiText::CurrentBook), 1,
                      display_.nanoColor(NanoRole::Muted));
  }
  display_.nanoText(nanoRect(g.detail.x + g.detail.w - kProgressWidth, g.detail.y, kProgressWidth, g.detail.h),
                    String(static_cast<unsigned>(hasProgress ? percent : 0)) + "%", 3, accent, NanoAlign::End);
}

// ─── Rozdzialy: chapter wheel (rsvpnano screens::ChaptersScreen) ────────────

void App::renderNanoChapters() {
  const ui::Rect content = nanoTabContent();
  const ui::Rect back = nanoRect(content.x, content.y, 64, kNanoHeaderHeight);
  display_.nanoButton(back, "<<", true, NanoIcon::None, 1, "", "", nanoPressed(0));
  nanoAddTarget(back, 0, "", ui::IconId::Back);

  const size_t count = chapterMarkers_.size();
  const size_t readingIndex = currentChapterIndex();
  if (nanoWheelCentered_ >= count && count > 0) {
    nanoWheelCentered_ = count - 1;
  }
  const int positionWidth = std::min(84, content.w / 4);
  display_.nanoLabel(nanoRect(content.x + 68, content.y, content.w - positionWidth - 72, kNanoHeaderHeight),
                     uiText(UiText::Chapters), 2, NanoRole::Foreground, NanoAlign::Center);
  display_.nanoLabel(nanoRect(content.x + content.w - positionWidth, content.y, positionWidth, kNanoHeaderHeight),
                     String(static_cast<unsigned>(count == 0 ? 0 : nanoWheelCentered_ + 1)) + " / " +
                         String(static_cast<unsigned>(count)),
                     1, NanoRole::Muted, NanoAlign::End);

  const ui::Rect viewport = wheelViewport();
  if (count == 0) {
    // "Start of book" (chapterMenuItems_[1], see openChapterPicker()).
    const String label = uiText(UiText::StartOfBook);
    display_.nanoButton(viewport, label, true, NanoIcon::None, 1, "", "",
                        nanoPressed(static_cast<int>(kChapterPickerFallbackIndex)));
    nanoAddTarget(viewport, static_cast<int>(kChapterPickerFallbackIndex), label);
    return;
  }

  const size_t first = nanoWheelCentered_ > 4 ? nanoWheelCentered_ - 4 : 0;
  const size_t last = std::min(count, nanoWheelCentered_ + 5);
  const int centerY = viewport.y + viewport.h / 2;
  const int halfHeight = std::max(1, viewport.h / 2);
  const int maximumWidth = std::max(40, viewport.w - 20);
  const uint16_t background = display_.nanoColor(NanoRole::Background);
  display_.nanoSetClip(viewport.x, viewport.y, viewport.w, viewport.h);
  for (size_t i = first; i < last; ++i) {
    const int y = wheelRowCenter(viewport, static_cast<int>(i) - static_cast<int>(nanoWheelCentered_),
                                 nanoWheelOffset_);
    const int curved = y - centerY;
    const bool centered = i == nanoWheelCentered_;
    const uint8_t alpha =
        centered ? 255 : static_cast<uint8_t>(std::max(48, 220 - std::abs(curved) * 172 / halfHeight));
    const int height = wheelRowHeight(centered);
    const int width = centered ? maximumWidth
                               : maximumWidth - std::min(std::abs(curved), halfHeight) * (maximumWidth / 3) /
                                                    halfHeight;
    const int x = viewport.x + (viewport.w - width) / 2;
    const int top = y - height / 2;
    if (!wheelRowVisible(viewport, y, height)) {
      continue;
    }
    const int right = x + width - 1;
    const int notch = std::min(10, height / 2);
    const uint16_t surface =
        centered ? display_.nanoColor(NanoRole::SurfaceActive) : display_.nanoBlend(NanoRole::SurfaceMuted, alpha);
    const uint16_t outline =
        centered ? display_.nanoColor(NanoRole::Outline) : display_.nanoBlend(NanoRole::Outline, alpha);
    display_.nanoFillRoundRect(x, top, width, height, 5, surface);
    if (i == readingIndex) {
      const int tailLeft = right - notch - 6;
      display_.nanoFillRect(tailLeft, top, right - tailLeft + 1, height,
                            centered ? display_.nanoColor(NanoRole::Accent)
                                     : display_.nanoBlend(NanoRole::Accent, alpha));
    }
    display_.nanoDrawRoundRect(x, top, width, height, 5, outline);
    display_.nanoFillTriangle(right - notch, y, right, top, right, top + height - 1, background);
    display_.nanoDrawLine(right - notch, y, right, top, outline);
    display_.nanoDrawLine(right - notch, y, right, top + height - 1, outline);
    if (centered) {
      display_.nanoFillRect(x + 4, top + 3, 3, height - 6, display_.nanoColor(NanoRole::Accent));
    }
    const String title = chapterMarkers_[i].title.isEmpty()
                             ? uiText(UiText::Chapters) + " " + String(static_cast<unsigned>(i + 1))
                             : chapterMarkers_[i].title;
    display_.nanoText(nanoRect(x + 10, top, width - notch - 24, height), title, centered ? 2 : 1,
                      display_.nanoBlend(NanoRole::Foreground, alpha), NanoAlign::Center);
  }
  display_.nanoResetClip();
}

// ─── Punkty zapisu ──────────────────────────────────────────────────────────

void App::renderNanoSavePoints() {
  const ui::Rect content = nanoTabContent();
  constexpr int kGap = 5;
  constexpr int kRows = 3;
  constexpr int kDeleteWidth = 96;
  const int rowHeight = (content.h - kNanoHeaderHeight - 6 - kGap * (kRows - 1)) / kRows;

  // Row 0 is "+ Dodaj punkt zapisu" (index 1), then one row per save point
  // (name at 2+2k, its delete at 3+2k — see openSavePointsList()).
  const size_t pointCount = savePointMenuItems_.size() > 2 ? (savePointMenuItems_.size() - 2) / 2 : 0;
  const size_t rowCount = 1 + pointCount;
  const size_t pageCount = std::max<size_t>(1, (rowCount + kRows - 1) / kRows);
  const size_t selectedRow = savePointSelectedIndex_ <= 1 ? 0 : 1 + (savePointSelectedIndex_ - 2) / 2;
  const size_t page = std::min(selectedRow / kRows, pageCount - 1);
  nanoPage_ = page;
  for (size_t p = 0; p < pageCount; ++p) {
    const size_t row = p * kRows;
    nanoPageFirstIndex_.push_back(row == 0 ? 1 : 2 + (row - 1) * 2);
  }

  // Header: <<, title, pager.
  const ui::Rect back = nanoRect(content.x, content.y, kNanoBackWidth, kNanoHeaderHeight);
  display_.nanoButton(back, "<<", true, NanoIcon::None, 1, "", "", nanoPressed(0));
  nanoAddTarget(back, 0, "", ui::IconId::Back);
  int titleRight = content.x + content.w;
  if (pageCount > 1) {
    const int nextX = content.x + content.w - kNanoPageButtonWidth;
    const int labelX = nextX - kNanoPageLabelWidth;
    const int prevX = labelX - kNanoPageButtonWidth;
    const ui::Rect prev = nanoRect(prevX, content.y, kNanoPageButtonWidth, kNanoHeaderHeight);
    const ui::Rect next = nanoRect(nextX, content.y, kNanoPageButtonWidth, kNanoHeaderHeight);
    display_.nanoButton(prev, "<", page > 0, NanoIcon::None, 1, "", "",
                        nanoPressed(kNanoActionBase + kNanoPagePrev));
    display_.nanoButton(next, ">", page + 1 < pageCount, NanoIcon::None, 1, "", "",
                        nanoPressed(kNanoActionBase + kNanoPageNext));
    if (page > 0) nanoAddTarget(prev, kNanoActionBase + kNanoPagePrev);
    if (page + 1 < pageCount) nanoAddTarget(next, kNanoActionBase + kNanoPageNext);
    display_.nanoLabel(nanoRect(labelX, content.y, kNanoPageLabelWidth, kNanoHeaderHeight),
                       String(static_cast<unsigned>(page + 1)) + "/" + String(static_cast<unsigned>(pageCount)), 2,
                       NanoRole::Muted, NanoAlign::Center);
    titleRight = prevX - 8;
  }
  display_.nanoLabel(nanoRect(content.x + kNanoBackWidth + 8, content.y,
                              titleRight - (content.x + kNanoBackWidth + 8), kNanoHeaderHeight),
                     uiText(UiText::SavePoints), 2, NanoRole::Foreground);

  const int listY = content.y + kNanoHeaderHeight + 6;
  for (size_t r = 0; r < static_cast<size_t>(kRows); ++r) {
    const size_t row = page * kRows + r;
    if (row >= rowCount) {
      break;
    }
    const int y = listY + static_cast<int>(r) * (rowHeight + kGap);
    if (row == 0) {
      const ui::Rect add = nanoRect(content.x, y, content.w, rowHeight);
      display_.nanoButton(add, savePointMenuItems_[1], true, NanoIcon::None, 1, "", "", nanoPressed(1));
      nanoAddTarget(add, 1, savePointMenuItems_[1]);
      continue;
    }
    const size_t nameIndex = 2 + (row - 1) * 2;
    const size_t deleteIndex = nameIndex + 1;
    if (deleteIndex >= savePointMenuItems_.size()) {
      break;
    }
    const ui::Rect name = nanoRect(content.x, y, content.w - kDeleteWidth - kGap, rowHeight);
    const ui::Rect remove = nanoRect(content.x + content.w - kDeleteWidth, y, kDeleteWidth, rowHeight);
    display_.nanoButton(name, savePointMenuItems_[nameIndex], true, NanoIcon::Bookmark, 1, "", "",
                        nanoPressed(static_cast<int>(nameIndex)));
    nanoAddTarget(name, static_cast<int>(nameIndex), savePointMenuItems_[nameIndex]);
    display_.nanoButton(remove, nanoStripColon(tr3(TrKey3::DeleteSpace)), true, NanoIcon::None, 1, "", "",
                        nanoPressed(static_cast<int>(deleteIndex)), nanoArmed(static_cast<int>(deleteIndex)));
    nanoAddTarget(remove, static_cast<int>(deleteIndex), savePointMenuItems_[deleteIndex]);
  }
}

// ─── Szczegoly ksiazki ──────────────────────────────────────────────────────

void App::renderNanoBookDetails() {
  const ui::Rect content = nanoFullContent();
  constexpr int kGap = 6;
  const ui::Rect back = nanoRect(content.x, content.y, kNanoBackWidth, kNanoHeaderHeight);
  display_.nanoButton(back, "<<", true, NanoIcon::None, 1, "", "", nanoPressed(0));
  nanoAddTarget(back, 0, "", ui::IconId::Back);
  display_.nanoLabel(nanoRect(content.x + kNanoBackWidth + 8, content.y, content.w - kNanoBackWidth - 8,
                              kNanoHeaderHeight),
                     storage_.bookDisplayName(bookDetailsBookIndex_), 2, NanoRole::Foreground);

  const String author = storage_.bookAuthorName(bookDetailsBookIndex_);
  uint8_t percent = 0;
  bookProgressPercent(bookDetailsBookIndex_, percent);
  const int infoY = content.y + kNanoHeaderHeight + 6;
  display_.nanoLabel(nanoRect(content.x, infoY, content.w - 80, 18),
                     author.isEmpty() ? String(tr3(TrKey3::NanoUnknownAuthor)) : author, 2, NanoRole::Muted);
  display_.nanoLabel(nanoRect(content.x + content.w - 76, infoY, 76, 18),
                     String(static_cast<unsigned>(percent)) + "%", 2, NanoRole::Accent, NanoAlign::End);
  display_.nanoProgress(nanoRect(content.x, infoY + 22, content.w, 6), percent, 0, 100);

  // Actions: bookDetailsMenuItems_[3..6] = read on / chapters / restart / delete.
  const int gridY = infoY + 36;
  const int rowHeight = (content.y + content.h - gridY - kGap) / 2;
  const int half = (content.w - kGap) / 2;
  for (size_t i = 0; i < 4; ++i) {
    const size_t index = 3 + i;
    if (index >= bookDetailsMenuItems_.size()) {
      break;
    }
    const int column = static_cast<int>(i % 2);
    const int row = static_cast<int>(i / 2);
    const int x = content.x + column * (half + kGap);
    const ui::Rect rect =
        nanoRect(x, gridY + row * (rowHeight + kGap), column == 1 ? content.x + content.w - x : half, rowHeight);
    display_.nanoButton(rect, bookDetailsMenuItems_[index], true, NanoIcon::None, 1, "", "",
                        nanoPressed(static_cast<int>(index)));
    nanoAddTarget(rect, static_cast<int>(index), bookDetailsMenuItems_[index]);
  }
}

// ─── Confirm dialogs (rsvpnano screens::storageEncryption panel) ────────────

void App::renderNanoConfirm(const String &title, const std::vector<String> &items, size_t headerRows) {
  (void)title;
  const ui::Rect content = nanoFullContent();
  constexpr int kPanelWidth = 440;
  constexpr int kPanelHeight = 144;
  const int width = std::min(static_cast<int>(content.w), kPanelWidth);
  const int height = std::min(static_cast<int>(content.h), kPanelHeight);
  const ui::Rect panel =
      nanoRect(content.x + (content.w - width) / 2, content.y + (content.h - height) / 2, width, height);

  // Question text: the header rows, or for the list-style confirms the info
  // row right after Back ("Usun: <tytul>").
  String question;
  std::vector<size_t> actions;  // canonical indices shown as buttons
  bool hasBack = false;
  const bool infoRow = menuScreen_ == MenuScreen::BookDeleteConfirm ||
                       menuScreen_ == MenuScreen::SavePointDeleteConfirm;
  for (size_t i = 0; i < headerRows && i < items.size(); ++i) {
    question += (question.isEmpty() ? "" : " ") + items[i];
  }
  for (size_t i = headerRows; i < items.size(); ++i) {
    const size_t canonical = i - headerRows;
    if (canonical == 0 && items[i] == uiText(UiText::Back)) {
      hasBack = true;
      continue;
    }
    if (infoRow && canonical == 1) {
      question = items[i];
      continue;
    }
    actions.push_back(canonical);
  }
  if (menuScreen_ == MenuScreen::PresetsDeleteConfirm && !actions.empty()) {
    String label;
    String value;
    if (nanoSplitSetting(items[headerRows + actions[0]], label, value)) {
      question = value;
    }
  }

  constexpr int kTitleHeight = 60;
  display_.nanoLabel(nanoRect(panel.x, panel.y, panel.w, kTitleHeight), question, 2, NanoRole::Foreground,
                     NanoAlign::Center, 2);
  const int buttonsY = panel.y + kTitleHeight + 20;
  const int buttonsHeight = panel.y + panel.h - buttonsY;
  constexpr int kGap = 8;
  int x = panel.x;
  const size_t slots = actions.size() + (hasBack ? 1 : 0);
  if (slots == 0) {
    return;
  }
  const int backWidth = hasBack ? 64 : 0;
  const int actionWidth = actions.empty()
                              ? 0
                              : (panel.w - (hasBack ? backWidth + kGap : 0) -
                                 kGap * (static_cast<int>(actions.size()) - 1)) /
                                    static_cast<int>(actions.size());
  if (hasBack) {
    const ui::Rect back = nanoRect(x, buttonsY, backWidth, buttonsHeight);
    display_.nanoButton(back, "<<", true, NanoIcon::None, 1, "", "", nanoPressed(0));
    nanoAddTarget(back, 0, "", ui::IconId::Back);
    x += backWidth + kGap;
  }
  for (size_t i = 0; i < actions.size(); ++i) {
    const size_t canonical = actions[i];
    const String &item = items[headerRows + canonical];
    String label = item;
    String value;
    if (menuScreen_ == MenuScreen::PresetsDeleteConfirm) {
      String name;
      if (nanoSplitSetting(item, name, value)) {
        label = name;
      }
    }
    const int w = (i + 1 == actions.size()) ? panel.x + panel.w - x : actionWidth;
    const ui::Rect rect = nanoRect(x, buttonsY, w, buttonsHeight);
    display_.nanoButton(rect, label, true, NanoIcon::None, 2, "", "", nanoPressed(static_cast<int>(canonical)),
                        nanoArmed(static_cast<int>(canonical)));
    nanoAddTarget(rect, static_cast<int>(canonical), item);
    x += w + kGap;
  }
}

// ─── Generic list (every other menu screen) ─────────────────────────────────

void App::renderNanoList(const String &title, const std::vector<String> &items, size_t headerRows,
                         const std::vector<String> &subtitles) {
  if (menuScreen_ == MenuScreen::SettingsDisplay) {
    nanoScreensaverFromSettingsHome_ = false;
  }
  const bool rail = nanoRailScreen();
  const ui::Rect area = rail ? nanoTabContent() : nanoFullContent();
  size_t itemCount = 0;
  const size_t *selectedPtr = currentMenuSelectedIndexPtr(itemCount);
  const size_t selected = selectedPtr != nullptr ? *selectedPtr : 0;

  const size_t actionable = items.size() > headerRows ? items.size() - headerRows : 0;
  const bool hasBack = actionable > 0 && items[headerRows] == uiText(UiText::Back);
  const size_t firstTile = hasBack ? 1 : 0;
  const size_t tileCount = actionable > firstTile ? actionable - firstTile : 0;

  // Two columns of rsvpnano setting rows when the items carry values ("Motyw:
  // Ciemny"), three columns of plain buttons otherwise (two next to the rail).
  bool settingsLike = !subtitles.empty();
  for (size_t i = firstTile; i < actionable && !settingsLike; ++i) {
    settingsLike = items[headerRows + i].indexOf(": ") > 0;
  }
  const bool detailScreen = menuScreen_ == MenuScreen::PluginDetail;
  const int columns = detailScreen ? 1 : (settingsLike || rail) ? 2 : 3;
  constexpr int kRows = 3;
  constexpr int kGap = 5;
  const size_t perPage = static_cast<size_t>(columns * kRows);
  const size_t pageCount = std::max<size_t>(1, (tileCount + perPage - 1) / perPage);
  const size_t selectedTile = selected > firstTile ? selected - firstTile : 0;
  const size_t page = std::min(selectedTile / perPage, pageCount - 1);
  nanoPage_ = page;
  for (size_t p = 0; p < pageCount; ++p) {
    nanoPageFirstIndex_.push_back(firstTile + p * perPage);
  }

  // Header row: "<<", title, pager.
  int titleX = area.x;
  if (hasBack) {
    const ui::Rect back = nanoRect(area.x, area.y, kNanoBackWidth, kNanoHeaderHeight);
    display_.nanoButton(back, "<<", true, NanoIcon::None, 1, "", "", nanoPressed(0));
    nanoAddTarget(back, 0, "", ui::IconId::Back);
    titleX += kNanoBackWidth + 8;
  }
  int titleRight = area.x + area.w;
  if (pageCount > 1) {
    const int nextX = area.x + area.w - kNanoPageButtonWidth;
    const int labelX = nextX - kNanoPageLabelWidth;
    const int prevX = labelX - kNanoPageButtonWidth;
    const ui::Rect prev = nanoRect(prevX, area.y, kNanoPageButtonWidth, kNanoHeaderHeight);
    const ui::Rect next = nanoRect(nextX, area.y, kNanoPageButtonWidth, kNanoHeaderHeight);
    display_.nanoButton(prev, "<", page > 0, NanoIcon::None, 1, "", "",
                        nanoPressed(kNanoActionBase + kNanoPagePrev));
    display_.nanoButton(next, ">", page + 1 < pageCount, NanoIcon::None, 1, "", "",
                        nanoPressed(kNanoActionBase + kNanoPageNext));
    if (page > 0) nanoAddTarget(prev, kNanoActionBase + kNanoPagePrev);
    if (page + 1 < pageCount) nanoAddTarget(next, kNanoActionBase + kNanoPageNext);
    display_.nanoLabel(nanoRect(labelX, area.y, kNanoPageLabelWidth, kNanoHeaderHeight),
                       String(static_cast<unsigned>(page + 1)) + "/" + String(static_cast<unsigned>(pageCount)), 2,
                       NanoRole::Muted, NanoAlign::Center);
    titleRight = prevX - 8;
  }
  const String heading = title.isEmpty() ? nanoScreenTitle() : title;
  display_.nanoLabel(nanoRect(titleX, area.y, titleRight - titleX, kNanoHeaderHeight), heading, 2,
                     NanoRole::Foreground);

  const int gridY = area.y + kNanoHeaderHeight + 6;
  const int gridH = area.y + area.h - gridY;
  const int rowHeight = (gridH - kGap * (kRows - 1)) / kRows;
  const int cellWidth = (area.w - kGap * (columns - 1)) / columns;
  const size_t pageStart = page * perPage;
  for (size_t i = 0; i < perPage && pageStart + i < tileCount; ++i) {
    const size_t canonical = firstTile + pageStart + i;
    const String &item = items[headerRows + canonical];
    const int column = static_cast<int>(i) % columns;
    const int row = static_cast<int>(i) / columns;
    const int x = area.x + column * (cellWidth + kGap);
    const int w = column == columns - 1 ? area.x + area.w - x : cellWidth;
    ui::Rect rect = nanoRect(x, gridY + row * (rowHeight + kGap), w, rowHeight);
    const int index = static_cast<int>(canonical);
    const bool pressed = nanoPressed(index);

    if (item == "---") {
      display_.nanoSeparator(rect, "");
      continue;
    }

    // Reuse the Buttons-grid annotations for what each row is.
    DisplayManager::Button info;
    info.label = item;
    if (menuScreen_ == MenuScreen::SettingsDisplay) {
      annotateSettingsDisplayButton(info, canonical);
    } else if (menuScreen_ == MenuScreen::PluginDetail) {
      annotatePluginDetailButton(info, canonical);
    } else if (menuScreen_ == MenuScreen::TypographyFontPicker) {
      annotateTypographyFontPickerButton(info, canonical);
    }

    if (info.kind == DisplayManager::Button::ButtonKind::Label) {
      // Plugin description: plain muted prose, two rows tall.
      rect.h = static_cast<uint16_t>(rowHeight * 2 + kGap);
      display_.nanoLabel(rect, info.sublabel.isEmpty() ? info.label : info.label + " " + info.sublabel, 2,
                         NanoRole::Muted, NanoAlign::Start, 2);
      continue;
    }
    if (detailScreen) {
      // Enable/disable button under the description.
      rect.y = static_cast<uint16_t>(gridY + 2 * (rowHeight + kGap));
    }
    if (info.kind == DisplayManager::Button::ButtonKind::Toggle) {
      display_.nanoToggle(rect, info.label, info.active, pressed);
    } else if (canonical < subtitles.size()) {
      display_.nanoSetting(rect, item, subtitles[canonical], true, pressed);
    } else {
      String label;
      String value;
      if (nanoSplitSetting(item, label, value) && !nanoArmed(index)) {
        display_.nanoSetting(rect, label, value, true, pressed);
      } else {
        // Font picker: the selection always equals the active face (set on
        // open, moved by the tap that applies a face), so mark it.
        const bool currentFace =
            menuScreen_ == MenuScreen::TypographyFontPicker && canonical == selected;
        display_.nanoButton(rect, item, true, currentFace ? NanoIcon::Bookmark : NanoIcon::None, 2, "", "",
                            pressed, nanoArmed(index), info.previewTypeface);
      }
    }
    nanoAddTarget(rect, index, item);
  }
}

// ─── Actions ────────────────────────────────────────────────────────────────

bool App::nanoChangePage(int delta, bool fromSwipe) {
  if (nanoPageFirstIndex_.size() <= 1) {
    return false;
  }
  const int target = std::max(0, std::min(static_cast<int>(nanoPageFirstIndex_.size()) - 1,
                                          static_cast<int>(nanoPage_) + delta));
  if (static_cast<size_t>(target) != nanoPage_) {
    size_t itemCount = 0;
    size_t *selected = currentMenuSelectedIndexPtr(itemCount);
    if (selected != nullptr) {
      *selected = nanoPageFirstIndex_[static_cast<size_t>(target)];
      if (fromSwipe) {
        // Same settle-time guard as handleGridPageSwipe().
        lastGridPageChangeAtMs_ = millis();
      }
      renderMenu();
    }
  }
  return true;
}

void App::runNanoAction(int action, uint32_t nowMs) {
  if (action >= kNanoLaunchPlugin) {
    const auto enabled = pluginLibrary_.enabledEntries();
    const size_t index = static_cast<size_t>(action - kNanoLaunchPlugin);
    if (index < enabled.size()) {
      pluginsActiveSelectedIndex_ = index + 1;
      selectPluginsActiveItem(nowMs);
    }
    return;
  }
  switch (action) {
    case kNanoTabRead:
      menuScreen_ = MenuScreen::Main;
      renderMainMenu();
      return;
    case kNanoTabSettings:
      openSettings();
      return;
    case kNanoTabDevice:
      openDeviceHome();
      return;
    case kNanoTabPlugins:
      openPluginsHome();
      return;
    case kNanoPowerOff:
      enterPowerOff(nowMs);
      return;
    case kNanoPagePrev:
      nanoChangePage(-1, false);
      return;
    case kNanoPageNext:
      nanoChangePage(1, false);
      return;
    case kNanoReadResume:
      if (!usingStorageBook_ && storage_.bookCount() > 0) {
        openBookPicker(false);
      } else {
        setState(AppState::Paused, nowMs);
      }
      return;
    case kNanoReadChapters:
      // Back from the wheel lands on Czytaj, not on a stale book-details page.
      bookDetailsMenuItems_.clear();
      openChapterPicker();
      return;
    case kNanoReadSavePoints:
      openSavePointsList();
      return;
    case kNanoReadLibrary:
      openBookPicker(false);
      return;
    case kNanoReadFonts:
      nanoFontPickerFromRead_ = true;
      openTypographyFontPicker();
      return;
    case kNanoSettingsScreensaver:
      nanoScreensaverFromSettingsHome_ = true;
      openScreensaverSettings();
      return;
    case kNanoPluginLibrary:
      openPluginLibraryScreen();
      return;
    default:
      return;
  }
}

void App::returnFromPlugin() {
  if (navMode_ == NavMode::Modern) {
    openPluginsHome();
  } else {
    openPluginsActive();
  }
}

// ─── Live-drag screens ──────────────────────────────────────────────────────

bool App::handleNanoTouch(const TouchEvent &event, uint32_t nowMs) {
  if (menuScreen_ == MenuScreen::BookPicker) {
    const ShelfGeometry g = shelfGeometry();
    const size_t count = bookMenuItems_.size() > 1 ? bookMenuItems_.size() - 1 : 0;
    if (event.phase == TouchPhase::Start) {
      nanoShelfDragging_ = count > 0 && g.viewport.contains(event.x, event.y);
      nanoShelfDetailTouch_ = !nanoShelfDragging_ && g.detail.contains(event.x, event.y);
      if (!nanoShelfDragging_ && !nanoShelfDetailTouch_) {
        return false;
      }
      nanoShelfMoved_ = false;
      nanoShelfDragStartX_ = event.x;
      nanoShelfDragStartY_ = event.y;
      nanoShelfDragStartOffset_ = nanoShelfOffset_;
      pausedTouch_.active = false;
      return true;
    }
    const bool trackingShelf = nanoShelfDragging_;
    const bool trackingDetail = nanoShelfDetailTouch_;
    if (!trackingShelf && !trackingDetail) {
      return false;
    }
    const int dx = static_cast<int>(event.x) - nanoShelfDragStartX_;
    const int dy = static_cast<int>(event.y) - nanoShelfDragStartY_;
    nanoShelfMoved_ = nanoShelfMoved_ || std::abs(dx) > kShelfDragThreshold || std::abs(dy) > kShelfDragThreshold;
    if (trackingShelf && nanoShelfMoved_) {
      nanoShelfOffset_ = shelfClampOffset(count, nanoShelfDragStartOffset_ + dx, g.viewport.w);
      bookPickerSelectedIndex_ = shelfNearest(count, nanoShelfOffset_, g.marker, g.viewport.x) + 1;
    }
    if (event.phase == TouchPhase::Move) {
      if (trackingShelf && nanoShelfMoved_ && nowMs - nanoShelfLastDragRenderMs_ >= kNanoDragFrameMs) {
        nanoShelfLastDragRenderMs_ = nowMs;
        renderBookPicker();
      }
      return true;
    }
    // End.
    nanoShelfDragging_ = false;
    nanoShelfDetailTouch_ = false;
    if (nanoShelfMoved_) {
      renderBookPicker();  // snaps to the selected spine
      return true;
    }
    if (trackingDetail) {
      if (!g.detail.contains(event.x, event.y)) {
        return true;
      }
      // Tap on the title strip: the book's detail page (chapters, restart,
      // delete).
      if (count > 0 && bookPickerSelectedIndex_ >= 1 &&
          bookPickerSelectedIndex_ - 1 < bookPickerBookIndices_.size()) {
        openBookDetails(bookPickerBookIndices_[bookPickerSelectedIndex_ - 1], nowMs);
      }
      return true;
    }
    // Tap on the shelf: pick the spine under the finger (or keep the
    // selected one) and open that book on the Czytaj tab.
    if (count == 0 || (lastMenuActionAtMs_ != 0 && nowMs - lastMenuActionAtMs_ < kMenuActionDebounceMs)) {
      return true;
    }
    const int32_t contentX = static_cast<int32_t>(event.x) - g.viewport.x - nanoShelfOffset_;
    const size_t tapped = shelfSpineIndexAt(contentX, count);
    const int spineX = g.viewport.x + static_cast<int>(shelfSpineLeft(tapped) + nanoShelfOffset_);
    if (static_cast<int>(event.x) >= spineX && static_cast<int>(event.x) < spineX + shelfSpineWidth(tapped)) {
      bookPickerSelectedIndex_ = tapped + 1;
    }
    lastMenuActionAtMs_ = nowMs;
    const size_t row = bookPickerSelectedIndex_ > 0 ? bookPickerSelectedIndex_ - 1 : 0;
    if (row >= bookPickerBookIndices_.size()) {
      return true;
    }
    const size_t bookIndex = bookPickerBookIndices_[row];
    renderBookPicker();
    if (!(usingStorageBook_ && bookIndex == currentBookIndex_)) {
      saveReadingPosition(true);
      if (!loadBookAtIndex(bookIndex, nowMs, true, true, true, true)) {
        display_.renderStatus(tr3(TrKey3::ErrorLabel), storage_.bookDisplayName(bookIndex), "");
        delay(1400);
        renderBookPicker();
        return true;
      }
    }
    bookDetailsMenuItems_.clear();
    menuScreen_ = MenuScreen::Main;
    renderMainMenu();
    return true;
  }

  if (menuScreen_ == MenuScreen::ChapterPicker && !chapterMarkers_.empty()) {
    const ui::Rect viewport = wheelViewport();
    const size_t count = chapterMarkers_.size();
    if (event.phase == TouchPhase::Start) {
      if (!viewport.contains(event.x, event.y)) {
        return false;
      }
      nanoWheelDragging_ = true;
      nanoWheelMoved_ = false;
      nanoWheelDragStartIndex_ = nanoWheelCentered_;
      nanoWheelDragStartY_ = event.y;
      pausedTouch_.active = false;
      return true;
    }
    if (!nanoWheelDragging_) {
      return false;
    }
    const int delta = static_cast<int>(event.y) - nanoWheelDragStartY_;
    nanoWheelMoved_ = nanoWheelMoved_ || std::abs(delta) > kWheelDragThreshold;
    if (nanoWheelMoved_) {
      // Displacement owns the selection: holding still never advances.
      const int64_t position = std::max<int64_t>(
          0, std::min<int64_t>(static_cast<int64_t>(nanoWheelDragStartIndex_) * kWheelRowStep - delta,
                               static_cast<int64_t>(count - 1) * kWheelRowStep));
      nanoWheelCentered_ = static_cast<size_t>((position + kWheelRowStep / 2) / kWheelRowStep);
      nanoWheelOffset_ = static_cast<int16_t>(static_cast<int64_t>(nanoWheelCentered_) * kWheelRowStep - position);
    }
    if (event.phase == TouchPhase::Move) {
      if (nanoWheelMoved_ && nowMs - nanoWheelLastDragRenderMs_ >= kNanoDragFrameMs) {
        nanoWheelLastDragRenderMs_ = nowMs;
        renderChapterPicker();
      }
      return true;
    }
    nanoWheelDragging_ = false;
    if (!nanoWheelMoved_ && viewport.contains(event.x, event.y)) {
      const size_t first = nanoWheelCentered_ > 4 ? nanoWheelCentered_ - 4 : 0;
      const size_t last = std::min(count, nanoWheelCentered_ + 5);
      size_t tappedIndex = count;
      int closest = kWheelRowStep / 2 + 1;
      for (size_t i = first; i < last; ++i) {
        const int y = wheelRowCenter(viewport, static_cast<int>(i) - static_cast<int>(nanoWheelCentered_),
                                     nanoWheelOffset_);
        if (!wheelRowVisible(viewport, y, wheelRowHeight(i == nanoWheelCentered_))) {
          continue;
        }
        const int distance = std::abs(y - static_cast<int>(event.y));
        if (distance < closest) {
          closest = distance;
          tappedIndex = i;
        }
      }
      if (tappedIndex != count) {
        nanoWheelCentered_ = tappedIndex;
        nanoWheelOffset_ = 0;
        chapterPickerSelectedIndex_ = tappedIndex + 1;
        selectChapterPickerItem(nowMs);
        return true;
      }
    }
    nanoWheelOffset_ = 0;  // snap to the highlighted chapter
    renderChapterPicker();
    return true;
  }
  return false;
}

bool App::batteryChargingNow() const {
  if (!batteryPresent_) {
    return false;
  }
#if RSVP_USB_TRANSFER_ENABLED && CONFIG_TINYUSB_MSC_ENABLED && !ARDUINO_USB_MODE
  // Enumerated by a computer = on USB power. A plain wall charger doesn't
  // enumerate; that case falls back to the voltage check below.
  if (tud_inited() && tud_mounted()) {
    return true;
  }
#endif
  // A charger holds the cell near 4.2 V; a resting pack sits below that.
  return batteryFilteredVoltage_ >= 4.18f;
}
