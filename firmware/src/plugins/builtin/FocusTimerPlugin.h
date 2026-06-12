// firmware/src/plugins/builtin/FocusTimerPlugin.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "plugins/sdk/PluginSdk.h"
#include "plugins/sdk/PluginImuService.h"
#include "plugins/sdk/PluginAudioService.h"
#include "plugins/sdk/PluginDisplayService.h"
#include "plugins/sdk/PluginOrientationService.h"

class FocusTimerCore {
 public:
  enum class Genre : uint8_t {
    Chores = 0,
    RsvpNano,
    StrengthLabs,
    SelfCare,
    Other,
    None = 0xFF,
  };

  enum class State : uint8_t {
    Unavailable = 0,
    GenreSelect,
    WaitForTouchStart,
    TouchRunning,
    WaitAfterTouch,
    WorkRunning,
    BreakRunning,
    WaitAfterWork,
    WaitAfterBreak,
    Cancelled,
    Complete,
  };

  FocusTimerCore(PluginImuService* imu, PluginAudioService* audio,
                 PluginDisplayService* display, PluginOrientationService* orientation);

  bool begin();
  void open();
  void update(uint32_t nowMs);
  void handleButton(const PluginButtonEvent* event);
  void handleTouch(const PluginTouchEvent* event);
  void draw();

  // Accessors
  State state() const;
  Genre genre() const;
  bool available() const;
  bool isActiveTimerRunning() const;
  uint32_t remainingMs(uint32_t nowMs) const;
  uint8_t progressPercent(uint32_t nowMs) const;
  uint8_t completedTouchBlocks() const;
  uint8_t completedWorkBlocks() const;
  uint8_t completedBreakBlocks() const;
  bool consumeCompletionCue();
  void chooseGenre(Genre genre, uint32_t nowMs);
  void cancelActiveTimer(uint32_t nowMs);
  void abandon();

  static const char* genreLabel(Genre genre);

 private:
  enum class TimerMode : uint8_t {
    None = 0,
    Touch,
    Work,
    Break,
  };

  enum class OrientationState : uint8_t {
    ShortSideA = 0,
    ShortSideB,
    LongSide,
    FlatBack,
    Unknown,
  };

  // Orientation detection
  void updateOrientation(uint32_t nowMs);
  void resetOrientationStability();
  OrientationState classify(float x, float y, float z) const;
  bool orientationInputArmed(uint32_t nowMs) const;

  // State machine
  void transitionTo(State nextState, uint32_t nowMs);
  void clearSession();
  void startMode(TimerMode mode, uint32_t nowMs, uint32_t durationMs,
                 OrientationState startOrientation);
  void stopActiveTimer();
  void completeActiveTimer();
  bool timerExpired(uint32_t nowMs) const;

  // Helpers
  static bool isShortSide(OrientationState orientation);
  static OrientationState oppositeShortSide(OrientationState orientation);
  PluginOrientation portraitOrientationForShortSide(OrientationState orientation) const;
  void updateUiOrientation();

  // Genre menu
  void moveGenreSelection(int direction);
  void selectGenreMenuItem(uint32_t nowMs);

  // Device services (not owned)
  PluginImuService* imu_;
  PluginAudioService* audio_;
  PluginDisplayService* display_;
  PluginOrientationService* orientation_;

  // IMU state
  bool imuAvailable_ = false;
  OrientationState rawOrientation_ = OrientationState::Unknown;
  OrientationState stableOrientation_ = OrientationState::Unknown;
  OrientationState candidateOrientation_ = OrientationState::Unknown;
  uint32_t candidateSinceMs_ = 0;

  // Timer state machine
  State state_ = State::Unavailable;
  Genre genre_ = Genre::None;
  TimerMode activeMode_ = TimerMode::None;
  OrientationState activeStartOrientation_ = OrientationState::Unknown;
  OrientationState lastShortSide_ = OrientationState::Unknown;
  uint32_t stateStartedMs_ = 0;
  uint32_t feedbackStartedMs_ = 0;
  uint32_t timerStartedMs_ = 0;
  uint32_t timerDurationMs_ = 0;
  bool timerRunning_ = false;
  bool completionCuePending_ = false;
  uint8_t completedTouchBlocks_ = 0;
  uint8_t completedWorkBlocks_ = 0;
  uint8_t completedBreakBlocks_ = 0;

  // Last update timestamp for draw
  uint32_t lastUpdateMs_ = 0;

  // Genre selection menu
  uint8_t genreSelectedIndex_ = 1;  // 0=Back, 1..5=genres

  // Touch hold for cancel
  bool touchActive_ = false;
  uint16_t touchStartX_ = 0;
  uint16_t touchStartY_ = 0;
  uint32_t touchStartMs_ = 0;
  bool cancelHoldTriggered_ = false;
};

/// Plugin SDK vtable entry points for the FocusTimer built-in plugin.
namespace FocusTimerPlugin {
    PluginVTable vtable();
}
