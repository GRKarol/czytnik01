# Design Document: Settings Presets

## Overview

This feature adds a **PresetManager** module to the firmware that enables users to save, list, restore, and delete named snapshots of their reading/display settings. Presets are serialized as JSON files on the SD card at `/config/presets/`, using the same manual JSON parsing approach already established in `CompanionSyncManager`. The module integrates with the existing `App` class menu system, NVS preferences under namespace `"rsvp"`, and `reloadRuntimePreferences()` for live application of restored settings.

## Architecture

### High-Level Architecture

The PresetManager is structured as a self-contained logic component within `firmware/src/storage/`, following the same organizational pattern as existing storage-related modules (`StorageManager`, `IndexedBookStore`). The `App` class owns the instance and delegates all preset operations to it.

### Architecture Diagram

```
┌──────────────────────────────────────────────────────┐
│                     App (UI Layer)                    │
│  ┌────────────────┐  ┌──────────────────────────┐   │
│  │ SettingsHome   │  │  Presets MenuScreen       │   │
│  │ (index 4:      │──│  • "Save Current"        │   │
│  │  Presets entry) │  │  • Preset list (by name) │   │
│  └────────────────┘  │  • Delete confirmation    │   │
│                       └────────────┬─────────────┘   │
│                                    │                  │
│  ┌─────────────────┐              │                  │
│  │ TextEntry       │◄─────────────┘ (name input)     │
│  │ (PresetName)    │                                  │
│  └─────────────────┘                                  │
└───────────────────────────────┬──────────────────────┘
                                │
                                ▼
┌──────────────────────────────────────────────────────┐
│               PresetManager (Logic Layer)             │
│  • savePreset(name) → captures NVS → writes JSON     │
│  • listPresets() → reads /config/presets/*.json       │
│  • restorePreset(filename) → reads JSON → writes NVS │
│  • deletePreset(filename) → removes file             │
│  • validateName(input) → validates/sanitizes          │
└────────────┬────────────────────────┬────────────────┘
             │                        │
             ▼                        ▼
┌────────────────────┐   ┌────────────────────────────┐
│   Preferences      │   │   SD Card (via SD_MMC)     │
│   (NVS "rsvp")     │   │   /config/presets/*.json   │
└────────────────────┘   └────────────────────────────┘
```

### Data Flow

**Save flow:**

1. User selects "Save Current" → TextEntry opens (purpose: `PresetName`)
2. User confirms name → `PresetManager::savePreset(name, preferences)` called
3. PresetManager reads all captured keys from NVS `preferences_`
4. Serializes to JSON `{ "name": "...", "settings": { ... } }`
5. Generates unique filename (timestamp-based), writes to `/config/presets/`
6. Returns success/failure → UI shows feedback, refreshes menu

**Restore flow:**

1. User selects a preset entry → `PresetManager::restorePreset(filename, preferences)` called
2. PresetManager reads file from SD, parses JSON
3. For each key in `"settings"`: validates, clamps, writes to NVS
4. Unknown keys are skipped; missing keys leave NVS unchanged
5. `reloadRuntimePreferences(nowMs, true)` applies changes live
6. UI shows confirmation message

**Delete flow:**

1. User long-presses or selects delete on a preset → confirmation prompt shown
2. On confirm → `PresetManager::deletePreset(filename)` removes file from SD
3. Menu refreshes; if count drops below 10, "Save Current" re-enabled

**List flow:**

1. When `MenuScreen::Presets` opens → `PresetManager::listPresets()` called
2. Scans `/config/presets/` for `*.json` files
3. Opens each, reads `"name"` field; skips files without valid `"name"`
4. Returns vector of `{name, filename}` sorted alphabetically by name
5. UI builds menu items: Back, Save Current (if < 10), preset names

---

## Components and Interfaces

### New Enum Values

```cpp
// In App.h, MenuScreen enum — add after existing entries:
enum class MenuScreen {
  // ... existing values ...
  Presets,              // NEW: Settings presets list
  PresetsDeleteConfirm, // NEW: Delete confirmation
};
```

### New TextEntryPurpose

```cpp
enum class TextEntryPurpose : uint8_t {
  // ... existing ...
  PresetName,  // NEW
};
```

### PresetManager Class Interface

```cpp
// firmware/src/storage/PresetManager.h
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
```

### App Integration — New Members

```cpp
// In App.h private section:
PresetManager presetManager_;
std::vector<String> presetFilenames_;  // Maps menu index → filename on SD
size_t presetsDeleteTargetIndex_ = 0;
```

### App Integration — New Methods

```cpp
// In App.h private section:
void openPresets();
void selectPresetsItem(uint32_t nowMs);
void confirmDeletePreset(size_t index, uint32_t nowMs);
void executeDeletePreset(uint32_t nowMs);
void executeSavePreset(uint32_t nowMs);
void executeRestorePreset(size_t index, uint32_t nowMs);
```

### Menu Integration Constants

```cpp
// Settings Home — insert Presets at position 4, shift About to 5
constexpr size_t kSettingsHomePresetsIndex = 4;      // NEW
constexpr size_t kSettingsHomeAboutIndex = 5;        // Shifted from 4 → 5
constexpr size_t kSettingsHomeTypographyIndex = 6;   // Shifted (dev)
constexpr size_t kSettingsHomeWifiIndex = 7;         // Shifted (dev)
constexpr size_t kSettingsHomeUpdateIndex = 8;       // Shifted (dev)

// Presets menu item indices
constexpr size_t kPresetsBackIndex = 0;
constexpr size_t kPresetsSaveCurrentIndex = 1;
constexpr size_t kPresetsFirstPresetIndex = 2;
```

---

## Data Models

### Captured Settings Key Table

The keys included in every preset snapshot, defined as a compile-time array for forward-extensibility:

```cpp
// In PresetManager.cpp
struct CapturedKey {
  const char* key;
  enum Type : uint8_t { U8, U16, I8, Bool } type;
  int minVal;
  int maxVal;
  int defaultVal;
};

static constexpr CapturedKey kCapturedKeys[] = {
  {"wpm",        CapturedKey::U16,  10,   1000, 300},
  {"bright",     CapturedKey::U8,   0,    4,    3},
  {"dark",       CapturedKey::Bool, 0,    1,    1},
  {"night",      CapturedKey::Bool, 0,    1,    0},
  {"read_mode",  CapturedKey::U8,   0,    1,    0},
  {"handed",     CapturedKey::U8,   0,    1,    0},
  {"phantom_on", CapturedKey::Bool, 0,    1,    1},
  {"prog_md",    CapturedKey::U8,   0,    2,    0},
  {"bat_md",     CapturedKey::U8,   0,    2,    0},
  {"read_bat",   CapturedKey::Bool, 0,    1,    1},
  {"read_ch",    CapturedKey::Bool, 0,    1,    0},
  {"read_pct",   CapturedKey::Bool, 0,    1,    0},
  {"font_size",  CapturedKey::U8,   0,    2,    0},
  {"typeface",   CapturedKey::U8,   0,    2,    0},
  {"type_hlt",   CapturedKey::Bool, 0,    1,    1},
  {"pace_lms",   CapturedKey::U16,  0,    600,  200},
  {"pace_cms",   CapturedKey::U16,  0,    600,  200},
  {"pace_pms",   CapturedKey::U16,  0,    600,  200},
  {"pause_md",   CapturedKey::U8,   0,    1,    0},
  {"type_trk",   CapturedKey::I8,   -2,   3,    0},
  {"type_anc",   CapturedKey::U8,   30,   40,   30},
  {"type_wid",   CapturedKey::U8,   12,   30,   30},
  {"type_gap",   CapturedKey::U8,   2,    8,    5},
  {"sc_font",    CapturedKey::U8,   0,    8,    4},
  {"sc_line_sp", CapturedKey::U8,   0,    2,    1},
  {"sc_margin",  CapturedKey::U8,   0,    2,    1},
  {"ss_mode",    CapturedKey::U8,   0,    6,    0},
  {"ss_timeout", CapturedKey::U8,   0,    5,    2},
  {"focus_clr",  CapturedKey::U8,   0,    5,    0},
};

static constexpr size_t kCapturedKeyCount =
    sizeof(kCapturedKeys) / sizeof(kCapturedKeys[0]);
```

### Excluded Keys (Never Captured)

```cpp
static constexpr const char* kExcludedKeys[] = {
  "wifi_ssid",
  "wifi_pass",
};
```

### Preset JSON Schema

```json
{
  "name": "My Reading Setup",
  "settings": {
    "wpm": 350,
    "bright": 3,
    "dark": true,
    "night": false,
    "read_mode": 0,
    "handed": 0,
    "phantom_on": true,
    "prog_md": 0,
    "bat_md": 0,
    "read_bat": true,
    "read_ch": false,
    "read_pct": false,
    "font_size": 1,
    "typeface": 0,
    "type_hlt": true,
    "pace_lms": 200,
    "pace_cms": 200,
    "pace_pms": 200,
    "pause_md": 0,
    "type_trk": 0,
    "type_anc": 33,
    "type_wid": 30,
    "type_gap": 5,
    "sc_font": 4,
    "sc_line_sp": 1,
    "sc_margin": 1,
    "ss_mode": 0,
    "ss_timeout": 2,
    "focus_clr": 0
  }
}
```

This flat format matches the existing manual JSON parsing approach in `CompanionSyncManager` (using `readJsonInt`, `readJsonBool` helpers), avoiding any external JSON library dependency.

### Filename Convention

Files are named `{millis}_{random_hex}.json` (e.g., `485230_a3f2.json`). The timestamp provides natural ordering for debugging; the random suffix prevents collisions during rapid successive saves.

---

## Error Handling

| Scenario                                    | Behavior                                   |
| ------------------------------------------- | ------------------------------------------ |
| SD card not mounted                         | Show error toast, return to Presets menu   |
| `/config/presets/` directory creation fails | Show error, abort save                     |
| File write incomplete                       | Delete partial file, show error            |
| Malformed JSON on restore                   | Show error, no NVS changes at all          |
| Unknown keys in preset JSON                 | Silently skip, restore remaining keys      |
| Out-of-range values                         | Clamp to valid range boundary, continue    |
| File delete fails                           | Show error, keep entry in list             |
| Missing "name" field during listing         | Skip file, don't show in menu              |
| Preset count at 10 during save              | Return `LimitReached`, hide "Save Current" |

---

## Testing Strategy

### Unit Tests (Example-Based)

- Menu placement: "Presets" at index 4 in SettingsHome
- Navigation: selecting Presets entry transitions to `MenuScreen::Presets`
- Save flow: "Save Current" opens TextEntry with PresetName purpose
- Empty directory: only "Save Current" shown with no presets
- Back navigation: returns to SettingsHome
- Limit enforcement: "Save Current" hidden when 10 presets exist
- Deletion confirmation dialog appears on long-press
- Cancel deletion preserves file and list
- Restore calls `reloadRuntimePreferences()` on success

### Property Tests (100+ iterations each)

- Round-trip: save then restore produces identical NVS state
- Exclusion: preset JSON never contains wifi_ssid or wifi_pass
- Listing: only valid .json files with "name" field appear, sorted alphabetically
- Filename uniqueness: no collisions across many saves
- Name validation: output always ≤ 20 chars, valid charset, empty iff no valid chars
- Range clamping: restored values always within [min, max]
- Partial restore: unmentioned keys unchanged

### Edge Case Tests

- SD card unmounted during save/restore/delete
- Corrupted JSON (missing braces, invalid UTF-8)
- Directory doesn't exist on first save
- Preset file deleted externally between listing and restore

---

## Key Implementation Algorithms

### `savePreset(name, prefs)`

```cpp
SaveResult PresetManager::savePreset(const String& name, Preferences& prefs) {
  if (presetCount() >= kMaxPresets) return SaveResult::LimitReached;

  if (!SD_MMC.exists(kPresetsDir)) {
    if (!SD_MMC.mkdir(kPresetsDir)) return SaveResult::DirectoryError;
  }

  String json = serializePreset(name, prefs);
  String filename = generateFilename();
  String path = String(kPresetsDir) + "/" + filename;

  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) return SaveResult::WriteError;

  size_t written = file.print(json);
  file.close();

  if (written != json.length()) {
    SD_MMC.remove(path.c_str());
    return SaveResult::WriteError;
  }
  return SaveResult::Ok;
}
```

### `restorePreset(filename, prefs)`

```cpp
RestoreResult PresetManager::restorePreset(const String& filename, Preferences& prefs) {
  String path = String(kPresetsDir) + "/" + filename;
  File file = SD_MMC.open(path, FILE_READ);
  if (!file) return RestoreResult::FileNotFound;

  String content = file.readString();
  file.close();

  if (content.isEmpty()) return RestoreResult::ParseError;
  if (!deserializeAndApply(content, prefs)) return RestoreResult::ParseError;

  return RestoreResult::Ok;
}
```

### `validateName(raw)`

```cpp
String PresetManager::validateName(const String& raw) {
  String result;
  result.reserve(kMaxPresetNameLength);

  for (size_t i = 0; i < raw.length() && result.length() < kMaxPresetNameLength; ++i) {
    char c = raw[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_') {
      result += c;
    }
  }
  result.trim();
  return result;
}
```

### `generateFilename()`

```cpp
String PresetManager::generateFilename() {
  uint32_t ts = millis();
  uint16_t rnd = esp_random() & 0xFFFF;
  char buf[24];
  snprintf(buf, sizeof(buf), "%lu_%04x.json", (unsigned long)ts, rnd);
  return String(buf);
}
```

---

## Correctness Properties

_A property is a characteristic or behavior that should hold true across all valid executions of a system — essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees._

### Property 1: Serialization Round-Trip

_For any_ valid combination of captured NVS settings values, saving those settings as a preset and then restoring that preset SHALL produce identical NVS values for all captured keys.

**Validates: Requirements 3.2, 4.3, 5.2, 9.1**

### Property 2: Exclusion Invariant

_For any_ preset saved by the firmware, regardless of the current NVS state (including any values stored under wifi_ssid or wifi_pass), the resulting preset JSON SHALL NOT contain any excluded settings keys.

**Validates: Requirements 4.5, 9.2**

### Property 3: Preset Listing Correctness

_For any_ collection of files in `/config/presets/` (including .json files with valid "name" fields, .json files without "name" fields, and non-.json files), the `listPresets()` function SHALL return only entries for files that (a) have a `.json` extension AND (b) contain a valid `"name"` string field, ordered alphabetically by name.

**Validates: Requirements 2.2, 2.4, 10.4, 10.5**

### Property 4: Filename Uniqueness

_For any_ sequence of `savePreset` calls, all generated filenames SHALL be distinct (no two calls produce the same filename).

**Validates: Requirements 3.6**

### Property 5: Preset Count Invariant

_For any_ sequence of save and delete operations, the number of preset files in `/config/presets/` SHALL never exceed 10. A save attempt when 10 presets exist SHALL be rejected with `LimitReached`.

**Validates: Requirements 7.1**

### Property 6: Name Validation

_For any_ input string, `validateName` SHALL return a result that is (a) at most 20 characters long, (b) composed only of alphanumeric characters, spaces, hyphens, and underscores, and (c) non-empty if and only if the input contains at least one allowed character.

**Validates: Requirements 8.1, 8.4**

### Property 7: Range Clamping on Restore

_For any_ preset JSON containing a numeric settings value outside the defined valid range for its key, restoring that preset SHALL write the value clamped to [min, max] for that key into NVS — never the raw out-of-range value.

**Validates: Requirements 10.2**

### Property 8: Partial Restore Preserves Unmentioned Keys

_For any_ preset JSON containing a proper subset of captured settings keys, restoring that preset SHALL modify only the NVS keys present in the JSON and SHALL leave all other NVS keys at their pre-restore values.

**Validates: Requirements 5.6, 10.1**
