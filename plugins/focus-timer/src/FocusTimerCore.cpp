// plugins/focus-timer/src/FocusTimerCore.cpp
#include "FocusTimerCore.h"

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

// Genre count for menu navigation
constexpr uint8_t kGenreCount = 5;

}  // namespace

// ─── Constructor ────────────────────────────────────────────────────────────

FocusTimerCore::FocusTimerCore(PluginImuService* imu, PluginAudioService* audio,
                               PluginDisplayService* display,
                               PluginOrientationService* orientation)
    : imu_(imu), audio_(audio), display_(display), orientation_(orientation) {}

// ─── Lifecycle ──────────────────────────────────────────────────────────────

bool FocusTimerCore::begin() {
  imuAvailable_ = imu_ && imu_->available && imu_->available();

  clearSession();
  resetOrientationStability();
  state_ = imuAvailable_ ? State::GenreSelect : State::Unavailable;
  stateStartedMs_ = 0;

  return true;
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
        startMode(TimerMode::Break, nowMs, kBreakDurationMs,
                  OrientationState::LongSide);
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
        startMode(TimerMode::Break, nowMs, kBreakDurationMs,
                  OrientationState::LongSide);
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
  if (!event || !event->pressed) {
    return;
  }

  // Boot button (id 0) cancels a running timer
  if (event->buttonId == 0 && timerRunning_) {
    stopActiveTimer();
    resetOrientationStability();
    feedbackStartedMs_ = event->timestampMs;
    transitionTo(State::Cancelled, event->timestampMs);
  }
}

void FocusTimerCore::handleTouch(const PluginTouchEvent* event) {
  if (!event) {
    return;
  }

  // Touch input only used during genre selection (phase 2 = end/tap)
  if (state_ != State::GenreSelect || event->phase != 2) {
    return;
  }

  // Simple tap-based genre selection using screen regions
  // Divide screen vertically into genre slots
  int height = display_ && display_->logicalHeight ? display_->logicalHeight() : 300;
  int slotHeight = height / kGenreCount;
  uint8_t selectedSlot = static_cast<uint8_t>(event->y / slotHeight);
  if (selectedSlot >= kGenreCount) {
    selectedSlot = kGenreCount - 1;
  }

  Genre selectedGenre = static_cast<Genre>(selectedSlot);
  // Choose genre and transition
  genre_ = selectedGenre;
  resetOrientationStability();
  transitionTo(State::WaitForTouchStart, event->timestampMs);
}

void FocusTimerCore::draw() {
  if (!display_ || !display_->renderFocusTimerScreen) {
    return;
  }

  if (state_ == State::GenreSelect) {
    // Render genre menu
    static const char* genreItems[] = {"Chores", "Work", "Fitness", "Self Care",
                                       "Other"};
    if (display_->renderMenu) {
      display_->renderMenu(genreItems, kGenreCount, genreSelectIndex_);
    }
    return;
  }

  if (state_ == State::Unavailable) {
    if (display_->renderStatus) {
      display_->renderStatus("Klepsydra", "IMU unavailable", "");
    }
    return;
  }

  // Build render parameters based on current state
  const char* modeStr = "";
  const char* instruction = "";
  const char* footer = "";
  bool breakAccent = false;
  int progress = 0;

  switch (activeMode_) {
    case TimerMode::Touch:
      modeStr = "Touch";
      break;
    case TimerMode::Work:
      modeStr = "Work";
      break;
    case TimerMode::Break:
      modeStr = "Break";
      breakAccent = true;
      break;
    case TimerMode::None:
    default:
      modeStr = "";
      break;
  }

  switch (state_) {
    case State::WaitForTouchStart:
      instruction = "Flip to short side";
      break;
    case State::WaitAfterTouch:
      instruction = "Flip to work or break";
      break;
    case State::WaitAfterWork:
      instruction = "Flip to work or break";
      break;
    case State::WaitAfterBreak:
      instruction = "Flip to short side";
      break;
    case State::Cancelled:
      instruction = "Cancelled";
      break;
    case State::Complete:
      instruction = "Complete!";
      break;
    default:
      break;
  }

  // Format timer string
  char timerBuf[16] = "";
  if (timerRunning_) {
    uint32_t remaining = remainingMs(lastUpdateMs_);
    uint32_t totalSec = remaining / 1000;
    uint32_t minutes = totalSec / 60;
    uint32_t seconds = totalSec % 60;
    // Simple integer-to-string without snprintf (plugin has no stdlib)
    timerBuf[0] = '0' + static_cast<char>(minutes / 10);
    timerBuf[1] = '0' + static_cast<char>(minutes % 10);
    timerBuf[2] = ':';
    timerBuf[3] = '0' + static_cast<char>(seconds / 10);
    timerBuf[4] = '0' + static_cast<char>(seconds % 10);
    timerBuf[5] = '\0';
    progress = progressPercent(lastUpdateMs_);
  }

  // Build footer with block counts
  char footerBuf[32] = "";
  {
    int pos = 0;
    footerBuf[pos++] = 'T';
    footerBuf[pos++] = ':';
    footerBuf[pos++] = '0' + completedTouchBlocks_;
    footerBuf[pos++] = ' ';
    footerBuf[pos++] = 'W';
    footerBuf[pos++] = ':';
    footerBuf[pos++] = '0' + completedWorkBlocks_;
    footerBuf[pos++] = ' ';
    footerBuf[pos++] = 'B';
    footerBuf[pos++] = ':';
    footerBuf[pos++] = '0' + completedBreakBlocks_;
    footerBuf[pos] = '\0';
    footer = footerBuf;
  }

  display_->renderFocusTimerScreen(modeStr, genreLabel(genre_), timerBuf,
                                   instruction, footer, progress, breakAccent);
}

// ─── Accessors ──────────────────────────────────────────────────────────────

FocusTimerCore::State FocusTimerCore::state() const { return state_; }

FocusTimerCore::Genre FocusTimerCore::genre() const { return genre_; }

bool FocusTimerCore::isActiveTimerRunning() const { return timerRunning_; }

uint32_t FocusTimerCore::remainingMs(uint32_t nowMs) const {
  if (!timerRunning_) {
    return 0;
  }
  const uint32_t elapsed = nowMs - timerStartedMs_;
  return (elapsed >= timerDurationMs_) ? 0 : (timerDurationMs_ - elapsed);
}

uint8_t FocusTimerCore::progressPercent(uint32_t nowMs) const {
  if (!timerRunning_ || timerDurationMs_ == 0) {
    return 0;
  }
  const uint32_t elapsed = nowMs - timerStartedMs_;
  const uint32_t clamped =
      (elapsed >= timerDurationMs_) ? timerDurationMs_ : elapsed;
  return static_cast<uint8_t>((clamped * 100U) / timerDurationMs_);
}

uint8_t FocusTimerCore::completedTouchBlocks() const {
  return completedTouchBlocks_;
}

uint8_t FocusTimerCore::completedWorkBlocks() const {
  return completedWorkBlocks_;
}

uint8_t FocusTimerCore::completedBreakBlocks() const {
  return completedBreakBlocks_;
}

bool FocusTimerCore::consumeCompletionCue() {
  const bool pending = completionCuePending_;
  completionCuePending_ = false;
  return pending;
}

const char* FocusTimerCore::genreLabel(Genre genre) {
  switch (genre) {
    case Genre::Chores:
      return "Chores";
    case Genre::RsvpNano:
      return "Work";
    case Genre::StrengthLabs:
      return "Fitness";
    case Genre::SelfCare:
      return "Self Care";
    case Genre::Other:
      return "Other";
    case Genre::None:
    default:
      return "";
  }
}

// ─── Orientation Detection ──────────────────────────────────────────────────

void FocusTimerCore::updateOrientation(uint32_t nowMs) {
  if (!imu_ || !imu_->readAccelerometer) {
    rawOrientation_ = OrientationState::Unknown;
    stableOrientation_ = OrientationState::Unknown;
    return;
  }

  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  if (!imu_->readAccelerometer(&x, &y, &z)) {
    return;
  }

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
  // Absolute value without math.h dependency
  float absX = x < 0.0f ? -x : x;
  float absY = y < 0.0f ? -y : y;
  float absZ = z < 0.0f ? -z : z;

  if (absZ >= kFlatAxisThreshold && absX <= 0.30f && absY <= 0.30f) {
    return OrientationState::FlatBack;
  }

  if (x >= kSideAxisThreshold && absY <= kCrossAxisLimit &&
      absZ <= kCrossAxisLimit) {
    return OrientationState::ShortSideA;
  }

  if (x <= -kSideAxisThreshold && absY <= kCrossAxisLimit &&
      absZ <= kCrossAxisLimit) {
    return OrientationState::ShortSideB;
  }

  if (absY >= kSideAxisThreshold && absX <= kCrossAxisLimit &&
      absZ <= kCrossAxisLimit) {
    return OrientationState::LongSide;
  }

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

  if (isShortSide(startOrientation)) {
    lastShortSide_ = startOrientation;
  }
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
  if (!timerRunning_) {
    return;
  }

  switch (activeMode_) {
    case TimerMode::Touch:
      ++completedTouchBlocks_;
      break;
    case TimerMode::Work:
      ++completedWorkBlocks_;
      break;
    case TimerMode::Break:
      ++completedBreakBlocks_;
      break;
    case TimerMode::None:
    default:
      break;
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
    case OrientationState::ShortSideA:
      return OrientationState::ShortSideB;
    case OrientationState::ShortSideB:
      return OrientationState::ShortSideA;
    default:
      return OrientationState::Unknown;
  }
}

PluginOrientation FocusTimerCore::portraitOrientationForShortSide(
    OrientationState orientation) const {
  return orientation == OrientationState::ShortSideB
             ? PLUGIN_ORIENTATION_PORTRAIT_B
             : PLUGIN_ORIENTATION_PORTRAIT_A;
}

void FocusTimerCore::updateUiOrientation() {
  if (!orientation_ || !orientation_->setUiOrientation) {
    return;
  }

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
