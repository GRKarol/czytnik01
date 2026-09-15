#pragma once

#include <Arduino.h>

#include "display/EmbeddedFontCommon.h"

// Reads one binary .fnt file (docs/FONT_FNT_FORMAT.md) from the SD card into
// a single PSRAM buffer and exposes it as an EmbeddedFontVariant pointing
// straight into that buffer, mirroring what the PROGMEM tables used to look
// like — DisplayManager's glyph lookups don't need to know the difference.
class SdFontLoader {
 public:
  enum class Status : uint8_t {
    NotLoaded = 0,
    Loaded,
    FileNotFound,
    InvalidFormat,
    OutOfMemory,
  };

  ~SdFontLoader();

  // Loads `path` into a fresh buffer, replacing whatever this instance held
  // before (the old buffer is freed either way). Returns true on success.
  bool load(const String &path);

  void unload();

  bool isLoaded() const { return status_ == Status::Loaded; }
  Status status() const { return status_; }

  // Only valid while isLoaded() is true.
  const EmbeddedFontVariant &variant() const { return variant_; }

 private:
  Status status_ = Status::NotLoaded;
  uint8_t *buffer_ = nullptr;
  EmbeddedFontVariant variant_ = {};
};
