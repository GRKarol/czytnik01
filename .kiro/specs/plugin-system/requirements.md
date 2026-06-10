# Requirements Document

## Introduction

Native plugin system for the ESP32-S3 e-reader (czytnik01) that replaces the current OTA variant-switching mechanism. Plugins are position-independent C++ binaries stored on SD card, loaded into PSRAM at runtime, and executed via a defined SDK interface. A Plugin Library connects to GitHub to browse, download, install, and remove plugins without SD card removal or device restart. Crash isolation via FreeRTOS tasks and a watchdog ensures firmware stability regardless of plugin behavior. The first plugin extracts the existing FocusTimer (klepsydra) from firmware into a standalone plugin binary.

## Glossary

- **Plugin_Loader**: Firmware component responsible for reading a plugin binary from SD card, loading it into PSRAM, relocating symbols, and invoking the Plugin SDK lifecycle functions.
- **Plugin_SDK**: C++ interface that every plugin binary must implement, defining lifecycle hooks (init, destroy, update, handle_button, handle_touch, draw) and providing access to device services.
- **Plugin_Binary**: Position-independent compiled C++ binary (.bin) stored on SD card that conforms to the Plugin_SDK interface.
- **Plugin_Manifest**: JSON file (manifest.json) co-located with a Plugin_Binary describing the plugin metadata (id, name, version, author, SDK version, permissions).
- **Plugin_Registry**: JSON file (plugins-registry.json) hosted on GitHub containing the list of all available plugins with their metadata and download URLs.
- **Plugin_Library**: On-device UI screen that connects to WiFi, fetches the Plugin_Registry, displays available plugins, and allows the user to download, install, or remove plugins.
- **Plugin_Task**: Dedicated FreeRTOS task in which a loaded plugin executes, providing crash isolation from the main firmware.
- **Plugin_Watchdog**: Software watchdog monitoring a Plugin_Task that detects hangs and terminates unresponsive plugins.
- **PSRAM**: 8 MB OPI external pseudo-static RAM on the ESP32-S3 used as the execution region for loaded Plugin_Binaries.
- **SD_Card**: MicroSD card managed by StorageManager used to persistently store Plugin_Binaries and their manifests.
- **Firmware**: The core e-reader application running on the ESP32-S3 that hosts the Plugin_Loader and Plugin_Library.
- **Device_Services**: Set of firmware APIs exposed to plugins through the Plugin_SDK including display, IMU, audio, storage, and orientation services.

## Requirements

### Requirement 1: Plugin SDK Interface

**User Story:** As a plugin developer, I want a well-defined C++ interface so that I can build plugins that integrate with the e-reader hardware and firmware services.

#### Acceptance Criteria

1. THE Plugin_SDK SHALL define the following lifecycle function pointers: `plugin_init`, `plugin_destroy`, `plugin_update`, `plugin_handle_button`, `plugin_handle_touch`, and `plugin_draw`.
2. THE Plugin_SDK SHALL define a versioned structure containing all lifecycle function pointers that the Plugin_Loader reads from a fixed symbol offset in the Plugin_Binary.
3. THE Plugin_SDK SHALL expose Device_Services accessors for DisplayManager rendering functions, AudioManager playback, IMU accelerometer readings, device orientation state, and SD_Card file operations.
4. THE Plugin_SDK SHALL define a `plugin_get_info` function that returns the plugin name, version, and required SDK version.
5. WHEN `plugin_init` is called, THE Plugin_SDK SHALL pass a context structure containing pointers to all available Device_Services.
6. WHEN `plugin_destroy` is called, THE Plugin_SDK SHALL guarantee that all Device_Services references become invalid and the plugin must release all allocated resources.

### Requirement 2: Plugin Binary Format

**User Story:** As a build system maintainer, I want plugins compiled as position-independent binaries so that they can be loaded at any PSRAM address without recompilation.

#### Acceptance Criteria

1. THE Plugin_Binary SHALL be compiled as a position-independent executable using the Xtensa GCC toolchain with `-fPIC` and appropriate linker flags.
2. THE Plugin_Binary SHALL contain a fixed-offset header structure identifying the SDK version, entry point offset, and binary size.
3. THE Plugin_Binary SHALL contain no absolute address references to firmware symbols; all firmware interaction SHALL occur through the Device_Services context passed at init.
4. THE Plugin_Binary SHALL be stored on SD_Card at the path `/plugins/{plugin_id}/plugin.bin`.
5. THE Plugin_Manifest SHALL be stored on SD_Card at the path `/plugins/{plugin_id}/manifest.json`.
6. THE Plugin_Manifest SHALL contain the fields: `id`, `name`, `version`, `author`, `sdk_version`, `description`, and `permissions`.

### Requirement 3: Plugin Loader

**User Story:** As a user, I want installed plugins to load from SD card into memory so that I can use them without reflashing the device.

#### Acceptance Criteria

1. WHEN the user selects an installed plugin from the device menu, THE Plugin_Loader SHALL read the Plugin_Binary from SD_Card into a PSRAM-allocated buffer.
2. WHEN the Plugin_Binary is loaded into PSRAM, THE Plugin_Loader SHALL validate the binary header SDK version against the firmware-supported SDK version.
3. IF the Plugin_Binary header SDK version does not match the firmware-supported SDK version, THEN THE Plugin_Loader SHALL display an incompatibility error message and abort loading.
4. WHEN the Plugin_Binary passes validation, THE Plugin_Loader SHALL resolve the entry point offset and invoke `plugin_init` with the Device_Services context.
5. WHEN `plugin_init` returns a failure code, THE Plugin_Loader SHALL free the PSRAM allocation, log the error, and return the user to the menu.
6. THE Plugin_Loader SHALL track PSRAM usage and refuse to load a Plugin_Binary that exceeds available PSRAM capacity.
7. WHEN the user exits a running plugin, THE Plugin_Loader SHALL invoke `plugin_destroy`, free the PSRAM allocation, and return to the main menu.

### Requirement 4: Crash Isolation

**User Story:** As a user, I want the device firmware to remain stable even if a plugin crashes or hangs so that I never lose access to the main e-reader functionality.

#### Acceptance Criteria

1. WHEN a plugin is launched, THE Plugin_Loader SHALL execute the plugin lifecycle within a dedicated Plugin_Task with its own FreeRTOS stack.
2. THE Plugin_Task SHALL have a configurable stack size with a default of 8192 bytes allocated from PSRAM.
3. WHEN a Plugin_Task is created, THE Plugin_Watchdog SHALL start monitoring the task with a configurable timeout defaulting to 5000 milliseconds.
4. IF the Plugin_Task does not reset the Plugin_Watchdog within the timeout period, THEN THE Plugin_Watchdog SHALL terminate the Plugin_Task, free all plugin-allocated PSRAM, and return the user to the main menu.
5. IF the Plugin_Task triggers a stack overflow or memory access violation, THEN THE Firmware SHALL catch the exception, terminate the Plugin_Task, free plugin memory, and display an error message to the user.
6. WHILE a plugin is running in a Plugin_Task, THE Firmware SHALL continue processing power button events, battery monitoring, and watchdog resets on the main task.

### Requirement 5: Plugin Library (Online Store)

**User Story:** As a user, I want to browse and download plugins over WiFi so that I can extend my e-reader without removing the SD card or connecting to a computer.

#### Acceptance Criteria

1. WHEN the user opens the Plugin_Library screen, THE Firmware SHALL connect to WiFi using stored credentials.
2. IF WiFi connection fails, THEN THE Plugin_Library SHALL display a connection error and offer to open WiFi settings.
3. WHEN WiFi is connected, THE Plugin_Library SHALL fetch the Plugin_Registry JSON from the configured GitHub repository release assets.
4. THE Plugin_Library SHALL display the list of available plugins with name, description, version, and installed status on the e-ink display.
5. WHEN the user selects a plugin for download, THE Plugin_Library SHALL download the Plugin_Binary and Plugin_Manifest from the URL specified in the Plugin_Registry to the SD_Card path `/plugins/{plugin_id}/`.
6. WHILE a download is in progress, THE Plugin_Library SHALL display a progress indicator showing percentage complete.
7. IF a download fails or is interrupted, THEN THE Plugin_Library SHALL remove any partially written files and display an error message.
8. THE Plugin_Library SHALL compare the installed plugin version from the local Plugin_Manifest against the Plugin_Registry version and indicate when an update is available.

### Requirement 6: Plugin Installation and Removal

**User Story:** As a user, I want to install and remove plugins from the device UI without restarting the device so that plugin management is seamless.

#### Acceptance Criteria

1. WHEN the user confirms plugin installation from the Plugin_Library, THE Firmware SHALL write the Plugin_Binary and Plugin_Manifest to `/plugins/{plugin_id}/` on SD_Card.
2. WHEN installation is complete, THE Firmware SHALL add the plugin to the installed plugins list and make it available in the main menu without requiring a device restart.
3. WHEN the user selects "Remove" for an installed plugin, THE Firmware SHALL delete the `/plugins/{plugin_id}/` directory and its contents from SD_Card.
4. WHEN plugin removal is complete, THE Firmware SHALL remove the plugin from the installed plugins list and update the menu without requiring a device restart.
5. IF the user attempts to remove a currently running plugin, THEN THE Firmware SHALL first invoke `plugin_destroy`, terminate the Plugin_Task, free PSRAM, and then proceed with file removal.
6. THE Firmware SHALL persist the installed plugins list so that it survives power cycles.

### Requirement 7: Plugin Registry Format

**User Story:** As a project maintainer, I want a structured registry hosted on GitHub so that the device can discover available plugins and their download locations.

#### Acceptance Criteria

1. THE Plugin_Registry SHALL be a JSON file named `plugins-registry.json` included in GitHub Release assets of the repository GRKarol/czytnik01.
2. THE Plugin_Registry SHALL contain an array of plugin entries, each with the fields: `id`, `name`, `description`, `version`, `author`, `sdk_version`, `binary_url`, `manifest_url`, and `size_bytes`.
3. THE Plugin_Registry SHALL include a `registry_version` field at the root level to allow future schema changes.
4. THE Plugin_Registry SHALL include a `min_firmware_version` field per plugin entry to indicate minimum compatible firmware version.

### Requirement 8: FocusTimer Plugin Extraction

**User Story:** As a developer, I want to extract the existing FocusTimer (klepsydra) from firmware into a standalone plugin so that it serves as the reference implementation for the plugin system.

#### Acceptance Criteria

1. THE FocusTimer plugin SHALL implement all Plugin_SDK lifecycle functions (init, destroy, update, handle_button, handle_touch, draw).
2. THE FocusTimer plugin SHALL replicate the full functionality of the built-in FocusTimer including the 11-state state machine, genre selection, orientation-based input, and timer modes (touch, work, break).
3. THE FocusTimer plugin SHALL access the IMU (QMI8658) accelerometer through the Device_Services interface for orientation detection and flip-based input.
4. THE FocusTimer plugin SHALL access the AudioManager through the Device_Services interface for completion cue playback.
5. THE FocusTimer plugin SHALL access the DisplayManager through the Device_Services interface for rendering the timer screen with mode, genre, timer value, instructions, and progress.
6. THE FocusTimer plugin SHALL access device orientation state through the Device_Services interface to determine portrait rendering orientation based on the device short-side position.
7. WHEN the FocusTimer plugin is installed, THE Firmware SHALL present it in the main menu identically to the current built-in FocusTimer entry.

### Requirement 9: OTA Variant System Replacement

**User Story:** As a developer, I want to remove the OTA variant system so that plugin management is fully handled by the native plugin loader instead of compile-time feature flags and firmware reflashing.

#### Acceptance Criteria

1. THE Firmware SHALL remove the build-variant OTA mechanism that uses NVS bitmask and firmware reflashing to enable or disable features.
2. THE Firmware SHALL remove all PlatformIO variant environments (variant_base, variant_timer, variant_rss, variant_timer_rss) from platformio.ini.
3. THE Firmware SHALL remove the `PLUGIN_TIMER_ENABLED` and `PLUGIN_RSS_ENABLED` compile-time flags and all conditional compilation guarded by those flags.
4. THE Firmware SHALL remove the `PluginManager::variantFilename` method and the associated OTA asset-name logic.
5. THE Firmware SHALL retain a single firmware binary that includes the Plugin_Loader and Plugin_Library but no plugin-specific functionality compiled in.
6. THE Firmware SHALL remove the `build-variants.yml` GitHub Actions workflow.

### Requirement 10: MonoRepo Build System

**User Story:** As a developer, I want plugin source code and CI in the same repository so that plugins are versioned, tested, and released alongside the firmware.

#### Acceptance Criteria

1. THE build system SHALL store plugin source code in the directory `plugins/{plugin_id}/` at the repository root.
2. THE build system SHALL include a PlatformIO build configuration or CMake script per plugin that compiles the plugin source into a position-independent Plugin_Binary.
3. THE build system SHALL include a CI workflow that builds all plugins on push or pull request and attaches the resulting Plugin_Binaries to GitHub Releases.
4. THE build system SHALL generate the `plugins-registry.json` file automatically from plugin manifests during the CI release process.
5. THE build system SHALL validate that each plugin compiles against the current Plugin_SDK version during CI.

### Requirement 11: SD Card Storage Layout

**User Story:** As a user, I want plugins stored in a predictable location on the SD card so that storage is organized and manageable.

#### Acceptance Criteria

1. THE Firmware SHALL store all plugin data under the SD_Card directory `/plugins/`.
2. THE Firmware SHALL store each plugin in its own subdirectory named by the plugin id: `/plugins/{plugin_id}/`.
3. THE Firmware SHALL store the compiled binary as `/plugins/{plugin_id}/plugin.bin`.
4. THE Firmware SHALL store the manifest as `/plugins/{plugin_id}/manifest.json`.
5. WHEN the StorageManager initializes, THE Firmware SHALL create the `/plugins/` directory if it does not exist.
6. THE Firmware SHALL allow plugins to create runtime data files within their own `/plugins/{plugin_id}/` directory only.

### Requirement 12: RSS Feed Plugin

**User Story:** As a user, I want an RSS feed plugin so that I can download and read articles from configured feeds on my e-reader.

#### Acceptance Criteria

1. THE RSS plugin SHALL implement all Plugin_SDK lifecycle functions (init, destroy, update, handle_button, handle_touch, draw).
2. THE RSS plugin SHALL provide a UI for configuring RSS feed URLs, stored in a configuration file within `/plugins/rss/`.
3. WHEN the user triggers a feed refresh, THE RSS plugin SHALL connect to WiFi and fetch articles from configured feed URLs.
4. THE RSS plugin SHALL parse RSS/Atom XML feeds and extract article titles and content.
5. THE RSS plugin SHALL store downloaded articles as text files on SD_Card within `/plugins/rss/articles/`.
6. THE RSS plugin SHALL display a list of downloaded articles and allow the user to read them using the display rendering Device_Services.
