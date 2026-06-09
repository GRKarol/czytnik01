#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

class PresetManager {
 public:
  static constexpr size_t kMaxPresets = 10;
  static constexpr size_t kMaxPresetNameLength = 20;
  static constexpr const char* kPresetsDir = "/config/presets";

  struct PresetInfo {
    String name;      // Display name from JSON "name" field
    String filename;  // Filename on SD card (e.g., "1717000000_a3f2.json")
  };

  enum class SaveResult : uint8_t {
    Ok,
    LimitReached,
    SdCardError,
    DirectoryError,
    WriteError,
  };

  enum class RestoreResult : uint8_t {
    Ok,
    FileNotFound,
    ParseError,
    SdCardError,
  };

  enum class DeleteResult : uint8_t {
    Ok,
    FileNotFound,
    SdCardError,
  };

  /// List all valid presets from SD card, sorted alphabetically by name.
  /// Skips non-.json files and files without a valid "name" field.
  std::vector<PresetInfo> listPresets();

  /// Save current NVS settings as a new preset with the given name.
  /// Generates a unique filename. Returns result code.
  SaveResult savePreset(const String& name, Preferences& prefs);

  /// Restore settings from a preset file into NVS.
  /// Skips unknown keys, clamps out-of-range values.
  /// Returns result code.
  RestoreResult restorePreset(const String& filename, Preferences& prefs);

  /// Delete a preset file from SD card.
  DeleteResult deletePreset(const String& filename);

  /// Validate and sanitize a preset name.
  /// Returns sanitized name (filtered chars, truncated to 20).
  /// Empty result means invalid (empty after sanitization).
  String validateName(const String& raw);

  /// Returns current preset count on SD card.
  size_t presetCount();

 private:
  /// Generate unique filename using millis() + random suffix.
  String generateFilename();

  /// Serialize all captured settings from NVS into JSON string.
  String serializePreset(const String& name, Preferences& prefs);

  /// Parse a preset JSON file and write settings into NVS.
  /// Returns false if JSON is malformed.
  bool deserializeAndApply(const String& json, Preferences& prefs);

  /// Read JSON "name" field from file content. Returns empty on failure.
  String extractPresetName(const String& json);

  /// Clamp a numeric value to [min, max].
  int clampValue(int value, int min, int max);

  /// Escape a string for safe JSON embedding.
  String jsonEscape(const String& value);
};
