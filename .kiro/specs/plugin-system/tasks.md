# Implementation Plan: Plugin System

## Overview

Replace the OTA variant-switching mechanism with a native plugin system for the ESP32-S3 e-reader. Plugins are position-independent C++ binaries loaded from SD card into PSRAM at runtime, executed in isolated FreeRTOS tasks. Implementation follows a strict dependency order: SDK headers → Loader → Device Services → Library → UI → FocusTimer extraction → RSS extraction → old system removal → build system + CI.

## Tasks

- [x] 1. Plugin SDK Headers
  - [x] 1.1 Create Plugin SDK core header and types
    - Create `firmware/src/plugins/sdk/PluginSdk.h` with all SDK types: `PluginBinaryHeader`, `PluginVTable`, `PluginContext`, `PluginResult`, `PluginButtonEvent`, `PluginTouchEvent`, `PluginInfo`, lifecycle function typedefs, and `PLUGIN_HEADER_MAGIC` / `PLUGIN_SDK_VERSION` constants
    - _Requirements: 1.1, 1.2, 1.4, 1.5, 2.2_

  - [x] 1.2 Create Device Services interface headers
    - Create `firmware/src/plugins/sdk/PluginDisplayService.h` with rendering function pointers (renderFocusTimerScreen, renderStatus, renderProgress, renderMenu, renderCenteredWord, setDarkMode, logicalWidth, logicalHeight)
    - Create `firmware/src/plugins/sdk/PluginAudioService.h` with beep and available function pointers
    - Create `firmware/src/plugins/sdk/PluginImuService.h` with readAccelerometer and available function pointers
    - Create `firmware/src/plugins/sdk/PluginStorageService.h` with sandboxed file operations (fileExists, readFile, writeFile, deleteFile, mkdir)
    - Create `firmware/src/plugins/sdk/PluginOrientationService.h` with currentOrientation and setUiOrientation function pointers
    - _Requirements: 1.3, 1.5, 1.6_

  - [x] 1.3 Create plugin linker script
    - Create `plugins/plugin.ld` with sections for `.plugin_header` at offset 0, `.text` (PIC code + rodata), `.plugin_vtable`, `.data`, `.bss`, and discard rules
    - _Requirements: 2.1, 2.3_

- [x] 2. Plugin Loader
  - [x] 2.1 Implement PluginLoader class
    - Create `firmware/src/plugins/PluginLoader.h` and `firmware/src/plugins/PluginLoader.cpp`
    - Implement state machine (Idle → Loading → Running → Error)
    - Implement `load()`: open SD file, check PSRAM capacity, allocate buffer with `heap_caps_malloc(MALLOC_CAP_SPIRAM | MALLOC_CAP_EXEC)`, read binary, validate header (magic + SDK version + size), resolve VTable at entryOffset, call `plugin_init` with context
    - Implement `unload()`: set exitRequested, wait for task exit, call `plugin_destroy`, free PSRAM, return to Idle
    - Implement `freePluginMemory()`, `validateHeader()`, `resolveVTable()`
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7_

  - [x] 2.2 Implement FreeRTOS plugin task and watchdog
    - Implement `pluginTaskEntry()` and `pluginTaskLoop()` — loop calling vtable->update and vtable->draw with 33ms yield
    - Create task pinned to Core 1 with configurable stack size (default 8192)
    - Implement `watchdogCheck()` — terminate task if elapsed > timeout (default 5000ms)
    - Implement `terminatePluginTask()` using `vTaskDelete`
    - Register ESP-IDF panic handler to detect plugin task crashes
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6_

  - [ ]\* 2.3 Write property tests for Plugin Loader validation
    - **Property 1: Binary Header Round-Trip** — serialize PluginBinaryHeader to bytes and parse back, verify identical struct
    - **Property 4: SDK Version Compatibility Check** — load accepts iff binary SDK version equals firmware SDK version
    - **Property 5: PSRAM Capacity Gate** — load accepts iff binary size <= available capacity
    - **Property 6: Watchdog Timeout Detection** — watchdog fires iff elapsed > timeout
    - **Validates: Requirements 2.2, 3.2, 3.3, 3.6, 4.4**

- [x] 3. Device Services Bridge
  - [x] 3.1 Implement Device Services bridge
    - Create `firmware/src/plugins/DeviceServicesBridge.h` and `firmware/src/plugins/DeviceServicesBridge.cpp`
    - Implement `setupDeviceServices()` that populates `PluginDisplayService` function pointers wrapping `DisplayManager` methods
    - Implement `PluginAudioService` wrappers around `AudioManager::beep()`
    - Implement `PluginImuService` wrappers around the QMI8658 accelerometer reads
    - Implement `PluginOrientationService` wrappers around `BoardConfig::UiOrientation`
    - Implement `PluginStorageService` with sandboxed path resolution — reject any path traversal attempts (`..`)
    - _Requirements: 1.3, 1.5, 1.6, 11.6_

  - [ ]\* 3.2 Write property test for file sandboxing
    - **Property 12: Plugin File Sandboxing** — write succeeds iff resolved path is strictly within `/plugins/{own_id}/`; path traversal (`../`) is rejected
    - **Validates: Requirements 11.6**

- [x] 4. Checkpoint - Ensure loader compiles and links
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Plugin Library (Online Store)
  - [x] 5.1 Implement PluginLibrary class
    - Create `firmware/src/plugins/PluginLibrary.h` and `firmware/src/plugins/PluginLibrary.cpp`
    - Implement `fetchRegistry()`: connect WiFi, HTTPS GET `plugins-registry.json` from GitHub release, parse JSON into `std::vector<RegistryEntry>`
    - Implement `downloadPlugin()`: download binary + manifest to `/plugins/{id}/`, show progress via callback, cleanup on failure
    - Implement `removePlugin()`: delete `/plugins/{id}/` directory recursively
    - Implement `scanInstalled()`: iterate `/plugins/` subdirs, parse each `manifest.json`
    - Implement `isUpdateAvailable()` using `compareVersions()`
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 5.8, 6.1, 6.2, 6.3, 6.4, 6.5, 6.6_

  - [ ]\* 5.2 Write property tests for Plugin Library
    - **Property 2: Plugin Path Construction** — for any valid plugin ID, paths match `/plugins/{id}/plugin.bin` and `/plugins/{id}/manifest.json`
    - **Property 3: Manifest Validation** — valid iff all required fields present with correct types
    - **Property 8: Semantic Version Comparison** — update available iff remote > local (major, minor, patch)
    - **Property 10: Registry Entry Validation** — valid iff all required fields present, root has `registry_version`
    - **Validates: Requirements 2.4, 2.5, 2.6, 5.8, 7.2, 7.3, 7.4, 11.1–11.4**

- [x] 6. UI Integration (App.cpp)
  - [x] 6.1 Replace PluginsList menu with dynamic plugin system
    - Modify `firmware/src/app/App.cpp` — replace `openPluginsList()` and `renderPluginsList()` to use `PluginLibrary::installed()` instead of `PluginManager` bitmask
    - Add "Plugin Library" menu item that opens the online store screen
    - Implement new `MenuScreen::PluginLibrary` for browsing/downloading plugins
    - Replace `runPluginInstall()` / `runPluginRemove()` to use `PluginLibrary::downloadPlugin()` / `removePlugin()`
    - Add "Launch" action for installed plugins that calls `PluginLoader::load()`
    - _Requirements: 5.4, 6.2, 6.4, 8.7_

  - [x] 6.2 Wire PluginLoader into App update loop
    - Add `PluginLoader pluginLoader_` member to `App`
    - Call `pluginLoader_.watchdogCheck(nowMs)` from `App::update()`
    - Forward button/touch events to running plugin when `pluginLoader_.isRunning()`
    - Handle plugin exit (user presses power button) → call `pluginLoader_.unload()` → return to menu
    - Handle plugin error/crash → display error screen → return to menu
    - _Requirements: 3.7, 4.4, 4.5, 4.6_

- [x] 7. Checkpoint - Ensure firmware compiles with Loader + Library + UI
  - Ensure all tests pass, ask the user if questions arise.

- [x] 8. FocusTimer Plugin Extraction
  - [x] 8.1 Create FocusTimer plugin project structure
    - Create `plugins/focus-timer/platformio.ini` with PIC build flags (`-fPIC`, `-fno-exceptions`, `-fno-rtti`, `-nostdlib`, `-DPLUGIN_BUILD=1`), custom linker script, and SDK include path
    - Create `plugins/focus-timer/manifest.json` with id, name, version, author, sdk_version, description, permissions
    - _Requirements: 2.1, 2.4, 2.5, 2.6, 10.1_

  - [x] 8.2 Extract FocusTimerCore from firmware
    - Create `plugins/focus-timer/src/FocusTimerCore.h` and `plugins/focus-timer/src/FocusTimerCore.cpp`
    - Move the 11-state state machine, genre selection, orientation detection (IMU classify), timer modes (touch/work/break), and completion logic from `firmware/src/timer/FocusTimer.h/.cpp`
    - Replace direct hardware access with Device_Services calls (PluginImuService, PluginAudioService, PluginDisplayService, PluginOrientationService)
    - _Requirements: 8.2, 8.3, 8.4, 8.5, 8.6_

  - [x] 8.3 Create FocusTimer plugin entry point
    - Create `plugins/focus-timer/src/main.cpp` with `extern "C"` block containing PluginBinaryHeader in `.plugin_header` section, PluginVTable in `.plugin_vtable` section
    - Implement `plugin_init` (allocate FocusTimerCore), `plugin_destroy` (free), `plugin_update`, `plugin_handle_button`, `plugin_handle_touch`, `plugin_draw`, `plugin_get_info`
    - _Requirements: 8.1, 8.7_

  - [ ]\* 8.4 Write property test for FocusTimer state machine equivalence
    - **Property 11: FocusTimer State Machine Equivalence** — for any sequence of events, original and extracted state machines produce identical transitions
    - **Validates: Requirements 8.2**

- [x] 9. RSS Feed Plugin Extraction
  - [x] 9.1 Create RSS plugin project structure
    - Create `plugins/rss/platformio.ini` with PIC build flags and SDK include path
    - Create `plugins/rss/manifest.json` with id, name, version, author, sdk_version, description, permissions
    - _Requirements: 2.1, 2.4, 2.5, 2.6, 10.1_

  - [x] 9.2 Implement RSS plugin core logic
    - Create `plugins/rss/src/RssPluginCore.h` and `plugins/rss/src/RssPluginCore.cpp`
    - Implement feed configuration UI (add/remove feed URLs stored in `/plugins/rss/config.json`)
    - Implement WiFi feed fetch + RSS/Atom XML parsing to extract article titles and content
    - Store articles as text files in `/plugins/rss/articles/`
    - Implement article list display and reading UI using PluginDisplayService
    - _Requirements: 12.2, 12.3, 12.4, 12.5, 12.6_

  - [x] 9.3 Create RSS plugin entry point
    - Create `plugins/rss/src/main.cpp` with plugin header, vtable, and lifecycle implementations (init, destroy, update, handle_button, handle_touch, draw, get_info)
    - _Requirements: 12.1_

- [x] 10. Checkpoint - Ensure both plugins compile as PIC binaries
  - Ensure all tests pass, ask the user if questions arise.

- [x] 11. Remove Old Variant System
  - [x] 11.1 Remove PluginManager and variant infrastructure
    - Delete `firmware/src/plugins/PluginManager.h`
    - Remove `#include "plugins/PluginManager.h"` from `App.h` and all referencing files
    - Remove `PluginManager pluginManager_` member from `App`
    - Remove all `pluginManager_` method calls (begin, isInstalled, maskAfterInstall, etc.)
    - Remove `PLUGIN_TIMER_ENABLED` and `PLUGIN_RSS_ENABLED` build flags from `platformio.ini` `[env]` section
    - Remove variant environments from `platformio.ini`: `[env:variant_base]`, `[env:variant_timer]`, `[env:variant_rss]`, `[env:variant_timer_rss]`
    - Remove all `#if PLUGIN_TIMER_ENABLED` / `#if PLUGIN_RSS_ENABLED` conditional compilation blocks throughout firmware
    - _Requirements: 9.1, 9.2, 9.3, 9.4, 9.5_

  - [x] 11.2 Remove built-in FocusTimer from firmware
    - Remove `firmware/src/timer/FocusTimer.h` and `firmware/src/timer/FocusTimer.cpp`
    - Remove `#include "timer/FocusTimer.h"` from `App.h`
    - Remove `FocusTimer focusTimer_` member and all direct FocusTimer method calls from App
    - Replace FocusTimer menu entries with plugin launch (handled by UI integration in task 6.1)
    - _Requirements: 9.5_

  - [x] 11.3 Remove built-in RSS code from firmware
    - Remove or refactor `firmware/src/rss/RssFeedManager.h/.cpp` — move relevant logic to RSS plugin
    - Remove `#include "rss/RssFeedManager.h"` from `App.h` if no longer needed
    - Remove `RssFeedManager rssFeedManager_` member from App if fully extracted
    - _Requirements: 9.5_

  - [x] 11.4 Ensure /plugins/ directory creation on boot
    - Modify `StorageManager` to create `/plugins/` directory on SD card if not present during initialization
    - _Requirements: 11.1, 11.5_

- [x] 12. Checkpoint - Firmware compiles as single binary without variants
  - Ensure all tests pass, ask the user if questions arise.

- [x] 13. Build System and CI
  - [x] 13.1 Create plugin build tooling
    - Create `tools/pio_plugin_build.py` — PlatformIO post-build script that patches PluginBinaryHeader in the output binary (fills `binarySize` and `entryOffset` fields from linker map)
    - Create `tools/generate_registry.py` — reads all `plugins/*/manifest.json` files and generates `plugins-registry.json` with download URLs based on GitHub release tag
    - _Requirements: 10.2, 10.4_

  - [x] 13.2 Replace build-variants.yml with plugin build workflow
    - Delete `.github/workflows/build-variants.yml`
    - Create new `.github/workflows/build-plugins.yml` that: builds each plugin in `plugins/*/` using PlatformIO, runs `generate_registry.py`, attaches `*-plugin.bin`, `*-manifest.json`, and `plugins-registry.json` to GitHub Release
    - Update `.github/workflows/release.yml` to call `build-plugins.yml` instead of `build-variants.yml`
    - _Requirements: 9.6, 10.3, 10.5_

  - [ ]\* 13.3 Write property test for registry generation
    - **Property 13: Registry Generation from Manifests** — for any set of valid manifest files, generated registry contains exactly one entry per manifest with matching fields
    - **Validates: Requirements 10.4**

- [x] 14. Final Checkpoint - Full build and all tests pass
  - Ensure all tests pass, ask the user if questions arise.

  - [ ]\* 14.1 Write property test for installed plugins persistence
    - **Property 9: Installed Plugins Persistence Round-Trip** — scanning `/plugins/` with valid manifests yields consistent list across reboots
    - **Validates: Requirements 6.6**

  - [ ]\* 14.2 Write property test for download failure cleanup
    - **Property 7: Download Failure Cleanup** — after any interrupted download, no partial files remain in plugin directory
    - **Validates: Requirements 5.7**

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation after major integration points
- Property tests validate universal correctness properties from the design document
- The implementation language is C++ (ESP32-S3 / PlatformIO / Arduino framework) with Python for build tooling
- Plugin binaries are compiled with Xtensa GCC using `-fPIC` — no standard library linked
- The old PluginManager.h bitmask + OTA variant system is completely replaced by SD-card-based plugin loading

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2", "1.3"] },
    { "id": 1, "tasks": ["2.1"] },
    { "id": 2, "tasks": ["2.2", "3.1"] },
    { "id": 3, "tasks": ["2.3", "3.2"] },
    { "id": 4, "tasks": ["5.1"] },
    { "id": 5, "tasks": ["5.2", "6.1"] },
    { "id": 6, "tasks": ["6.2"] },
    { "id": 7, "tasks": ["8.1", "9.1"] },
    { "id": 8, "tasks": ["8.2", "9.2"] },
    { "id": 9, "tasks": ["8.3", "8.4", "9.3"] },
    { "id": 10, "tasks": ["11.1"] },
    { "id": 11, "tasks": ["11.2", "11.3", "11.4"] },
    { "id": 12, "tasks": ["13.1"] },
    { "id": 13, "tasks": ["13.2", "13.3"] },
    { "id": 14, "tasks": ["14.1", "14.2"] }
  ]
}
```
