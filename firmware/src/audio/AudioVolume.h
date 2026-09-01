#pragma once

#include <stdint.h>

// Persisted playback volume (0-100), shared by AudioManager (beep) and
// AudioRecorder (WAV playback) — the only two DAC-output paths in the
// firmware. Lazily loaded from NVS on first read, cached in RAM after.
namespace AudioVolume {

uint8_t percent();
void setPercent(uint8_t percent);

// Linear mapping of percent() onto the ES8311 DAC volume register range.
uint8_t dacRegisterValue();

}  // namespace AudioVolume
