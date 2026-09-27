// Screens added by the 2026-09-27 update: Przejdz do (jump to %, page or
// chapter), the bookmark-name choice, the help page, the letter-color
// palette. All of them are drawn with the Nano painters (ui/NanoScreens.cpp)
// in every navigation mode; their buttons are Nano actions at
// kNanoActionBase + kExtraActionBase + n, dispatched by runExtraAction().
//
// Included at the bottom of App.cpp after AppNano.inl (shares its
// anonymous-namespace helpers and NanoSinkAdapter).

namespace {

enum ExtraAction : int {
  kExtraGoToSegment = kExtraActionBase,  // + 0 percent / 1 page / 2 chapter
  kExtraGoToMinus = kExtraActionBase + 10,
  kExtraGoToPlus,
  kExtraGoToRead,
  kExtraBookmarkDefault,
  kExtraBookmarkCustom,
};

constexpr size_t kGoToWordsPerPage = 250;

}  // namespace

bool App::isExtraScreen() const {
  switch (menuScreen_) {
    case MenuScreen::GoToPosition:
    case MenuScreen::SavePointNameChoice:
      return true;
    default:
      return false;
  }
}

bool App::renderExtraScreen() {
  if (!isExtraScreen()) {
    return false;
  }
  applyReaderUiOrientation();
  display_.setModernCardStyle(true);
  nanoSyncLayout();
  currentGridButtons_.clear();
  currentGridItemIndices_.clear();
  nanoSliderTargets_.clear();
  gridHeaderRows_ = 0;
  gridHasBack_ = false;
  gridItemsPerPage_ = 1;
  gridPageCount_ = 1;
  gridPage_ = 0;
  gridPagesVertically_ = false;
  nanoPage_ = 0;
  nanoPageFirstIndex_.clear();
  display_.nanoBeginFrame();
  switch (menuScreen_) {
    case MenuScreen::GoToPosition:
      renderGoToPosition();
      break;
    case MenuScreen::SavePointNameChoice:
      renderSavePointNameChoice();
      break;
    default:
      break;
  }
  display_.nanoEndFrame();
  return true;
}

void App::extraScreenBack(uint32_t nowMs) {
  switch (menuScreen_) {
    case MenuScreen::GoToPosition:
      if (goToFromBookDetails_ && !bookDetailsMenuItems_.empty()) {
        menuScreen_ = MenuScreen::BookDetails;
        renderBookDetails();
        return;
      }
      menuScreen_ = MenuScreen::Main;
      setState(AppState::Paused, nowMs);
      return;
    case MenuScreen::SavePointNameChoice:
      if (savePointQuickSaveFromReader_) {
        savePointQuickSaveFromReader_ = false;
        menuScreen_ = MenuScreen::Main;
        setState(AppState::Paused, nowMs);
        return;
      }
      openSavePointsList();
      return;
    default:
      menuScreen_ = MenuScreen::Main;
      renderMenu();
      return;
  }
}

void App::runExtraAction(int action, uint32_t nowMs) {
  if (action >= kExtraGoToSegment && action < kExtraGoToSegment + 3) {
    goToSegment_ = static_cast<uint8_t>(action - kExtraGoToSegment);
    if (goToSegment_ == 2 && chapterMarkers_.empty()) {
      goToSegment_ = 0;
      showGridToast(tr4(TrKey4::GoToNoChapters), nowMs);
    }
    renderMenu();
    return;
  }
  switch (action) {
    case kExtraGoToMinus:
    case kExtraGoToPlus: {
      int minimum = 0;
      int maximum = 0;
      const int value = goToSliderValue(minimum, maximum);
      goToSetSliderValue(std::max(minimum, std::min(maximum, value + (action == kExtraGoToPlus ? 1 : -1))));
      renderMenu();
      return;
    }
    case kExtraGoToRead: {
      const size_t wordCount = reader_.wordCount();
      if (wordCount == 0) {
        extraScreenBack(nowMs);
        return;
      }
      saveReadingPosition(true);
      reader_.seekTo(std::min(goToTargetWord_, wordCount - 1));
      invalidateContextPreviewWindow();
      bookDetailsMenuItems_.clear();
      goToFromBookDetails_ = false;
      menuScreen_ = MenuScreen::Main;
      setState(AppState::Paused, nowMs);
      saveReadingPosition(true);
      Serial.printf("[goto] jumped to word %u / %u\n", static_cast<unsigned>(reader_.currentIndex()),
                    static_cast<unsigned>(wordCount));
      return;
    }
    case kExtraBookmarkDefault:
      finishSavePointCreation(savePointDefaultName(), nowMs);
      return;
    case kExtraBookmarkCustom:
      openTextEntry(TextEntryPurpose::SavePointName, tr3(TrKey3::NameBookmark), tr3(TrKey3::EnterNamePrompt), "",
                    savePointDefaultName(), "", false, 30, MenuScreen::SavePointsList);
      return;
    default:
      return;
  }
}

// ─── Przejdz do ─────────────────────────────────────────────────────────────

void App::openGoToPosition(bool fromBookDetails, uint32_t nowMs) {
  if (!usingStorageBook_ || reader_.wordCount() == 0) {
    return;
  }
  goToFromBookDetails_ = fromBookDetails;
  goToTargetWord_ = reader_.currentIndex();
  goToDragging_ = false;
  if (goToSegment_ == 2 && chapterMarkers_.empty()) {
    goToSegment_ = 0;
  }
  menuScreen_ = MenuScreen::GoToPosition;
  if (state_ != AppState::Menu) {
    setState(AppState::Menu, nowMs);
  } else {
    renderMenu();
  }
}

size_t App::chapterIndexForWord(size_t wordIndex) const {
  size_t found = 0;
  for (size_t i = 0; i < chapterMarkers_.size(); ++i) {
    if (chapterMarkers_[i].wordIndex <= wordIndex) {
      found = i;
    } else {
      break;
    }
  }
  return found;
}

int App::goToSliderValue(int &minimum, int &maximum) const {
  const size_t wordCount = std::max<size_t>(1, reader_.wordCount());
  const size_t target = std::min(goToTargetWord_, wordCount - 1);
  switch (goToSegment_) {
    case 1: {
      const size_t pages = std::max<size_t>(1, (wordCount + kGoToWordsPerPage - 1) / kGoToWordsPerPage);
      minimum = 1;
      maximum = static_cast<int>(pages);
      return static_cast<int>(target / kGoToWordsPerPage) + 1;
    }
    case 2:
      minimum = 0;
      maximum = chapterMarkers_.empty() ? 0 : static_cast<int>(chapterMarkers_.size()) - 1;
      return static_cast<int>(chapterIndexForWord(target));
    default:
      minimum = 0;
      maximum = 100;
      return static_cast<int>((static_cast<uint64_t>(target) * 100U) / wordCount);
  }
}

void App::goToSetSliderValue(int value) {
  const size_t wordCount = std::max<size_t>(1, reader_.wordCount());
  switch (goToSegment_) {
    case 1:
      goToTargetWord_ = static_cast<size_t>(std::max(0, value - 1)) * kGoToWordsPerPage;
      break;
    case 2:
      if (!chapterMarkers_.empty()) {
        const size_t index = std::min(static_cast<size_t>(std::max(0, value)), chapterMarkers_.size() - 1);
        goToTargetWord_ = chapterMarkers_[index].wordIndex;
      }
      break;
    default:
      goToTargetWord_ = static_cast<size_t>((static_cast<uint64_t>(std::max(0, value)) * wordCount) / 100U);
      break;
  }
  goToTargetWord_ = std::min(goToTargetWord_, wordCount - 1);
}

void App::renderGoToPosition() {
  const size_t wordCount = std::max<size_t>(1, reader_.wordCount());
  const size_t target = std::min(goToTargetWord_, wordCount - 1);
  const size_t pages = std::max<size_t>(1, (wordCount + kGoToWordsPerPage - 1) / kGoToWordsPerPage);
  const unsigned page = static_cast<unsigned>(target / kGoToWordsPerPage) + 1;
  const unsigned percent = static_cast<unsigned>((static_cast<uint64_t>(target) * 100U) / wordCount);
  const String pageText = String(tr4(TrKey4::GoToPage)) + " " + String(page) + " " + tr4(TrKey4::GoToPageOf) + " " +
                          String(static_cast<unsigned>(pages));

  nano::GoToView view;
  view.header.backId = 0;
  view.header.title = tr4(TrKey4::GoToTitle);
  const String segments[3] = {tr4(TrKey4::GoToPercent), tr4(TrKey4::GoToPage), tr4(TrKey4::GoToChapter)};
  for (int i = 0; i < 3; ++i) {
    view.segmentIds[i] = kNanoActionBase + kExtraGoToSegment + i;
    view.segmentLabels[i] = segments[i];
  }
  view.segment = goToSegment_;
  view.sliderValue = goToSliderValue(view.sliderMin, view.sliderMax);
  view.dragging = goToDragging_;
  if (!chapterMarkers_.empty()) {
    const size_t chapter = chapterIndexForWord(target);
    view.detail = chapterMenuLabel(chapter);
  }
  switch (goToSegment_) {
    case 1:
      view.value = String(page) + " / " + String(static_cast<unsigned>(pages));
      view.hint = String(tr4(TrKey4::GoToPageSize)) + "  -  " + String(percent) + "%";
      break;
    case 2:
      view.value = String(static_cast<unsigned>(chapterIndexForWord(target) + 1)) + " / " +
                   String(static_cast<unsigned>(chapterMarkers_.size()));
      view.hint = pageText + "  -  " + String(percent) + "%";
      break;
    default:
      view.value = String(percent) + "%";
      view.hint = pageText;
      break;
  }
  const String toast = activeGridToastText(millis());
  if (!toast.isEmpty()) {
    view.hint = toast;
  }
  view.minusId = kNanoActionBase + kExtraGoToMinus;
  view.plusId = kNanoActionBase + kExtraGoToPlus;
  view.readId = kNanoActionBase + kExtraGoToRead;
  view.readLabel = tr4(TrKey4::GoToReadHere);
  NanoSinkAdapter sink(*this);
  sink.zeroIsBack = true;
  nano::paintGoTo(display_, sink, view);
}

bool App::handleGoToTouch(const TouchEvent &event, uint32_t nowMs) {
  const ui::Rect bar = nano::goToBarRect();
  // A little extra height: the bar is a thin strip under a thumb.
  const bool onBar = event.x >= bar.x && event.x < bar.x + bar.w && event.y + 12 >= bar.y &&
                     event.y < bar.y + bar.h + 12;
  if (event.phase == TouchPhase::Start) {
    goToDragging_ = onBar;
    if (!goToDragging_) {
      return false;
    }
    pausedTouch_.active = false;
  } else if (!goToDragging_) {
    return false;
  }
  int minimum = 0;
  int maximum = 0;
  goToSliderValue(minimum, maximum);
  const int x = std::max<int>(bar.x, std::min<int>(bar.x + bar.w, event.x));
  const int value = minimum + ((x - bar.x) * (maximum - minimum) + bar.w / 2) / std::max<int>(1, bar.w);
  goToSetSliderValue(value);
  if (event.phase == TouchPhase::End) {
    goToDragging_ = false;
    renderMenu();
    return true;
  }
  if (event.phase == TouchPhase::Start || nowMs - goToLastRenderMs_ >= kNanoDragFrameMs) {
    goToLastRenderMs_ = nowMs;
    renderMenu();
  }
  return true;
}

// ─── Nazwa zakladki ─────────────────────────────────────────────────────────

void App::beginSavePointNaming(uint32_t nowMs) {
  if (!savePointUseCustomName_) {
    finishSavePointCreation(savePointDefaultName(), nowMs);
    return;
  }
  menuScreen_ = MenuScreen::SavePointNameChoice;
  if (state_ != AppState::Menu) {
    setState(AppState::Menu, nowMs);
  } else {
    renderMenu();
  }
}

void App::renderSavePointNameChoice() {
  nano::ChoiceView view;
  view.header.backId = 0;
  view.header.title = tr3(TrKey3::NameBookmark);
  view.question = tr4(TrKey4::BookmarkNameQuestion);
  nano::ChoiceView::Option standard;
  standard.id = kNanoActionBase + kExtraBookmarkDefault;
  standard.label = tr4(TrKey4::BookmarkNameDefault);
  standard.detail = savePointDefaultName();
  standard.icon = NanoIcon::Bookmark;
  standard.accent = true;
  nano::ChoiceView::Option custom;
  custom.id = kNanoActionBase + kExtraBookmarkCustom;
  custom.label = tr4(TrKey4::BookmarkNameCustom);
  custom.detail = tr4(TrKey4::BookmarkNameCustomDetail);
  custom.icon = NanoIcon::Edit;
  view.options = {standard, custom};
  NanoSinkAdapter sink(*this);
  sink.zeroIsBack = true;
  nano::paintChoice(display_, sink, view);
}
