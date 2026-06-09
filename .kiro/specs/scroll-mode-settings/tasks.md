# Implementation Plan: Scroll Mode Settings (Firmware + PWA)

## Overview

Add configurable scroll mode settings (font size, line spacing, margins) to the Flower firmware and PWA. Settings are stored in NVS, synced from the PWA via CompanionSyncManager, and applied by DisplayManager when rendering the scroll view. The PWA already has the UI changes implemented.

## Tasks

- [x] 1. PWA changes (completed)
  - [x] 1.1 DeviceSettings interface, label maps, settings panel restructure — all done

- [ ] 2. Add NVS keys and sync support in CompanionSyncManager
  - [ ] 2.1 Add scroll setting NVS keys, JSON output, and JSON parsing
    - In `firmware/src/sync/CompanionSyncManager.cpp`, add NVS key constants: `kPrefScrollFontSize = "sc_font"`, `kPrefScrollLineSpacing = "sc_line_sp"`, `kPrefScrollMargin = "sc_margin"` and validation constants: `kMaxScrollFontSize = 4`, `kDefaultScrollFontSize = 2`, `kMaxScrollLineSpacing = 2`, `kDefaultScrollLineSpacing = 1`, `kMaxScrollMargin = 2`, `kDefaultScrollMargin = 1`
    - In `settingsJson()`: add scroll settings to the JSON output in a new `"scroll"` section
    - In `applySettingsJson()`: parse `scrollFontSize` (0-4), `scrollLineSpacing` (0-2), `scrollMargin` (0-2) from incoming JSON and write to NVS
    - _Requirements: 4.2, 5.2, 6.2, 7.1, 7.3_

- [ ] 3. Make DisplayManager use configurable scroll parameters
  - [ ] 3.1 Add scroll configuration to DisplayManager and replace hardcoded constants
    - In `firmware/src/display/DisplayManager.h`: add `void setScrollFontSize(uint8_t)`, `void setScrollLineSpacing(uint8_t)`, `void setScrollMargin(uint8_t)` public methods and private members `scrollFontSize_`, `scrollLineSpacing_`, `scrollMargin_`
    - In `firmware/src/display/DisplayManager.cpp`: add private helpers `scrollLineHeightPx()`, `scrollMarginPx()`, `scrollSerifDivisor()` with lookup tables
    - In `renderScrollView()`: replace `kScrollMarginX` with `scrollMarginPx()`, `kScrollLineHeight` with `scrollLineHeightPx()`, `kScrollSerifDivisor` with `scrollSerifDivisor()`
    - _Requirements: 4.3, 4.5, 4.6, 5.3, 6.3, 6.6_

- [ ] 4. Load scroll settings from NVS in App and apply to DisplayManager
  - [ ] 4.1 Read scroll NVS keys in App::begin() and applyDisplayPreferences()
    - In `firmware/src/app/App.h`: add `uint8_t scrollFontSize_`, `scrollLineSpacing_`, `scrollMargin_` members
    - In `firmware/src/app/App.cpp`: add NVS key constants, read settings in `begin()`, call `display_.setScrollFontSize/LineSpacing/Margin()` in `applyDisplayPreferences()`
    - _Requirements: 7.2, 3.3, 3.4, 4.4, 5.4, 6.4_

- [ ] 5. Build and upload firmware
  - [ ] 5.1 Compile firmware with PlatformIO and flash to device via USB
    - Run `pio run -e waveshare_esp32s3_usb_msc` to compile
    - Fix any compilation errors
    - Run `pio run -e waveshare_esp32s3_usb_msc -t upload` to flash
    - _Requirements: all_

## Notes

- PWA changes already implemented (DeviceSettings, settings-panel, mock API)
- Firmware uses NVS namespace "rsvp" for all Preferences
- Upload command: `pio run -e waveshare_esp32s3_usb_msc -t upload`
- Device must be connected via USB cable for upload
- NO git push — local testing only

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["2.1"] },
    { "id": 1, "tasks": ["3.1"] },
    { "id": 2, "tasks": ["4.1"] },
    { "id": 3, "tasks": ["5.1"] }
  ]
}
```
