// Host implementations of the hardware hooks DisplayManager touches.
#include <Arduino.h>
#include <SD_MMC.h>
#include <chrono>

#include "display/SdFontLoader.h"
#include "display/axs15231b.h"

SerialStub Serial;
SdMmcStub SD_MMC;

static uint32_t gFakeMillis = 100000;
uint32_t millis() { return gFakeMillis; }
uint32_t micros() { return gFakeMillis * 1000; }
void delay(uint32_t ms) { gFakeMillis += ms; }

void axs15231bInit() {}
void axs15231bSetBacklight(bool) {}
void axs15231bSetBrightnessPercent(uint8_t) {}
void axs15231bSleep() {}
void axs15231bWake() {}
void axs15231bPushColors(uint16_t, uint16_t, uint16_t, uint16_t, const uint16_t *) {}

SdFontLoader::~SdFontLoader() {}
bool SdFontLoader::load(const String &) {
  status_ = Status::FileNotFound;
  return false;
}
void SdFontLoader::unload() { status_ = Status::NotLoaded; }
