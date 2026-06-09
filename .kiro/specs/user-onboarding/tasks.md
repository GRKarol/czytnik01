# Implementation Plan: User Onboarding

## Overview

Implementacja systemu onboardingu dla czytnika Flower PWA. Obejmuje: moduł i18n z 6 językami, onboarding-store (localStorage z prefixem `flower.onboarding.*`), tutorial wizard (5 ekranów edukacyjnych), tooltip system dla settings-panel, stronę pomocy (help-panel), first-use hint overlaye, oraz integrację z istniejącymi komponentami. Technologia: TypeScript + Lit Web Components.

## Tasks

- [x] 1. Infrastructure — onboarding-store and i18n module
  - [x] 1.1 Create onboarding-store module
    - Create `src/app/onboarding/onboarding-store.ts`
    - Implement `getTutorialStatus()`, `setTutorialStatus()`, `resetTutorial()`
    - Implement `isHintSeen()`, `markHintSeen()`, `isLocalStorageAvailable()`
    - All keys use `flower.onboarding.` prefix — never touch existing localStorage keys
    - Wrap all reads/writes in try/catch; corrupted values → treat as `"not_seen"`
    - If localStorage unavailable: tutorial → show (not_seen), hints → DON'T show (seen)
    - _Requirements: 1.4, 1.7, 1.8, 6.3, 6.5_

  - [ ]\* 1.2 Write property tests for onboarding-store (Properties 1, 2, 7, 8)
    - **Property 1: Tutorial state classification** — for any string in localStorage, `getTutorialStatus()` returns exactly one of: "completed", "skipped", or "not_seen"
    - **Property 2: Tutorial state persistence round-trip** — write then read returns written status; reset → "not_seen"
    - **Property 7: Hint visibility decision** — unseen key → false, seen key → true, no localStorage → true
    - **Property 8: Hint key isolation** — marking key A seen does not affect key B
    - Use fast-check with 100+ iterations, random strings for localStorage values, random screen keys
    - **Validates: Requirements 1.1, 1.4, 1.7, 1.8, 5.2, 5.3, 6.1, 6.2, 6.3, 6.5**

  - [x] 1.3 Create i18n module core
    - Create `src/app/i18n/index.ts` with exports: `t()`, `setLang()`, `getLang()`, `onLangChange()`
    - Type `SupportedLang = "en" | "es" | "fr" | "de" | "ro" | "pl"`
    - Implement flat key lookup from JSON locale files
    - Fallback chain: active locale → English → key string as-is
    - Invalid language code → default to Polish (app default)
    - `setLang()` emits `lang-changed` event on `document` for reactive updates
    - _Requirements: 7.1, 7.2, 7.3, 7.4_

  - [x] 1.4 Create English locale file with all onboarding keys
    - Create `src/app/i18n/locales/en.json`
    - Define all keys: `tutorial.*`, `tooltip.*`, `help.*`, `hint.*`
    - Flat structure with namespace prefixes
    - Ensure tutorial descriptions ≤ 2 sentences, each ≤ 120 chars
    - Ensure tooltip texts ≤ 200 chars combined
    - Ensure hint overlay texts ≤ 150 chars
    - _Requirements: 2.1, 3.2, 3.5, 6.1, 6.2, 7.1_

  - [ ]\* 1.5 Write property test for i18n module (Property 9)
    - **Property 9: i18n completeness and fallback** — for any onboarding key × any supported language, `t(key)` returns non-empty string; missing key in locale → English fallback; missing in English → key itself
    - Use fast-check, iterate all keys × all 6 languages
    - **Validates: Requirements 7.1, 7.2, 7.4**

- [x] 2. Checkpoint — Infrastructure tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 3. Tutorial wizard component
  - [x] 3.1 Create tutorial-wizard element
    - Create `src/app/components/tutorial-wizard.element.ts`
    - Implement `@customElement("tutorial-wizard")` extending LitElement
    - 5 screens: (1) RSVP mode, (2) WPM tempo, (3) Pause modes, (4) Theme & brightness, (5) HUD elements
    - Each screen: title, description (i18n keys), visual/icon
    - Navigation buttons: "Dalej" / "Wstecz" / "Pomiń" / "Zakończ" (last screen)
    - Progress indicator: "krok X z 5"
    - Hide "Wstecz" on first screen; replace "Dalej" with "Zakończ" on last screen
    - On complete: call `setTutorialStatus("completed")` and dispatch close event
    - On skip: call `setTutorialStatus("skipped")` and dispatch close event
    - Use CSS variables from app theme (jasny/ciemny/nocny)
    - Font size ≥ 16px, contrast ≥ 4.5:1 (WCAG AA)
    - No animations > 200ms
    - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.9, 8.2, 8.3, 8.4_

  - [x] 3.2 Implement swipe gesture navigation for tutorial
    - Add `touchstart`/`touchend` listeners with 50px threshold
    - Swipe left → next screen, swipe right → previous screen
    - Block swipe-right on first screen
    - Buttons remain functional as fallback if gesture detection fails
    - _Requirements: 2.7, 2.8_

  - [ ]\* 3.3 Write property test for tutorial navigation (Property 3)
    - **Property 3: Tutorial navigation state machine** — step in [0,4], next → step+1 (max 4), back → step-1 (min 0), back at 0 is no-op, progress always "krok {step+1} z 5"
    - Use fast-check with random action sequences [next, back, skip]
    - **Validates: Requirements 1.3, 1.6, 2.7, 2.8, 2.9**

  - [ ]\* 3.4 Write property test for tutorial content constraints (Property 4)
    - **Property 4: Tutorial content length constraints** — for any language × any tutorial screen, description ≤ 2 sentences, each sentence ≤ 120 chars
    - Iterate all 6 languages × 5 screens
    - **Validates: Requirements 2.1**

  - [ ]\* 3.5 Write unit tests for tutorial-wizard
    - Test correct screen order (RSVP → WPM → Pause → Theme → HUD)
    - Test "Pomiń" button triggers skip state
    - Test progress indicator format
    - Test theme CSS variable usage
    - Test font-size ≥ 16px
    - _Requirements: 2.2, 1.5, 1.6, 8.2, 8.3_

- [x] 4. Tooltip system
  - [x] 4.1 Create setting-tooltip element
    - Create `src/app/components/setting-tooltip.element.ts`
    - Implement `@customElement("setting-tooltip")` extending LitElement
    - Properties: `settingKey: string`
    - Render "?" icon; on click show popup with description, effect, default value
    - Max 200 chars total; truncate with "…" at 197 if exceeded
    - Singleton pattern: dispatch `tooltip-open` on `document`; listen and close self when another opens
    - Dismiss on click outside (≤ 150ms)
    - Display ≤ 200ms from tap
    - Use i18n keys for all tooltip text
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7_

  - [x] 4.2 Create tooltip data registry
    - Define `TooltipData` entries for all 26+ settings across categories:
      - Czytanie: Reading mode, Pause behaviour, Base speed, Long words, Complexity, Punctuation
      - Typografia: Font size, Typeface, Phantom words, Focus highlight, Tracking, Anchor, Guide width, Guide gap
      - Wyświetlanie: Theme, Brightness, Reader hand, Footer label, Battery label, Screensaver, Reading battery, Reading chapter, Reading percent, Focus color, Save btn
    - Each entry: settingKey, descriptionKey (i18n), effectKey (i18n), defaultValue
    - _Requirements: 3.3, 3.5_

  - [ ]\* 4.3 Write property test for tooltip data (Property 5)
    - **Property 5: Tooltip data completeness and constraints** — for each of 26+ settings, tooltip data exists with non-empty description, effect, default value; combined text ≤ 200 chars
    - **Validates: Requirements 3.2, 3.3, 3.5**

  - [ ]\* 4.4 Write unit tests for setting-tooltip
    - Test singleton behavior (only 1 tooltip visible at a time)
    - Test dismiss on outside click
    - Test truncation at 197 chars + "…"
    - _Requirements: 3.4, 3.6, 3.7_

- [x] 5. Help page panel
  - [x] 5.1 Create help-panel element
    - Create `src/app/components/help-panel.element.ts`
    - Implement `@customElement("help-panel")` extending LitElement
    - Sections: "Szybki start" at top (connect device, send book, start reading)
    - Categories: Wyświetlanie, Czytanie, HUD, Język, Połączenie
    - Accordion: expanding one item collapses previous in same category (max 1 expanded per category)
    - "Uruchom tutorial ponownie" button → calls `resetTutorial()` then dispatches event to show tutorial
    - Back button at top to return to settings-panel
    - All text via i18n keys
    - Works fully offline (static data in bundle, no network requests)
    - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5, 4.6, 4.7, 5.1, 5.2_

  - [x] 5.2 Create help page data (categories and items)
    - Define `HelpCategory[]` data structure with all categories and items
    - Each item: nameKey, descKey, valuesKey (all i18n keys)
    - Ensure data covers all configurable features
    - _Requirements: 4.2, 4.3_

  - [ ]\* 5.3 Write property test for accordion logic (Property 6)
    - **Property 6: Help page accordion invariant** — for any category and any sequence of expansions, at most 1 item is expanded at any time
    - Use fast-check with random toggle sequences
    - **Validates: Requirements 4.7**

  - [ ]\* 5.4 Write unit tests for help-panel
    - Test "Szybki start" section renders at top
    - Test back button presence
    - Test offline rendering (no fetch calls)
    - Test "Uruchom tutorial ponownie" resets state
    - _Requirements: 4.4, 4.5, 4.6, 5.1, 5.2_

- [x] 6. Checkpoint — Core components tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 7. First-use hints
  - [x] 7.1 Create first-use-hint element
    - Create `src/app/components/first-use-hint.element.ts`
    - Implement `@customElement("first-use-hint")` extending LitElement
    - Property: `screenKey: string` ("reading" | "converter")
    - On `connectedCallback`: check `onboarding-store.isHintSeen(screenKey)`
    - If not seen and localStorage available → show overlay blocking interactions (pointer-events on backdrop)
    - If localStorage unavailable → DON'T show hint, don't block
    - Close button: min 44×44px touch area
    - On close: call `markHintSeen(screenKey)`, remove overlay, unblock in ≤ 300ms
    - Max 150 chars text via i18n key
    - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 6.6_

  - [x] 7.2 Define hint content for reading screen
    - i18n key: `hint.reading.text`
    - Content: explain basic controls (pause, tempo change, return to menu)
    - Ensure ≤ 150 chars in all 6 languages
    - _Requirements: 6.1_

  - [x] 7.3 Define hint content for converter screen
    - i18n key: `hint.converter.text`
    - Content: explain supported formats (EPUB, PDF, TXT, MD, HTML) and .rsvp conversion
    - Ensure ≤ 150 chars in all 6 languages
    - _Requirements: 6.2_

  - [ ]\* 7.4 Write unit tests for first-use-hint
    - Test close button ≥ 44×44px
    - Test overlay blocks interaction (pointer-events)
    - Test unblock timing ≤ 300ms
    - Test no hint shown when localStorage unavailable
    - _Requirements: 6.4, 6.5, 6.6_

- [x] 8. Integration — Connect to existing panels
  - [x] 8.1 Integrate tutorial-wizard into czytnik-app
    - In the main app component, after `onboarding-wizard` dismissal (device connected):
      - Check `getTutorialStatus()` → if "not_seen" → render `<tutorial-wizard>`
    - On tutorial close event → remove tutorial-wizard from DOM
    - Ensure no device commands sent during tutorial display
    - _Requirements: 1.1, 8.1_

  - [x] 8.2 Integrate setting-tooltip into settings-panel
    - Add `<setting-tooltip>` next to each supported setting in settings-panel
    - Pass appropriate `settingKey` to each tooltip instance
    - _Requirements: 3.1_

  - [x] 8.3 Integrate help-panel into settings navigation
    - Add "Pomoc / Przewodnik" menu item in settings-panel
    - Navigation to help-panel and back button wiring
    - _Requirements: 4.1, 4.6_

  - [x] 8.4 Integrate first-use-hint into reading and converter screens
    - Wrap reading screen with `<first-use-hint screen-key="reading">`
    - Wrap converter screen with `<first-use-hint screen-key="converter">`
    - _Requirements: 6.1, 6.2_

  - [x] 8.5 Wire i18n language to device ui_lang setting
    - Map firmware `ui_lang` index (0–5) to SupportedLang
    - On language change in settings-panel → call `setLang()`
    - Ensure all visible onboarding components re-render ≤ 500ms
    - _Requirements: 7.1, 7.2, 7.3_

- [x] 9. Translation content — All 6 languages
  - [x] 9.1 Create Polish locale file
    - Create `src/app/i18n/locales/pl.json`
    - Translate all onboarding keys from English
    - Respect constraints: tutorial desc ≤ 2 sentences × 120 chars, tooltip ≤ 200 chars, hint ≤ 150 chars
    - _Requirements: 7.1, 7.2_

  - [x] 9.2 Create Spanish locale file
    - Create `src/app/i18n/locales/es.json`
    - Translate all onboarding keys from English
    - Respect same constraints
    - _Requirements: 7.1, 7.2_

  - [x] 9.3 Create French locale file
    - Create `src/app/i18n/locales/fr.json`
    - Translate all onboarding keys from English
    - Respect same constraints
    - _Requirements: 7.1, 7.2_

  - [x] 9.4 Create German locale file
    - Create `src/app/i18n/locales/de.json`
    - Translate all onboarding keys from English
    - Respect same constraints
    - _Requirements: 7.1, 7.2_

  - [x] 9.5 Create Romanian locale file
    - Create `src/app/i18n/locales/ro.json`
    - Translate all onboarding keys from English
    - Respect same constraints
    - _Requirements: 7.1, 7.2_

  - [ ]\* 9.6 Write integration tests for multi-language onboarding
    - Test language switch triggers re-render of all visible onboarding components
    - Test full tutorial flow: open → navigate → complete → verify localStorage state
    - Test no device commands during onboarding
    - _Requirements: 7.3, 8.1_

- [x] 10. Final checkpoint — All tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- All localStorage keys use `flower.onboarding.*` prefix — existing reading progress data is never touched
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document
- Unit tests validate specific examples and edge cases
- The app uses TypeScript + Lit Web Components; no external UI frameworks
- English locale (1.4) must be created first as it serves as the fallback language
- Translation tasks (9.x) can be parallelized once English keys are defined

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.3"] },
    { "id": 1, "tasks": ["1.2", "1.4", "1.5"] },
    { "id": 2, "tasks": ["3.1", "4.1", "4.2", "5.1", "5.2", "7.1"] },
    {
      "id": 3,
      "tasks": ["3.2", "3.3", "3.4", "3.5", "4.3", "4.4", "5.3", "5.4", "7.2", "7.3", "7.4"]
    },
    { "id": 4, "tasks": ["8.1", "8.2", "8.3", "8.4", "8.5"] },
    { "id": 5, "tasks": ["9.1", "9.2", "9.3", "9.4", "9.5"] },
    { "id": 6, "tasks": ["9.6"] }
  ]
}
```
