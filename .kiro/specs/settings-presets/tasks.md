# Implementation Plan: Settings Presets

## Overview

Implement the PresetManager module and integrate it with the App class menu system. The feature enables users to save, list, restore, and delete named snapshots of reading/display settings as JSON files on the SD card. Implementation follows a bottom-up approach: core data structures first, then logic layer, then UI integration and wiring.

## Tasks

- [x] 1. Create PresetManager core module
  - [x] 1.1 Create `firmware/src/storage/PresetManager.h` with class interface
    - Define the `PresetManager` class with all public types (`PresetInfo`, `SaveResult`, `RestoreResult`, `DeleteResult`)
    - Define public method signatures: `listPresets()`, `savePreset()`, `restorePreset()`, `deletePreset()`, `validateName()`, `presetCount()`
    - Define constants: `kMaxPresets = 10`, `kMaxPresetNameLength = 20`, `kPresetsDir = "/config/presets"`
    - Define private helpers: `generateFilename()`, `serializePreset()`, `deserializeAndApply()`, `extractPresetName()`, `clampValue()`, `jsonEscape()`
    - _Requirements: 4.1, 4.3, 4.4, 7.1_

  - [x] 1.2 Create `firmware/src/storage/PresetManager.cpp` with CapturedKey table and excluded keys
    - Define the `CapturedKey` struct with `key`, `type` enum (U8, U16, I8, Bool), `minVal`, `maxVal`, `defaultVal`
    - Define the `kCapturedKeys[]` constexpr array with all 29 captured settings keys and their ranges
    - Define `kExcludedKeys[]` array with `wifi_ssid` and `wifi_pass`
    - _Requirements: 9.1, 9.2, 9.3, 9.4_

  - [x] 1.3 Implement `validateName()` in PresetManager.cpp
    - Filter input to only alphanumeric, space, hyphen, underscore characters
    - Truncate to 20 characters max
    - Trim whitespace from result
    - Return empty string if no valid characters remain
    - _Requirements: 8.1, 8.2, 8.3, 8.4_

  - [x] 1.4 Implement `generateFilename()` and `jsonEscape()` helpers
    - Generate unique filename using `millis()` and `esp_random()` as `{millis}_{hex4}.json`
    - Implement JSON string escaping for safe embedding of preset names
    - _Requirements: 3.6, 4.4_

  - [x] 1.5 Implement `serializePreset()` — NVS to JSON serialization
    - Read all captured keys from NVS `Preferences` using appropriate typed getters (getUChar, getUShort, getChar, getBool)
    - Build JSON string manually (matching CompanionSyncManager approach) with `"name"` and `"settings"` fields
    - Ensure excluded keys are never serialized
    - _Requirements: 3.2, 3.3, 4.3, 4.5, 9.1, 9.2_

  - [x] 1.6 Implement `deserializeAndApply()` — JSON to NVS restore
    - Parse JSON manually using string scanning (matching existing `readJsonInt`/`readJsonBool` patterns)
    - For each key in parsed `"settings"` object: look up in `kCapturedKeys[]`, skip if unknown
    - Clamp numeric values to [min, max] range before writing to NVS
    - Use typed NVS putters (putUChar, putUShort, putChar, putBool)
    - Return false on malformed JSON (missing braces, parse failure)
    - _Requirements: 5.2, 5.6, 10.1, 10.2, 10.3_

  - [x] 1.7 Implement `savePreset()` — full save flow
    - Check `presetCount() >= kMaxPresets`, return `LimitReached` if at limit
    - Create `/config/presets/` directory if not existing via `SD_MMC.mkdir()`
    - Call `serializePreset()` to build JSON content
    - Generate unique filename, open file for write, write content
    - Verify written bytes match expected; delete partial file on failure
    - Return appropriate `SaveResult` code
    - _Requirements: 3.2, 3.5, 3.6, 4.1, 4.2, 7.1_

  - [x] 1.8 Implement `restorePreset()` — full restore flow
    - Open file by filename from `/config/presets/`
    - Read entire file content as string
    - Call `deserializeAndApply()` to parse and write to NVS
    - Return appropriate `RestoreResult` code
    - _Requirements: 5.1, 5.2, 5.5_

  - [x] 1.9 Implement `deletePreset()` and `listPresets()`
    - `deletePreset()`: remove file via `SD_MMC.remove()`, return result code
    - `listPresets()`: open directory, iterate files, filter `.json` extension, read each file to extract `"name"` field via `extractPresetName()`, skip files without valid name, sort results alphabetically by name
    - `presetCount()`: return size of `listPresets()` result
    - _Requirements: 2.2, 2.3, 6.2, 10.4, 10.5_

- [x] 2. Checkpoint - Ensure PresetManager compiles
  - Ensure the PresetManager module compiles cleanly with `pio build`. Ask the user if questions arise.

- [x] 3. Extend App enums and add new members
  - [x] 3.1 Add `Presets` and `PresetsDeleteConfirm` to `MenuScreen` enum in App.h
    - Add enum values after existing entries
    - _Requirements: 1.3_

  - [x] 3.2 Add `PresetName` to `TextEntryPurpose` enum in App.h
    - Add value after existing `SavePointName` entry
    - _Requirements: 3.1_

  - [x] 3.3 Add PresetManager member and preset state variables to App.h
    - Add `#include "storage/PresetManager.h"`
    - Add `PresetManager presetManager_;` private member
    - Add `std::vector<String> presetFilenames_;` for menu index to filename mapping
    - Add `size_t presetsDeleteTargetIndex_ = 0;` for tracking delete target
    - Add `size_t presetsSelectedIndex_ = 0;` for menu selection tracking
    - _Requirements: 1.2, 6.1_

  - [x] 3.4 Declare new App methods for presets navigation
    - Declare `openPresets()`, `selectPresetsItem(uint32_t nowMs)`, `confirmDeletePreset(size_t index, uint32_t nowMs)`, `executeDeletePreset(uint32_t nowMs)`, `executeSavePreset(uint32_t nowMs)`, `executeRestorePreset(size_t index, uint32_t nowMs)`
    - _Requirements: 1.2, 3.1, 5.1, 6.1_

- [x] 4. Implement App presets UI logic
  - [x] 4.1 Implement `openPresets()` in App.cpp
    - Set `menuScreen_` to `MenuScreen::Presets`
    - Call `presetManager_.listPresets()` and populate `presetFilenames_` vector
    - Build `settingsMenuItems_` with Back, Save Current (if count < 10, else limit label), and preset names
    - Set `presetsSelectedIndex_ = 0`
    - Render the menu
    - _Requirements: 1.2, 2.1, 2.2, 2.3, 7.2, 7.3_

  - [x] 4.2 Implement `selectPresetsItem()` in App.cpp
    - Handle Back (index 0): return to SettingsHome
    - Handle Save Current (index 1): open TextEntry with purpose `PresetName`, title "Preset Name", max length 20
    - Handle preset selection (index >= 2): call `executeRestorePreset()` with mapped filename
    - Handle long-press on preset entry: call `confirmDeletePreset()`
    - _Requirements: 2.5, 3.1, 5.1, 6.1_

  - [x] 4.3 Implement `executeSavePreset()` in App.cpp
    - Called from `commitTextEntry()` when purpose is `PresetName`
    - Validate name via `presetManager_.validateName()`
    - Call `presetManager_.savePreset()` with validated name and `preferences_`
    - Show success toast or error message based on result
    - Call `openPresets()` to refresh the list
    - _Requirements: 3.2, 3.4, 3.5, 8.1, 8.2_

  - [x] 4.4 Implement `executeRestorePreset()` in App.cpp
    - Call `presetManager_.restorePreset(filename, preferences_)`
    - On success: call `reloadRuntimePreferences(nowMs, true)` and show confirmation toast with preset name
    - On failure: show error message, leave settings unchanged
    - Return to Presets menu
    - _Requirements: 5.2, 5.3, 5.4, 5.5_

  - [x] 4.5 Implement `confirmDeletePreset()` and `executeDeletePreset()` in App.cpp
    - `confirmDeletePreset()`: store target index in `presetsDeleteTargetIndex_`, switch to `MenuScreen::PresetsDeleteConfirm`, render confirmation prompt
    - `executeDeletePreset()`: call `presetManager_.deletePreset()` with stored filename, show success/error, call `openPresets()` to refresh
    - Handle cancel: return to Presets menu without changes
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

- [x] 5. Wire presets into navigation and menu system
  - [x] 5.1 Update Settings Home menu to include Presets entry at index 4
    - In `rebuildSettingsMenuItems()` or equivalent, insert "Presets" label at position 4
    - Shift "About" to index 5 (and dev-mode items accordingly)
    - Update existing index constants if they exist
    - _Requirements: 1.1_

  - [x] 5.2 Update `selectSettingsItem()` to handle Presets index
    - Add case for index 4 that calls `openPresets()`
    - Adjust existing cases for About and dev-mode items (shifted indices)
    - _Requirements: 1.2_

  - [x] 5.3 Update `selectMenuItem()` / touch handling to route `MenuScreen::Presets` and `MenuScreen::PresetsDeleteConfirm`
    - In the menu screen dispatch logic, route `Presets` to `selectPresetsItem()`
    - Route `PresetsDeleteConfirm` to execute/cancel delete
    - _Requirements: 1.2, 6.1, 6.5_

  - [x] 5.4 Update `commitTextEntry()` to handle `TextEntryPurpose::PresetName`
    - When text entry purpose is `PresetName`, call `executeSavePreset()`
    - _Requirements: 3.1, 3.2_

  - [x] 5.5 Update `isSettingsListScreen()` to include `MenuScreen::Presets` and `MenuScreen::PresetsDeleteConfirm`
    - Add the new screen enum values to the list of settings screens
    - _Requirements: 1.2_

  - [x] 5.6 Update menu limit re-enable logic after deletion
    - After a preset is deleted and count drops below 10, the refreshed menu shows "Save Current" again
    - _Requirements: 7.4_

- [x] 6. Checkpoint - Full build and manual verification
  - Ensure all tests pass and the firmware compiles cleanly with `pio build`. Ask the user if questions arise.

- [ ]\* 7. Property-based tests
  - [ ]\* 7.1 Write property test for serialization round-trip
    - **Property 1: Serialization Round-Trip**
    - Generate random valid NVS values within each key's [min, max] range, save as preset, restore, verify all values match
    - **Validates: Requirements 3.2, 4.3, 5.2, 9.1**

  - [ ]\* 7.2 Write property test for exclusion invariant
    - **Property 2: Exclusion Invariant**
    - For any generated preset JSON, assert it never contains `wifi_ssid` or `wifi_pass` keys
    - **Validates: Requirements 4.5, 9.2**

  - [ ]\* 7.3 Write property test for name validation
    - **Property 6: Name Validation**
    - For any random input string, assert `validateName()` output is ≤ 20 chars, only allowed characters, non-empty iff input has at least one valid char
    - **Validates: Requirements 8.1, 8.4**

  - [ ]\* 7.4 Write property test for range clamping
    - **Property 7: Range Clamping on Restore**
    - Generate preset JSON with out-of-range values, restore, verify NVS values are clamped to [min, max]
    - **Validates: Requirements 10.2**

  - [ ]\* 7.5 Write property test for partial restore
    - **Property 8: Partial Restore Preserves Unmentioned Keys**
    - Generate preset JSON with a random subset of keys, set known NVS state, restore, verify unmentioned keys unchanged
    - **Validates: Requirements 5.6, 10.1**

  - [ ]\* 7.6 Write property test for filename uniqueness
    - **Property 4: Filename Uniqueness**
    - Generate many filenames via `generateFilename()`, assert no duplicates
    - **Validates: Requirements 3.6**

  - [ ]\* 7.7 Write property test for preset count invariant
    - **Property 5: Preset Count Invariant**
    - Simulate save/delete sequences, assert count never exceeds 10 and save at limit returns `LimitReached`
    - **Validates: Requirements 7.1**

  - [ ]\* 7.8 Write property test for listing correctness
    - **Property 3: Preset Listing Correctness**
    - Create mix of valid/invalid files in presets directory, verify `listPresets()` returns only valid entries sorted alphabetically
    - **Validates: Requirements 2.2, 2.4, 10.4, 10.5**

- [x] 8. Final checkpoint - Ensure all tests pass
  - Ensure all tests pass and the full firmware builds cleanly. Ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document
- The implementation uses manual JSON parsing (no external library) matching the existing `CompanionSyncManager` approach
- All SD card operations use `SD_MMC` consistent with the existing `StorageManager` patterns
- `reloadRuntimePreferences()` is already implemented in App and will be called after restore

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2"] },
    { "id": 1, "tasks": ["1.3", "1.4"] },
    { "id": 2, "tasks": ["1.5", "1.6"] },
    { "id": 3, "tasks": ["1.7", "1.8", "1.9"] },
    { "id": 4, "tasks": ["3.1", "3.2"] },
    { "id": 5, "tasks": ["3.3", "3.4"] },
    { "id": 6, "tasks": ["4.1", "4.2"] },
    { "id": 7, "tasks": ["4.3", "4.4", "4.5"] },
    { "id": 8, "tasks": ["5.1"] },
    { "id": 9, "tasks": ["5.2", "5.3", "5.4", "5.5"] },
    { "id": 10, "tasks": ["5.6"] },
    { "id": 11, "tasks": ["7.1", "7.2", "7.3", "7.4", "7.5", "7.6", "7.7", "7.8"] }
  ]
}
```
