# Requirements Document

## Introduction

The Flower e-reader PWA currently displays all reading settings (pause behaviour, WPM, word delays) regardless of the active reading mode. These settings only apply to RSVP mode, leaving Scroll/Page mode with zero user-configurable options — it relies on hardcoded firmware constants for line height, font size, margins, and spacing.

This feature introduces **mode-contextual settings** in the PWA settings panel: when the user selects Scroll mode, RSVP-specific controls are hidden and replaced with scroll-specific controls (font size, line spacing, margins). The firmware will read these new NVS settings instead of using hardcoded constants, enabling users to personalize their scroll reading experience.

## Glossary

- **Settings_Panel**: The `<settings-panel>` Lit web component in the PWA that renders device settings and syncs changes to the device
- **Firmware**: The ESP32-S3 firmware running on the Flower e-reader device
- **NVS**: Non-Volatile Storage on the ESP32-S3, used to persist user settings as key-value pairs
- **Scroll_Mode**: The page-based reading mode where text is displayed in multi-line pages navigated by gestures (NVS key `read_mode` = 1)
- **RSVP_Mode**: Rapid Serial Visual Presentation reading mode where one word at a time is displayed at configurable speed (NVS key `read_mode` = 0)
- **Scroll_Font_Size**: A numeric setting (0–4) controlling the text size in Scroll_Mode, which determines how many lines fit per page
- **Scroll_Line_Spacing**: A numeric setting controlling vertical space between lines of text in Scroll_Mode
- **Scroll_Margin**: A numeric setting controlling horizontal padding on both sides of text in Scroll_Mode
- **CompanionSyncManager**: The firmware component that receives JSON settings from the PWA over WiFi/BLE and writes them to NVS
- **DeviceSettings**: The TypeScript interface in the PWA defining all settings fields synced with the device

## Requirements

### Requirement 1: Reading Mode Selector Prominence

**User Story:** As a reader, I want the reading mode selector to be the first and most prominent control in the settings panel, so that I can quickly switch between RSVP and Scroll modes before adjusting mode-specific settings.

#### Acceptance Criteria

1. THE Settings_Panel SHALL render the reading mode segmented control (RSVP / Przewijanie) as the first interactive element at the top of the settings panel, immediately below the brand header
2. THE Settings_Panel SHALL display the reading mode selector in its own visually distinct section, separate from other setting groups
3. WHEN the Settings_Panel is opened, THE reading mode selector SHALL be visible without scrolling on any standard mobile viewport (360px width or larger)
4. THE reading mode selector SHALL remain in its top position regardless of which mode is currently active

### Requirement 2: Mode-Contextual Settings Display

**User Story:** As a reader, I want the settings panel to show only settings relevant to my active reading mode, so that I am not confused by irrelevant controls.

#### Acceptance Criteria

1. WHILE Scroll_Mode is selected as the active reading mode, THE Settings_Panel SHALL hide the Pause behaviour, Base WPM, Long word delay, Complex word delay, and Punctuation delay controls
2. WHILE Scroll_Mode is selected as the active reading mode, THE Settings_Panel SHALL display Scroll_Font_Size, Scroll_Line_Spacing, and Scroll_Margin controls in a dedicated fieldset below the reading mode selector
3. WHILE RSVP_Mode is selected as the active reading mode, THE Settings_Panel SHALL display the Pause behaviour, Base WPM, Long word delay, Complex word delay, and Punctuation delay controls in a dedicated fieldset below the reading mode selector
4. WHILE RSVP_Mode is selected as the active reading mode, THE Settings_Panel SHALL hide the Scroll_Font_Size, Scroll_Line_Spacing, and Scroll_Margin controls
5. WHEN the user switches reading mode via the segmented control, THE Settings_Panel SHALL swap the visible settings group within 300 milliseconds without requiring a page reload
6. WHEN the Settings_Panel is opened, THE Settings_Panel SHALL display the settings group corresponding to the reading mode value currently stored on the connected device
7. IF the reading mode value received from the device is outside the valid range (not 0 and not 1), THEN THE Settings_Panel SHALL treat the value as RSVP_Mode (0) and display the RSVP settings group

### Requirement 3: Independent Settings Storage Per Mode

**User Story:** As a reader, I want the device to remember my RSVP settings and my Scroll settings independently, so that switching between modes never loses my personalized configuration for either mode.

#### Acceptance Criteria

1. THE Firmware SHALL store RSVP-specific settings (pause behaviour, base WPM, long word delay, complex word delay, punctuation delay) in their existing NVS keys independently from scroll mode settings
2. THE Firmware SHALL store scroll-specific settings (scroll_font_size, scroll_line_sp, scroll_margin) in their own dedicated NVS keys independently from RSVP mode settings
3. WHEN the user switches from Scroll_Mode to RSVP_Mode, THE Firmware SHALL apply the previously saved RSVP settings without modification
4. WHEN the user switches from RSVP_Mode to Scroll_Mode, THE Firmware SHALL apply the previously saved scroll settings without modification
5. WHEN the user changes an RSVP setting while in RSVP_Mode, THE change SHALL NOT affect any scroll mode setting values stored in NVS
6. WHEN the user changes a scroll setting while in Scroll_Mode, THE change SHALL NOT affect any RSVP setting values stored in NVS
7. WHEN the device restarts, THE Firmware SHALL load settings for both modes from NVS and apply the settings corresponding to the currently active reading mode

### Requirement 4: Scroll Mode Font Size Control

**User Story:** As a reader using scroll mode, I want to adjust the text size, so that I can find a comfortable reading size that balances readability with content density.

#### Acceptance Criteria

1. THE Settings_Panel SHALL provide a Scroll_Font_Size control with 5 discrete levels: Extra Small (0), Small (1), Medium (2), Large (3), Extra Large (4)
2. WHEN the user changes Scroll_Font_Size, THE Settings_Panel SHALL sync the new value to the device using NVS key `scroll_font_size` as a uint8_t
3. WHEN the Firmware receives a new `scroll_font_size` value, THE Firmware SHALL recalculate the scroll line height and font divisor to render text at the corresponding size
4. IF no `scroll_font_size` value is stored in NVS, THEN THE Firmware SHALL use a default Scroll_Font_Size of 2 (Medium)
5. WHEN Scroll_Font_Size is set to Extra Large (4), THE Firmware SHALL render no fewer than 2 lines of text per page
6. WHEN Scroll_Font_Size is set to Extra Small (0), THE Firmware SHALL render no more than 8 lines of text per page
7. WHEN Scroll_Font_Size is set to Small (1), THE Firmware SHALL render no more than 7 lines of text per page
8. WHEN Scroll_Font_Size is set to Medium (2), THE Firmware SHALL render no more than 6 lines of text per page
9. WHEN Scroll_Font_Size is set to Large (3), THE Firmware SHALL render no more than 4 lines of text per page
10. WHEN Scroll_Font_Size is changed, THE Firmware SHALL apply the new size on the next page render

### Requirement 5: Scroll Mode Line Spacing Control

**User Story:** As a reader using scroll mode, I want to adjust line spacing, so that I can improve readability by controlling how dense or airy the text layout feels.

#### Acceptance Criteria

1. THE Settings_Panel SHALL provide a Scroll_Line_Spacing control with 3 discrete levels: Compact (0), Normal (1), Relaxed (2)
2. WHEN the user changes Scroll_Line_Spacing, THE Settings_Panel SHALL sync the new value to the device using NVS key `scroll_line_sp` as a uint8_t
3. WHEN the Firmware receives a `scroll_line_sp` value, THE Firmware SHALL apply vertical spacing between lines using the following mapping: Compact (0) = 24 pixels, Normal (1) = 29 pixels, Relaxed (2) = 36 pixels
4. IF no `scroll_line_sp` value is stored in NVS, THEN THE Firmware SHALL use a default Scroll_Line_Spacing of 1 (Normal, 29 pixels)
5. WHEN Scroll_Line_Spacing is changed, THE Firmware SHALL reflow the current page to reflect the new spacing on the next page render
6. IF a `scroll_line_sp` value outside the range 0–2 is stored in NVS, THEN THE Firmware SHALL ignore it and use the default value of 1 (Normal)

### Requirement 6: Scroll Mode Margin Control

**User Story:** As a reader using scroll mode, I want to adjust horizontal margins, so that I can control line length for comfortable reading.

#### Acceptance Criteria

1. THE Settings_Panel SHALL provide a Scroll_Margin control with 3 discrete levels: Narrow (0), Normal (1), Wide (2)
2. WHEN the user changes Scroll_Margin, THE Settings_Panel SHALL sync the new value to the device using NVS key `scroll_margin` as a uint8_t
3. WHEN the Firmware receives a new `scroll_margin` value, THE Firmware SHALL apply horizontal padding on both sides of the text in scroll view using the following mapping: Narrow (0) = 8 pixels, Normal (1) = 18 pixels, Wide (2) = 32 pixels
4. IF no `scroll_margin` value is stored in NVS, THEN THE Firmware SHALL use a default Scroll_Margin of 1 (Normal, 18 pixels)
5. WHEN Scroll_Margin is changed, THE Firmware SHALL reflow the current page to reflect the new margin on the next page render
6. WHEN Scroll_Margin is set to any level, THE Firmware SHALL apply the configured margin equally to the left and right sides of the text area

### Requirement 7: Settings Persistence and Sync

**User Story:** As a reader, I want my scroll mode settings to be saved and synced reliably, so that they persist across device restarts and are always in sync with my PWA choices.

#### Acceptance Criteria

1. WHEN the user changes any scroll mode setting in the Settings_Panel, THE CompanionSyncManager SHALL write the new value to the corresponding NVS key on the device within 500 milliseconds of the user action
2. WHEN the device restarts, THE Firmware SHALL read `scroll_font_size`, `scroll_line_sp`, and `scroll_margin` from NVS and apply them to the scroll view renderer before the first page is displayed
3. IF a scroll mode NVS key contains a value outside its valid range (`scroll_font_size` > 4, `scroll_line_sp` > 2, `scroll_margin` > 2), THEN THE Firmware SHALL ignore the invalid value and use the default for that setting
4. WHEN the PWA connects to the device, THE Settings_Panel SHALL read current scroll mode settings from the device and display them accurately within 1 second of connection establishment
5. THE DeviceSettings interface SHALL include fields `scrollFontSize` (number, 0–4), `scrollLineSpacing` (number, 0–2), and `scrollMargin` (number, 0–2)
6. IF a CompanionSyncManager write to NVS fails, THEN THE Settings_Panel SHALL display an error indication and retain the previous setting value in the UI

### Requirement 8: Scroll Settings Live Preview Feedback

**User Story:** As a reader, I want to see a visual indication of my font size choice in the settings panel, so that I can understand the effect before returning to my book.

#### Acceptance Criteria

1. WHILE the Scroll_Font_Size control is displayed, THE Settings_Panel SHALL show a text label indicating the currently selected level name (Extra Small, Small, Medium, Large, Extra Large)
2. WHEN the user changes Scroll_Font_Size, THE Settings_Panel SHALL update the displayed level name label immediately, without waiting for device sync confirmation
3. WHILE the Scroll_Line_Spacing control is displayed, THE Settings_Panel SHALL show a text label indicating the currently selected level name (Compact, Normal, Relaxed)
4. WHEN the user changes Scroll_Line_Spacing, THE Settings_Panel SHALL update the displayed level name label immediately, without waiting for device sync confirmation
5. WHILE the Scroll_Margin control is displayed, THE Settings_Panel SHALL show a text label indicating the currently selected level name (Narrow, Normal, Wide)
6. WHEN the user changes Scroll_Margin, THE Settings_Panel SHALL update the displayed level name label immediately, without waiting for device sync confirmation
