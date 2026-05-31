# Implementation Plan: Reader PWA Split

## Overview

Rozdzielenie logiki instalacji PWA: usunięcie niedziałającego komponentu `czytnik-install-prompt`, zastąpienie go nowym `<pwa-install-dialog>` opartym na natywnym `beforeinstallprompt`, integracja z onboarding wizardem i konfiguracja Netlify dla SPA routing + cache headers.

## Tasks

- [x] 1. Remove old install-prompt component and references
  - [x] 1.1 Delete `src/app/components/install-prompt.element.ts`
    - Remove the entire file from the source tree
    - _Requirements: 4.1_

  - [x] 1.2 Remove all references from `src/app/app.element.ts`
    - Remove `import "./components/install-prompt.element"` import statement
    - Remove `<czytnik-install-prompt></czytnik-install-prompt>` from the `render()` method
    - Remove the `renderInstallBanner()` method entirely
    - Remove the `triggerInstall()` method entirely
    - Remove the `${!isStandalone ? this.renderInstallBanner() : ""}` call in `renderHome()`
    - Remove the `.install-card` CSS rule (no longer needed)
    - _Requirements: 4.2, 4.3, 4.4_

- [x] 2. Create new `<pwa-install-dialog>` component
  - [x] 2.1 Create `src/app/components/pwa-install-dialog.element.ts`
    - Implement `BeforeInstallPromptEvent` interface
    - Implement `PwaInstallDialog` Lit element with `@customElement("pwa-install-dialog")`
    - Add reactive state: `deferredPrompt`, `visible`, `isIos`, `isStandalone`
    - Implement `connectedCallback` with standalone detection, iOS detection, and event listeners for `beforeinstallprompt` and `appinstalled`
    - Implement `disconnectedCallback` to clean up event listeners and timers
    - Implement `scheduleShow()` with 30-second delay (`setTimeout`)
    - Implement `isDismissedRecently()` checking localStorage key `flower.installPrompt.dismissedAt` with 7-day TTL
    - Implement `handleInstallClick()` calling `prompt()` on deferred event
    - Implement `dismiss()` storing timestamp in localStorage and hiding dialog
    - Implement public `triggerInstall()` method returning `Promise<"accepted" | "dismissed" | "unavailable">`
    - Implement public `installAvailable` getter
    - Implement `render()` with modal backdrop, dialog card, close button, app name "Flower", install button (Chrome) or iOS Share Sheet instructions
    - Dispatch `pwa-install-available` custom event (bubbles, composed) when `beforeinstallprompt` is intercepted
    - Add CSS styles for modal backdrop, dialog, animation (fade-in within 300ms)
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.6_

  - [ ]\* 2.2 Write property test for standalone suppression
    - **Property 1: Standalone suppression**
    - **Validates: Requirements 3.4, 7.7**

  - [ ]\* 2.3 Write property test for cooldown enforcement
    - **Property 2: Cooldown enforcement**
    - **Validates: Requirements 3.3, 3.6**

  - [ ]\* 2.4 Write property test for delay guarantee
    - **Property 3: Delay guarantee**
    - **Validates: Requirements 3.1**

  - [ ]\* 2.5 Write property test for install prompt single-fire
    - **Property 7: Install prompt single-fire**
    - **Validates: Requirements 3.2**

- [x] 3. Integrate `<pwa-install-dialog>` into app.element.ts
  - [x] 3.1 Add import and tag to `src/app/app.element.ts`
    - Add `import "./components/pwa-install-dialog.element"` import statement
    - Add `<pwa-install-dialog></pwa-install-dialog>` in the `render()` method (replacing the old `<czytnik-install-prompt>` position)
    - _Requirements: 4.4, 3.1, 3.2_

- [x] 4. Checkpoint - Ensure build compiles and app renders
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 5. Update onboarding wizard with PWA install step
  - [-] 5.1 Modify `src/app/components/onboarding.element.ts` install step logic
    - Import `PwaInstallDialog` type from `./pwa-install-dialog.element`
    - Add `@state() private installAvailable = false` and `@state() private isStandalone = false`
    - In `connectedCallback`, detect standalone mode and listen for `pwa-install-available` event on `window`
    - Modify step 1 (`case 1`) to use intelligent logic:
      - If standalone → skip install step entirely (proceed to step 2)
      - If iOS → show numbered Share Sheet instructions (at least 2 steps)
      - If `beforeinstallprompt` available → show "Zainstaluj" button that calls `pwa-install-dialog.triggerInstall()`
      - If none of the above → skip install step
    - Add `handleOnboardingInstall()` method that queries `document.querySelector("pwa-install-dialog")` and calls `triggerInstall()`, then advances to next step regardless of outcome
    - Ensure wizard does NOT block if user dismisses native prompt (outcome "dismissed" → proceed)
    - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7_

  - [ ]\* 5.2 Write unit tests for onboarding wizard step-skipping logic
    - Test: standalone mode → install step skipped
    - Test: iOS platform → Share Sheet instructions shown
    - Test: no prompt available and not iOS → step skipped
    - Test: prompt available → "Zainstaluj" button rendered
    - _Requirements: 7.5, 7.7_

- [x] 6. Update Netlify configuration
  - [x] 6.1 Add redirects and headers to `netlify.toml`
    - Add `[[redirects]]` rule: `from = "/app/*"`, `to = "/app/index.html"`, `status = 200`, `force = false`
    - Add `[[headers]]` for `/app/sw.js` with `Cache-Control = "no-cache"`
    - Add `[[headers]]` for `/app/manifest.webmanifest` with `Cache-Control = "no-cache"`
    - Add `[[headers]]` for `/app/assets/*` with `Cache-Control = "public, max-age=31536000, immutable"`
    - _Requirements: 6.4, 6.5_

  - [ ]\* 6.2 Write property test for SPA routing
    - **Property 5: SPA routing**
    - **Validates: Requirements 6.4**

- [~] 7. Checkpoint - Verify build and old component removal
  - Ensure all tests pass, ask the user if questions arise.
  - Run `npx vite build` and verify zero TypeScript errors
  - Verify `dist/index.html` (Flasher) does NOT contain `<link rel="manifest">` or SW registration
  - Verify `dist/app/index.html` (Reader App) is generated correctly
  - _Requirements: 4.5, 6.1_

- [ ]\* 8. Write property test for old component removal
  - **Property 8: Old component removal**
  - Verify source tree does not contain `install-prompt.element.ts`, any import referencing it, any `<czytnik-install-prompt>` tag, or `triggerInstall()` querying that element
  - **Validates: Requirements 4.1, 4.2, 4.3, 4.4**

- [ ]\* 9. Write property test for Flasher isolation
  - **Property 4: Flasher isolation**
  - Verify built `dist/index.html` has no `<link rel="manifest">`, no SW registration script, no PWA module imports
  - **Validates: Requirements 2.2**

- [~] 10. Final checkpoint - Full verification
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document
- Unit tests validate specific examples and edge cases
- The project uses TypeScript with Lit, Vite with vite-plugin-pwa, deployed on Netlify
- `vite.config.ts` does NOT need changes — PWA config is already correct per design

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "1.2"] },
    { "id": 1, "tasks": ["2.1", "6.1"] },
    { "id": 2, "tasks": ["2.2", "2.3", "2.4", "2.5", "3.1"] },
    { "id": 3, "tasks": ["5.1"] },
    { "id": 4, "tasks": ["5.2", "6.2"] }
  ]
}
```
