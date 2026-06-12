// firmware/src/plugins/builtin/FocusTimerPlugin.cpp
#include "plugins/builtin/FocusTimerPlugin.h"

namespace {

constexpr uint32_t kOrientationStableMs = 700;
constexpr uint32_t kTouchStartArmDelayMs = 350;
constexpr uint32_t kPostTimerFlipGraceMs = 900;
constexpr uint32_t kFeedbackMs = 900;
constexpr uint32_t kTouchDurationMs = 2UL * 60UL * 1000UL;
constexpr uint32_t kWorkDurationMs = 20UL * 60UL * 1000UL;
constexpr uint32_t kBreakDurationMs = 5UL * 60UL * 1000UL;

constexpr float kSideAxisThreshold = 0.78f;
constexpr float kCrossAxisLimit = 0.42f;
constexpr float kFlatAxisThreshold = 0.84f;

// Genre menu items (index 0 = Back, 1..5 = genres)
constexpr uint8_t kGenreMenuItemCount = 6;
constexpr uint8_t kGenreBackIndex = 0;
constexpr uint8_t kGenreFirstIndex = 1;

// Touch cancel hold thresholds
constexpr uint16_t kCancelHoldMaxDriftPx = 20;
constexpr uint32_t kCancelHoldMs = 850;

// Tap detection
constexpr uint16_t kTapSlopPx = 20;
constexpr uint16_t kBackTapWidth = 80;
constexpr uint16_t kBackTapHeight = 35;

// Static genre menu labels
static const char* kGenreMenuLabels[kGenreMenuItemCount] = {
    "< Back",
    "Chores - 15 min",
    "Work - 25 min",
    "Fitness - 30 min",
    "Self Care - 20 min",
    "Other - 10 min",
};

// Singleton instance — created on init, destroyed on destroy
FocusTimerCore* s_instance = nullptr;

}  // namespace

// ─── FocusTimerCore Implementation ──────────────────────────────────────────

FocusTimerCore::FocusTimerCore(PluginImuService* imu, PluginAudioService* audio,
                               PluginDisplayService* display,
                               PluginOrientationService* orientation)
    : imu_(imu), audio_(audio), display_(display), orientation_(orientation) {}

bool FocusTimerCore::begin() {
  imuAvailable_ = imu_ && imu_->available && imu_->available();
  return true;
}

void FocusTimerCore::open() {
  // Re-probe IMU in case it wasn't ready at boot
  if (!imuAvailable_ && imu_ && imu_->available) {
    imuAvailable_ = imu_->available();
  }

  clearSession();
  resetOrientationStability();
  state_ = imuAvailable_ ? State::GenreSelect : State::Unavailable;
  stateStartedMs_ = lastUpdateMs_;
  genreSelectedIndex_ = kGenreFirstIndex;
  cancelHoldTriggered_ = false;
  touchActive_ = false;
}

void FocusTimerCore::update(uint32_t nowMs) {
  lastUpdateMs_ = nowMs;

  if (imuAvailable_) {
    updateOrientation(nowMs);
  }

  switch (state_) {
    case State::Unavailable:
    case State::GenreSelect:
      break;

    case State::WaitForTouchStart:
      if (orientationInputArmed(nowMs) && isShortSide(stableOrientation_)) {
        startMode(TimerMode::Touch, nowMs, kTouchDurationMs, stableOrientation_);
        transitionTo(State::TouchRunning, nowMs);
      }
      break;

    case State::TouchRunning:
      if (timerExpired(nowMs)) {
        completeActiveTimer();
        resetOrientationStability();
        transitionTo(State::WaitAfterTouch, nowMs);
      }
      break;

    case State::WaitAfterTouch:
      if (!orientationInputArmed(nowMs)) {
        break;
      }
      if (stableOrientation_ == oppositeShortSide(lastShortSide_)) {
        startMode(TimerMode::Work, nowMs, kWorkDurationMs, stableOrientation_);
        transitionTo(State::WorkRunning, nowMs);
      } else if (stableOrientation_ == OrientationState::LongSide) {
        startMode(TimerMode::Break, nowMs, kBreakDurationMs, OrientationState::LongSide);
        transitionTo(State::BreakRunning, nowMs);
      }
      break;

    case State::WorkRunning:
      if (timerExpired(nowMs)) {
        completeActiveTimer();
        resetOrientationStability();
        transitionTo(State::WaitAfterWork, nowMs);
      }
      break;

    case State::WaitAfterWork:
      if (!orientationInputArmed(nowMs)) {
        break;
      }
      if (stableOrientation_ == oppositeShortSide(lastShortSide_)) {
        startMode(TimerMode::Work, nowMs, kWorkDurationMs, stableOrientation_);
        transitionTo(State::WorkRunning, nowMs);
      } else if (stableOrientation_ == OrientationState::LongSide) {
        startMode(TimerMode::Break, nowMs, kBreakDurationMs, OrientationState::LongSide);
        transitionTo(State::BreakRunning, nowMs);
      }
      break;

    case State::BreakRunning:
      if (timerExpired(nowMs)) {
        completeActiveTimer();
        resetOrientationStability();
        transitionTo(State::WaitAfterBreak, nowMs);
      }
      break;

    case State::WaitAfterBreak:
      if (orientationInputArmed(nowMs) && isShortSide(stableOrientation_)) {
        startMode(TimerMode::Work, nowMs, kWorkDurationMs, stableOrientation_);
        transitionTo(State::WorkRunning, nowMs);
      }
      break;

    case State::Cancelled:
      if (nowMs - feedbackStartedMs_ >= kFeedbackMs) {
        resetOrientationStability();
        transitionTo(State::WaitForTouchStart, nowMs);
      }
      break;

    case State::Complete:
      if (nowMs - feedbackStartedMs_ >= kFeedbackMs) {
        clearSession();
        resetOrientationStability();
        transitionTo(imuAvailable_ ? State::GenreSelect : State::Unavailable, nowMs);
      }
      break;
  }

  // Play completion cue via audio service
  if (completionCuePending_ && audio_ && audio_->beep) {
    audio_->beep();
    completionCuePending_ = false;
  }

  // Update the UI orientation based on current state
  updateUiOrientation();
}

void FocusTimerCore::handleButton(const PluginButtonEvent* event) {
  if (!event) {
    return;
  }

  const uint32_t nowMs = event->timestampMs;

  // Button 0 (boot button) — used for menu navigation and cancel
  if (event->buttonId == 0) {
    if (!event->pressed) {
      return;
    }

    if (state_ == State::GenreSelect) {
      // In genre select: button 0 = move selection down
      moveGenreSelection(1);
      return;
    }

    // During timer running: cancel via button press
    if (timerRunning_) {
      cancelActiveTimer(nowMs);
      return;
    }
    return;
  }

  // Button 1 (power button) — select/confirm
  if (event->buttonId == 1) {
    if (!event->pressed) {
      return;
    }

    if (state_ == State::GenreSelect) {
      selectGenreMenuItem(nowMs);
      return;
    }
    return;
  }
}

void FocusTimerCore::handleTouch(const PluginTouchEvent* event) {
  if (!event) {
    return;
  }

  const uint32_t nowMs = event->timestampMs;

  // ─── Genre Select: handle touch as tap on menu rows ───
  if (state_ == State::GenreSelect) {
    if (event->phase == 2) {  // Touch end
      // Calculate which row was tapped
      int height = display_ && display_->logicalHeight ? display_->logicalHeight() : 172;
      int rowHeight = height / static_cast<int>(kGenreMenuItemCount);
      if (rowHeight < 1) rowHeight = 1;
      uint8_t tappedIndex = static_cast<uint8_t>(event->y / rowHeight);
      if (tappedIndex >= kGenreMenuItemCount) {
        tappedIndex = kGenreMenuItemCount - 1;
      }
      genreSelectedIndex_ = tappedIndex;
      selectGenreMenuItem(nowMs);
    }
    return;
  }

  // ─── Timer Session: touch-hold to cancel, tap top-left to exit ───
  if (event->phase == 0) {  // Touch begin
    touchActive_ = true;
    touchStartX_ = event->x;
    touchStartY_ = event->y;
    touchStartMs_ = nowMs;
    cancelHoldTriggered_ = false;
    return;
  }

  if (!touchActive_) {
    return;
  }

  const int deltaX = static_cast<int>(event->x) - static_cast<int>(touchStartX_);
  const int deltaY = static_cast<int>(event->y) - static_cast<int>(touchStartY_);
  const int absDeltaX = deltaX < 0 ? -deltaX : deltaX;
  const int absDeltaY = deltaY < 0 ? -deltaY : deltaY;

  // Long-press cancel detection during timer running
  if (timerRunning_ && !cancelHoldTriggered_ && event->phase != 2 &&
      absDeltaX <= static_cast<int>(kCancelHoldMaxDriftPx) &&
      absDeltaY <= static_cast<int>(kCancelHoldMaxDriftPx) &&
      nowMs - touchStartMs_ >= kCancelHoldMs) {
    cancelActiveTimer(nowMs);
    touchActive_ = false;
    cancelHoldTriggered_ = true;
    return;
  }

  if (event->phase != 2) {  // Not end
    return;
  }

  // Touch ended
  touchActive_ = false;

  if (cancelHoldTriggered_) {
    cancelHoldTriggered_ = false;
    return;
  }

  const bool tapLike = absDeltaX <= static_cast<int>(kTapSlopPx) &&
                       absDeltaY <= static_cast<int>(kTapSlopPx);

  // Top-left tap = exit timer back to caller (abandon)
  if (tapLike && event->x < kBackTapWidth && event->y < kBackTapHeight) {
    abandon();
    return;
  }
}

void FocusTimerCore::draw() {
  if (!display_) {
    return;
  }

  // ─── Genre select: render menu ───
  if (state_ == State::GenreSelect) {
    if (display_->renderMenu) {
      display_->renderMenu(kGenreMenuLabels, kGenreMenuItemCount, genreSelectedIndex_);
    }
    return;
  }

  // ─── Unavailable state ───
  if (state_ == State::Unavailable) {
    if (display_->renderFocusTimerScreen) {
      display_->renderFocusTimerScreen("TIMER", "", "", "IMU unavailable", "", -1, false);
    }
    return;
  }

  // ─── Timer session rendering ───
  if (!display_->renderFocusTimerScreen) {
    return;
  }

  const char* modeStr = "";
  const char* instruction = "";
  int progress = -1;
  bool breakAccent = false;

  switch (state_) {
    case State::WaitForTouchStart:
      modeStr = "BEGIN";
      instruction = "Place on short side";
      break;
    case State::TouchRunning:
      modeStr = "BEGIN";
      progress = progressPercent(lastUpdateMs_);
      break;
    case State::WaitAfterTouch:
      modeStr = "WORK";
      instruction = "Flip to continue";
      break;
    case State::WorkRunning:
      modeStr = "WORK";
      progress = progressPercent(lastUpdateMs_);
      break;
    case State::BreakRunning:
      modeStr = "BREAK";
      progress = progressPercent(lastUpdateMs_);
      breakAccent = true;
      break;
    case State::WaitAfterWork:
      modeStr = "BREAK";
      instruction = "Turn for break";
      breakAccent = true;
      break;
    case State::WaitAfterBreak:
      modeStr = "WORK";
      instruction = "Flip to begin";
      break;
    case State::Cancelled:
      modeStr = "BEGIN";
      instruction = "Place to begin again";
      break;
    case State::Complete:
      modeStr = "DONE";
      instruction = "Session complete";
      break;
    default:
      break;
  }

  // Format remaining time if timer is running
  char timerBuf[8] = "";
  if (timerRunning_) {
    uint32_t remaining = remainingMs(lastUpdateMs_);
    uint32_t totalSec = remaining / 1000;
    uint32_t minutes = totalSec / 60;
    uint32_t seconds = totalSec % 60;
    timerBuf[0] = '0' + static_cast<char>(minutes / 10);
    timerBuf[1] = '0' + static_cast<char>(minutes % 10);
    timerBuf[2] = ':';
    timerBuf[3] = '0' + static_cast<char>(seconds / 10);
    timerBuf[4] = '0' + static_cast<char>(seconds % 10);
    timerBuf[5] = '\0';
  }

  display_->renderFocusTimerScreen(modeStr, genreLabel(genre_), timerBuf,
                                   instruction, "", progress, breakAccent);
}

// ─── Public Methods ─────────────────────────────────────────────────────────

void FocusTimerCore::chooseGenre(Genre genre, uint32_t nowMs) {
  if (genre == Genre::None) {
    return;
  }

  clearSession();
  genre_ = genre;
  resetOrientationStability();
  transitionTo(State::WaitForTouchStart, nowMs);
}

void FocusTimerCore::cancelActiveTimer(uint32_t nowMs) {
  if (!timerRunning_) {
    return;
  }

  stopActiveTimer();
  resetOrientationStability();
  feedbackStartedMs_ = nowMs;
  transitionTo(State::Cancelled, nowMs);
}

void FocusTimerCore::abandon() {
  clearSession();
  resetOrientationStability();
  state_ = imuAvailable_ ? State::GenreSelect : State::Unavailable;
  stateStartedMs_ = lastUpdateMs_;
  genreSelectedIndex_ = kGenreFirstIndex;
}

// ─── Accessors ──────────────────────────────────────────────────────────────

FocusTimerCore::State FocusTimerCore::state() const { return state_; }
FocusTimerCore::Genre FocusTimerCore::genre() const { return genre_; }
bool FocusTimerCore::available() const { return imuAvailable_; }
bool FocusTimerCore::isActiveTimerRunning() const { return timerRunning_; }

uint32_t FocusTimerCore::remainingMs(uint32_t nowMs) const {
  if (!timerRunning_) return 0;
  const uint32_t elapsed = nowMs - timerStartedMs_;
  return (elapsed >= timerDurationMs_) ? 0 : (timerDurationMs_ - elapsed);
}

uint8_t FocusTimerCore::progressPercent(uint32_t nowMs) const {
  if (!timerRunning_ || timerDurationMs_ == 0) return 0;
  const uint32_t elapsed = nowMs - timerStartedMs_;
  const uint32_t clamped =
      (elapsed >= timerDurationMs_) ? timerDurationMs_ : elapsed;
  return static_cast<uint8_t>((clamped * 100U) / timerDurationMs_);
}

uint8_t FocusTimerCore::completedTouchBlocks() const { return completedTouchBlocks_; }
uint8_t FocusTimerCore::completedWorkBlocks() const { return completedWorkBlocks_; }
uint8_t FocusTimerCore::completedBreakBlocks() const { return completedBreakBlocks_; }

bool FocusTimerCore::consumeCompletionCue() {
  const bool pending = completionCuePending_;
  completionCuePending_ = false;
  return pending;
}

const char* FocusTimerCore::genreLabel(Genre genre) {
  switch (genre) {
    case Genre::Chores: return "Chores";
    case Genre::RsvpNano: return "Work";
    case Genre::StrengthLabs: return "Fitness";
    case Genre::SelfCare: return "Self Care";
    case Genre::Other: return "Other";
    case Genre::None:
    default: return "";
  }
}

// ─── Genre Menu ─────────────────────────────────────────────────────────────

void FocusTimerCore::moveGenreSelection(int direction) {
  int newIndex = static_cast<int>(genreSelectedIndex_) + direction;
  if (newIndex < 0) {
    newIndex = kGenreMenuItemCount - 1;
  } else if (newIndex >= static_cast<int>(kGenreMenuItemCount)) {
    newIndex = 0;
  }
  genreSelectedIndex_ = static_cast<uint8_t>(newIndex);
}

void FocusTimerCore::selectGenreMenuItem(uint32_t nowMs) {
  if (genreSelectedIndex_ == kGenreBackIndex) {
    // "Back" selected — abandon and return to landscape
    abandon();
    return;
  }

  Genre genre = Genre::None;
  switch (genreSelectedIndex_) {
    case 1: genre = Genre::Chores; break;
    case 2: genre = Genre::RsvpNano; break;
    case 3: genre = Genre::StrengthLabs; break;
    case 4: genre = Genre::SelfCare; break;
    case 5: genre = Genre::Other; break;
    default: break;
  }

  if (genre == Genre::None) {
    return;
  }

  chooseGenre(genre, nowMs);
}

// ─── Orientation Detection ──────────────────────────────────────────────────

void FocusTimerCore::updateOrientation(uint32_t nowMs) {
  if (!imu_ || !imu_->readAccelerometer) {
    rawOrientation_ = OrientationState::Unknown;
    stableOrientation_ = OrientationState::Unknown;
    return;
  }

  float x = 0.0f, y = 0.0f, z = 0.0f;
  if (!imu_->readAccelerometer(&x, &y, &z)) return;

  rawOrientation_ = classify(x, y, z);
  if (rawOrientation_ != candidateOrientation_) {
    candidateOrientation_ = rawOrientation_;
    candidateSinceMs_ = nowMs;
    return;
  }

  if ((nowMs - candidateSinceMs_) >= kOrientationStableMs) {
    stableOrientation_ = candidateOrientation_;
  }
}

void FocusTimerCore::resetOrientationStability() {
  rawOrientation_ = OrientationState::Unknown;
  stableOrientation_ = OrientationState::Unknown;
  candidateOrientation_ = OrientationState::Unknown;
  candidateSinceMs_ = 0;
}

FocusTimerCore::OrientationState FocusTimerCore::classify(float x, float y,
                                                          float z) const {
  float absX = x < 0.0f ? -x : x;
  float absY = y < 0.0f ? -y : y;
  float absZ = z < 0.0f ? -z : z;

  if (absZ >= kFlatAxisThreshold && absX <= 0.30f && absY <= 0.30f)
    return OrientationState::FlatBack;
  if (x >= kSideAxisThreshold && absY <= kCrossAxisLimit && absZ <= kCrossAxisLimit)
    return OrientationState::ShortSideA;
  if (x <= -kSideAxisThreshold && absY <= kCrossAxisLimit && absZ <= kCrossAxisLimit)
    return OrientationState::ShortSideB;
  if (absY >= kSideAxisThreshold && absX <= kCrossAxisLimit && absZ <= kCrossAxisLimit)
    return OrientationState::LongSide;
  return OrientationState::Unknown;
}

bool FocusTimerCore::orientationInputArmed(uint32_t nowMs) const {
  switch (state_) {
    case State::WaitForTouchStart:
      return (nowMs - stateStartedMs_) >= kTouchStartArmDelayMs;
    case State::WaitAfterTouch:
    case State::WaitAfterWork:
    case State::WaitAfterBreak:
      return (nowMs - stateStartedMs_) >= kPostTimerFlipGraceMs;
    default:
      return true;
  }
}

// ─── State Machine ──────────────────────────────────────────────────────────

void FocusTimerCore::transitionTo(State nextState, uint32_t nowMs) {
  state_ = nextState;
  stateStartedMs_ = nowMs;
}

void FocusTimerCore::clearSession() {
  genre_ = Genre::None;
  activeMode_ = TimerMode::None;
  activeStartOrientation_ = OrientationState::Unknown;
  lastShortSide_ = OrientationState::Unknown;
  timerStartedMs_ = 0;
  timerDurationMs_ = 0;
  timerRunning_ = false;
  feedbackStartedMs_ = 0;
  completionCuePending_ = false;
  completedTouchBlocks_ = 0;
  completedWorkBlocks_ = 0;
  completedBreakBlocks_ = 0;
}

void FocusTimerCore::startMode(TimerMode mode, uint32_t nowMs,
                               uint32_t durationMs,
                               OrientationState startOrientation) {
  activeMode_ = mode;
  activeStartOrientation_ = startOrientation;
  timerStartedMs_ = nowMs;
  timerDurationMs_ = durationMs;
  timerRunning_ = true;
  if (isShortSide(startOrientation)) lastShortSide_ = startOrientation;
}

void FocusTimerCore::stopActiveTimer() {
  timerRunning_ = false;
  activeMode_ = TimerMode::None;
  activeStartOrientation_ = OrientationState::Unknown;
  timerStartedMs_ = 0;
  timerDurationMs_ = 0;
  lastShortSide_ = OrientationState::Unknown;
}

void FocusTimerCore::completeActiveTimer() {
  if (!timerRunning_) return;
  switch (activeMode_) {
    case TimerMode::Touch: ++completedTouchBlocks_; break;
    case TimerMode::Work: ++completedWorkBlocks_; break;
    case TimerMode::Break: ++completedBreakBlocks_; break;
    default: break;
  }
  timerRunning_ = false;
  activeMode_ = TimerMode::None;
  activeStartOrientation_ = OrientationState::Unknown;
  timerStartedMs_ = 0;
  timerDurationMs_ = 0;
  completionCuePending_ = true;
}

bool FocusTimerCore::timerExpired(uint32_t nowMs) const {
  return timerRunning_ && (nowMs - timerStartedMs_ >= timerDurationMs_);
}

// ─── Helpers ────────────────────────────────────────────────────────────────

bool FocusTimerCore::isShortSide(OrientationState orientation) {
  return orientation == OrientationState::ShortSideA ||
         orientation == OrientationState::ShortSideB;
}

FocusTimerCore::OrientationState FocusTimerCore::oppositeShortSide(
    OrientationState orientation) {
  switch (orientation) {
    case OrientationState::ShortSideA: return OrientationState::ShortSideB;
    case OrientationState::ShortSideB: return OrientationState::ShortSideA;
    default: return OrientationState::Unknown;
  }
}

PluginOrientation FocusTimerCore::portraitOrientationForShortSide(
    OrientationState orientation) const {
  // ShortSideB maps to PortraitFlipped in original code (which used
  // BoardConfig::UiOrientation::PortraitFlipped). The bridge remaps
  // PLUGIN_ORIENTATION_PORTRAIT_A → PortraitFlipped and
  // PLUGIN_ORIENTATION_PORTRAIT_B → Portrait.
  // Original: ShortSideB → PortraitFlipped → bridge expects PORTRAIT_A
  //           ShortSideA → Portrait        → bridge expects PORTRAIT_B
  return orientation == OrientationState::ShortSideB
             ? PLUGIN_ORIENTATION_PORTRAIT_A
             : PLUGIN_ORIENTATION_PORTRAIT_B;
}

void FocusTimerCore::updateUiOrientation() {
  if (!orientation_ || !orientation_->setUiOrientation) return;

  PluginOrientation uiOri = PLUGIN_ORIENTATION_LANDSCAPE;

  switch (state_) {
    case State::GenreSelect:
    case State::Unavailable:
    case State::Complete:
      uiOri = PLUGIN_ORIENTATION_LANDSCAPE;
      break;
    case State::WaitForTouchStart:
    case State::TouchRunning:
    case State::Cancelled:
      uiOri = portraitOrientationForShortSide(activeStartOrientation_);
      break;
    case State::WaitAfterTouch:
    case State::WorkRunning:
    case State::WaitAfterBreak:
      uiOri = portraitOrientationForShortSide(lastShortSide_);
      break;
    case State::BreakRunning:
    case State::WaitAfterWork:
      uiOri = PLUGIN_ORIENTATION_LANDSCAPE;
      break;
  }

  orientation_->setUiOrientation(uiOri);
}

// ─── Plugin SDK VTable Glue ─────────────────────────────────────────────────

static PluginResult focusTimerInit(PluginContext* ctx) {
    s_instance = new FocusTimerCore(
        ctx->imu, ctx->audio, ctx->display, ctx->orientation);
    if (!s_instance) return PLUGIN_ERROR_MEMORY;
    if (!s_instance->begin()) {
        delete s_instance;
        s_instance = nullptr;
        return PLUGIN_ERROR_INIT;
    }
    s_instance->open();
    return PLUGIN_OK;
}

static void focusTimerDestroy() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

static void focusTimerUpdate(uint32_t nowMs) {
    if (s_instance) s_instance->update(nowMs);
}

static void focusTimerHandleButton(const PluginButtonEvent* event) {
    if (s_instance) s_instance->handleButton(event);
}

static void focusTimerHandleTouch(const PluginTouchEvent* event) {
    if (s_instance) s_instance->handleTouch(event);
}

static void focusTimerDraw() {
    if (s_instance) s_instance->draw();
}

static PluginInfo focusTimerGetInfo() {
    return {"Focus Timer", "1.0.0", PLUGIN_SDK_VERSION};
}

PluginVTable FocusTimerPlugin::vtable() {
    return {
        focusTimerInit,
        focusTimerDestroy,
        focusTimerUpdate,
        focusTimerHandleButton,
        focusTimerHandleTouch,
        focusTimerDraw,
        focusTimerGetInfo,
    };
}
