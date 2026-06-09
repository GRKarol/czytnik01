# Requirements Document

## Introduction

The e-reader firmware supports extensive reading and display customization (WPM, pacing delays, font, theme, brightness, typography tuning, screensaver, handedness, footer/battery modes, phantom words, scroll settings). When lending the device, a friend may change all of these settings. Currently there is no way to snapshot and restore a full configuration.

This feature introduces **Settings Presets** — named snapshots of all reading and display settings stored as JSON files on the SD card. Users can save their current configuration as a preset, manage up to 10 presets, and restore any preset with a single tap. Connectivity settings (Wi-Fi, BLE) are excluded from presets. The Presets menu is accessible as a new top-level entry in Settings Home.

## Glossary

- **Firmware**: The ESP32-S3 firmware running on the Flower e-reader device
- **Preset**: A named snapshot of all reading and display settings, stored as a JSON file on the SD card
- **Preset_File**: A JSON file located in `/config/presets/` on the SD card containing all captured settings key-value pairs and a user-assigned name
- **Presets_Menu**: A new MenuScreen accessible from Settings Home that lists saved presets and provides save/delete actions
- **Settings_Home**: The top-level settings menu listing Reading, Display, Connectivity, Presets, and About entries
- **NVS**: Non-Volatile Storage on the ESP32-S3, used to persist user settings as key-value pairs under namespace "rsvp"
- **StorageManager**: The firmware component that handles SD card file system access
- **Captured_Settings**: The set of all reading and display NVS preference keys included in a preset snapshot (wpm, bright, dark, night, read_mode, handed, phantom_on, prog_md, bat_md, read_bat, read_ch, read_pct, font_size, typeface, type_hlt, pace_lms, pace_cms, pace_pms, pause_md, type_trk, type_anc, type_wid, type_gap, sc_font, sc_line_sp, sc_margin, screensaver-related keys, and focus color)
- **Excluded_Settings**: Connectivity settings (Wi-Fi SSID, Wi-Fi password, BLE-related configuration) that are never captured or restored by a preset

## Requirements

### Requirement 1: Presets Menu Placement in Settings Home

**User Story:** As a reader, I want to access my settings presets from the main settings screen, so that I can quickly save or restore a configuration.

#### Acceptance Criteria

1. THE Settings_Home SHALL display a "Presets" entry at position 4 in the menu list (after Reading, Display, Connectivity and before About)
2. WHEN the user selects the Presets entry in Settings_Home, THE Firmware SHALL navigate to the Presets_Menu screen
3. THE Firmware SHALL include a Presets value in the MenuScreen enum to represent the Presets_Menu screen

### Requirement 2: Presets Menu Content and Layout

**User Story:** As a reader, I want to see all my saved presets in a list with a clear option to save a new one, so that I can manage my configurations at a glance.

#### Acceptance Criteria

1. WHEN the Presets_Menu is opened, THE Firmware SHALL display a "Save Current" action item as the first entry in the list
2. WHEN the Presets_Menu is opened, THE Firmware SHALL list all existing presets by name below the "Save Current" entry, ordered by file creation or alphabetical order
3. WHEN no presets are saved, THE Firmware SHALL display only the "Save Current" entry in the Presets_Menu
4. THE Presets_Menu SHALL display the preset name for each saved preset entry
5. WHEN the user navigates away from the Presets_Menu, THE Firmware SHALL return to the Settings_Home screen

### Requirement 3: Save Current Settings as Preset

**User Story:** As a reader, I want to save my current settings configuration under a custom name, so that I can restore it later after someone else changes my settings.

#### Acceptance Criteria

1. WHEN the user selects the "Save Current" entry in the Presets_Menu, THE Firmware SHALL open a text entry screen prompting the user to enter a preset name
2. WHEN the user confirms a preset name, THE Firmware SHALL capture all Captured_Settings values from NVS and write them to a new Preset_File on the SD card
3. THE Firmware SHALL store the preset name inside the Preset_File JSON as a "name" field
4. WHEN the preset is saved successfully, THE Firmware SHALL return to the Presets_Menu showing the new preset in the list
5. IF the SD card is not mounted or the write operation fails, THEN THE Firmware SHALL display an error message and return to the Presets_Menu without creating a file
6. THE Firmware SHALL generate a unique filename for each Preset_File to avoid collisions (using a timestamp or sequential identifier)

### Requirement 4: Preset Storage Format and Location

**User Story:** As a reader, I want my presets stored on the SD card as readable files, so that they persist across firmware updates and can be managed externally if needed.

#### Acceptance Criteria

1. THE Firmware SHALL store all Preset_Files in the `/config/presets/` directory on the SD card
2. WHEN the `/config/presets/` directory does not exist, THE Firmware SHALL create the directory before writing a Preset_File
3. THE Firmware SHALL serialize each preset as a JSON object containing a "name" string field and a "settings" object field with key-value pairs matching NVS preference keys
4. THE Firmware SHALL use the `.json` file extension for all Preset_Files
5. THE Firmware SHALL NOT include any Excluded_Settings keys in the Preset_File JSON

### Requirement 5: Restore a Preset

**User Story:** As a reader, I want to restore a saved preset with a single tap, so that I can instantly recover my preferred reading configuration after lending the device.

#### Acceptance Criteria

1. WHEN the user selects a saved preset entry in the Presets_Menu, THE Firmware SHALL read the corresponding Preset_File from the SD card
2. WHEN the Preset_File is read successfully, THE Firmware SHALL write all settings values from the Preset_File "settings" object into NVS under their corresponding preference keys
3. WHEN all NVS writes are complete, THE Firmware SHALL call the runtime preferences reload function to apply restored settings immediately without requiring a device restart
4. WHEN a preset is restored successfully, THE Firmware SHALL display a confirmation message indicating the preset name that was applied
5. IF the Preset_File cannot be read (file missing, SD card error, malformed JSON), THEN THE Firmware SHALL display an error message and leave current settings unchanged
6. IF a Preset_File contains a settings key that is not recognized by the current firmware version, THEN THE Firmware SHALL skip that key and continue restoring the remaining settings

### Requirement 6: Delete a Preset

**User Story:** As a reader, I want to delete presets I no longer need, so that my preset list stays manageable and I free up space.

#### Acceptance Criteria

1. WHEN the user long-presses or selects a delete action on a preset entry in the Presets_Menu, THE Firmware SHALL display a confirmation prompt asking the user to confirm deletion
2. WHEN the user confirms deletion, THE Firmware SHALL remove the corresponding Preset_File from the SD card
3. WHEN the file is deleted successfully, THE Firmware SHALL refresh the Presets_Menu list to reflect the removal
4. IF the file deletion fails, THEN THE Firmware SHALL display an error message and retain the preset entry in the list
5. WHEN the user cancels the deletion confirmation, THE Firmware SHALL return to the Presets_Menu without modifying any files

### Requirement 7: Preset Limit Enforcement

**User Story:** As a reader, I want the system to enforce a maximum preset count, so that SD card storage is used responsibly and the menu remains navigable.

#### Acceptance Criteria

1. THE Firmware SHALL allow a maximum of 10 saved presets at any time
2. WHILE 10 presets exist on the SD card, THE Firmware SHALL disable or hide the "Save Current" entry in the Presets_Menu
3. WHILE 10 presets exist on the SD card, THE Firmware SHALL display a label indicating the preset limit has been reached
4. WHEN the user deletes a preset and the count drops below 10, THE Firmware SHALL re-enable the "Save Current" entry in the Presets_Menu

### Requirement 8: Preset Name Validation

**User Story:** As a reader, I want guidance on valid preset names, so that my presets are clearly identifiable and stored correctly.

#### Acceptance Criteria

1. THE Firmware SHALL accept preset names with a length between 1 and 20 characters
2. IF the user submits an empty preset name, THEN THE Firmware SHALL reject the submission and keep the text entry screen open
3. IF the user submits a preset name exceeding 20 characters, THEN THE Firmware SHALL truncate the name to 20 characters before saving
4. THE Firmware SHALL allow alphanumeric characters, spaces, and basic punctuation (hyphen, underscore) in preset names

### Requirement 9: Captured Settings Scope

**User Story:** As a reader, I want presets to capture all my reading and display preferences but not connectivity settings, so that restoring a preset does not break my Wi-Fi connection.

#### Acceptance Criteria

1. THE Firmware SHALL include the following NVS keys in each preset snapshot: wpm, bright, dark, night, read_mode, handed, phantom_on, prog_md, bat_md, read_bat, read_ch, read_pct, font_size, typeface, type_hlt, pace_lms, pace_cms, pace_pms, pause_md, type_trk, type_anc, type_wid, type_gap, sc_font, sc_line_sp, sc_margin
2. THE Firmware SHALL NOT include the following NVS keys in any preset snapshot: wifi_ssid, wifi_pass
3. WHEN a new reading or display NVS preference key is added to the firmware in a future update, THE preset save operation SHALL capture all keys listed in the Captured_Settings definition at compile time
4. THE Firmware SHALL include screensaver mode and screensaver timeout settings in each preset snapshot

### Requirement 10: Preset File Integrity on Load

**User Story:** As a reader, I want the system to gracefully handle corrupted or outdated preset files, so that a bad file does not crash the device or corrupt my current settings.

#### Acceptance Criteria

1. WHEN a Preset_File contains valid JSON but is missing one or more expected settings keys, THE Firmware SHALL restore only the keys present in the file and leave missing keys at their current NVS values
2. WHEN a Preset_File contains a settings value outside the valid range for a given key, THE Firmware SHALL clamp the value to the valid range before writing it to NVS
3. IF a Preset_File contains invalid JSON (parse failure), THEN THE Firmware SHALL reject the entire file, display an error message, and leave all current settings unchanged
4. WHEN listing presets at Presets_Menu open, THE Firmware SHALL skip any file in `/config/presets/` that does not have a `.json` extension
5. WHEN listing presets at Presets_Menu open, THE Firmware SHALL skip any JSON file that does not contain a valid "name" string field
