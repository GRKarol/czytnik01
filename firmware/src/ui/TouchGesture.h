#pragma once

// Shared tap/swipe classification for the menu (button-grid) touch surface.
//
// The RSVP reader keeps its own bespoke multi-intent gesture recognizer
// (hold-to-read, scrub, browse-scroll, WPM swipe all share one continuous
// touch session in App::applyPausedTouchGesture) — that logic is proven on
// hardware and stays untouched. This header only centralizes the
// tap-vs-swipe tolerance used by the new grid-menu dispatch, so the numbers
// live in one named place instead of being re-derived ad hoc.

#include <Arduino.h>
#include <cstdlib>

namespace ui {

struct TouchConfig {
  uint16_t tapMoveTolerancePx = 26;
  uint32_t tapMaxDurationMs = 600;
  uint32_t holdMs = 600;
  uint16_t swipeThresholdPx = 40;
  uint16_t swipeAxisBiasPx = 12;
};

enum class Gesture {
  None,
  Tap,
  Hold,
  SwipeLeft,
  SwipeRight,
  SwipeUp,
  SwipeDown,
};

// Pure classifier: given the total displacement and duration of a finished
// (or long-held) touch, decide what it was. Called once at touch-end (or
// once a hold threshold is crossed) — the same Rects used to draw the grid
// are then hit-tested only when this returns Gesture::Tap.
inline Gesture classify(int deltaX, int deltaY, uint32_t durationMs, bool stillDown,
                        const TouchConfig &cfg = TouchConfig()) {
  const int absDeltaX = std::abs(deltaX);
  const int absDeltaY = std::abs(deltaY);
  const bool withinTapTolerance =
      absDeltaX <= static_cast<int>(cfg.tapMoveTolerancePx) &&
      absDeltaY <= static_cast<int>(cfg.tapMoveTolerancePx);

  if (stillDown) {
    if (withinTapTolerance && durationMs >= cfg.holdMs) {
      return Gesture::Hold;
    }
    return Gesture::None;
  }

  if (withinTapTolerance && durationMs <= cfg.tapMaxDurationMs) {
    return Gesture::Tap;
  }

  if (absDeltaX >= static_cast<int>(cfg.swipeThresholdPx) &&
      absDeltaX > absDeltaY + static_cast<int>(cfg.swipeAxisBiasPx)) {
    return deltaX < 0 ? Gesture::SwipeLeft : Gesture::SwipeRight;
  }

  if (absDeltaY >= static_cast<int>(cfg.swipeThresholdPx) &&
      absDeltaY > absDeltaX + static_cast<int>(cfg.swipeAxisBiasPx)) {
    return deltaY < 0 ? Gesture::SwipeUp : Gesture::SwipeDown;
  }

  return withinTapTolerance ? Gesture::Tap : Gesture::None;
}

}  // namespace ui
