#include "audio/AudioVolume.h"

#include <Preferences.h>

namespace {

constexpr char kNamespace[] = "audio";
constexpr char kKeyVolume[] = "vol";
// Matches the hardcoded 0xFF (max) DAC volume every playback path used
// before this file existed — keeps default behavior unchanged.
constexpr uint8_t kDefaultPercent = 100;

int16_t cachedPercent = -1;

}  // namespace

namespace AudioVolume {

uint8_t percent() {
  if (cachedPercent < 0) {
    Preferences prefs;
    prefs.begin(kNamespace, true);
    cachedPercent = prefs.getUChar(kKeyVolume, kDefaultPercent);
    prefs.end();
  }
  return static_cast<uint8_t>(cachedPercent);
}

void setPercent(uint8_t value) {
  if (value > 100) value = 100;
  cachedPercent = value;

  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.putUChar(kKeyVolume, value);
  prefs.end();
}

uint8_t dacRegisterValue() {
  const uint16_t pct = percent();
  return static_cast<uint8_t>((pct * 255U) / 100U);
}

}  // namespace AudioVolume
