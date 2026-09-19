#include "input/ButtonHandler.h"

namespace {
// 25ms comfortably clears mechanical-switch bounce (typically <10ms) without
// making a deliberate tap feel laggy.
constexpr uint32_t kDebounceMs = 25;
}  // namespace

ButtonHandler::ButtonHandler(int pin) : pin_(pin) {}

void ButtonHandler::begin() {
  pinMode(pin_, INPUT_PULLUP);
  held_ = !digitalRead(pin_);
  rawHeld_ = held_;
  pressedEvent_ = false;
  releasedEvent_ = false;
  lastEdgeMs_ = millis();
  rawChangedMs_ = lastEdgeMs_;
  pressStartedMs_ = held_ ? lastEdgeMs_ : 0;
  lastHoldDurationMs_ = 0;
}

void ButtonHandler::update(uint32_t nowMs) {
  pressedEvent_ = false;
  releasedEvent_ = false;

  const bool currentRead = !digitalRead(pin_);  // Board buttons are active-low.
  if (currentRead != rawHeld_) {
    rawHeld_ = currentRead;
    rawChangedMs_ = nowMs;
  }

  // Only commit the raw reading as the debounced state once it has held
  // steady for kDebounceMs — a mid-bounce read is ignored, not promoted.
  if (rawHeld_ != held_ && (nowMs - rawChangedMs_) >= kDebounceMs) {
    held_ = rawHeld_;
    lastEdgeMs_ = nowMs;
    if (held_) {
      pressStartedMs_ = nowMs;
      pressedEvent_ = true;
    } else {
      lastHoldDurationMs_ = nowMs - pressStartedMs_;
      releasedEvent_ = true;
    }
  }
}

bool ButtonHandler::isHeld() const { return held_; }

bool ButtonHandler::wasPressedEvent() const { return pressedEvent_; }

bool ButtonHandler::wasReleasedEvent() const { return releasedEvent_; }

uint32_t ButtonHandler::lastEdgeMs() const { return lastEdgeMs_; }

uint32_t ButtonHandler::heldDurationMs(uint32_t nowMs) const {
  return held_ ? nowMs - pressStartedMs_ : 0;
}

uint32_t ButtonHandler::lastHoldDurationMs() const { return lastHoldDurationMs_; }
