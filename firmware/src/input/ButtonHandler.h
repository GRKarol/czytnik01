#pragma once

#include <Arduino.h>

class ButtonHandler {
 public:
  explicit ButtonHandler(int pin);

  void begin();
  void update(uint32_t nowMs);

  bool isHeld() const;
  bool wasPressedEvent() const;
  bool wasReleasedEvent() const;
  uint32_t lastEdgeMs() const;
  uint32_t heldDurationMs(uint32_t nowMs) const;
  uint32_t lastHoldDurationMs() const;

 private:
  int pin_;
  bool held_ = false;
  bool pressedEvent_ = false;
  bool releasedEvent_ = false;
  uint32_t lastEdgeMs_ = 0;
  uint32_t pressStartedMs_ = 0;
  uint32_t lastHoldDurationMs_ = 0;
  // Debounce: raw pin reads flip back and forth for a few ms on every
  // mechanical press/release. Without filtering, that bounce commits as a
  // real release+press pair mid-hold, which resets pressStartedMs_ and
  // stops heldDurationMs() from ever reaching a long-press threshold — the
  // theme-toggle long-press bug reported 2026-09-19. A candidate reading
  // must stay stable for kDebounceMs before it's accepted as the new state.
  bool rawHeld_ = false;
  uint32_t rawChangedMs_ = 0;
};
