# Bugfix Requirements Document

## Introduction

Dwa krytyczne błędy w firmware czytnika ESP32-S3 uniemożliwiają normalne użytkowanie urządzenia:

1. **Reset stanu przy starcie (Autosave failure)** — Urządzenie nie zapamiętuje pozycji czytania. Po każdym włączeniu pokazuje ekran wyboru języka (welcome wizard), zamiast wznowić czytanie od ostatniego miejsca.

2. **Zamrożenie UI na ekranie Wi-Fi (WelcomeConnect)** — Krytyczny błąd logiczny w sekwencji ekranów startowych. Próba opuszczenia ekranu konfiguracji Wi-Fi (WelcomeConnect) przez przycisk power przekierowuje do menu głównego bez oznaczenia wizarda jako ukończonego, a elementy informacyjne na ekranie (indeksy 0–2) nie reagują na dotyk — użytkownik nie ma intuicyjnej drogi wyjścia bez twardego restartu.

## Bug Analysis

### Current Behavior (Defect)

1.1 WHEN the user completes the welcome wizard (WelcomeLanguage → WelcomeTheme → WelcomeConnect) and exits the WelcomeConnect screen via the power button THEN the system navigates to Main menu without calling `finishWelcomeWizard()`, leaving `kPrefSetupDone == false` in NVS

1.2 WHEN the device is rebooted after the user exited WelcomeConnect via power button THEN the system shows the welcome wizard again (WelcomeLanguage screen) because `kPrefSetupDone` was never persisted as `true`

1.3 WHEN the user is on the WelcomeConnect screen and taps the info rows (Wi-Fi name, IP address, separator — indices 0–2) THEN the system does nothing (no visual feedback, no navigation), making the touch layer appear completely unresponsive

1.4 WHEN the user presses the power button on WelcomeConnect and then selects "Resume" from Main menu THEN the system transitions to Paused state but `kPrefSetupDone` remains false, causing the welcome wizard to reappear on next boot regardless of reading progress

1.5 WHEN `maybeSaveReadingPosition()` is called while `state_ != AppState::Playing` (e.g. in Paused state) THEN the system skips saving the reading position, meaning position is only saved during active playback or explicit state transitions (Playing→Paused, enterStandby, enterPowerOff)

### Expected Behavior (Correct)

2.1 WHEN the user presses the power button on the WelcomeConnect screen THEN the system SHALL call `finishWelcomeWizard()` (persisting `kPrefSetupDone = true`) before navigating away, ensuring the wizard is marked as complete

2.2 WHEN the device is rebooted after the welcome wizard has been completed (either via "Connect"/"Skip" buttons or via power button exit from WelcomeConnect) THEN the system SHALL skip the welcome wizard and restore the last saved book and reading position

2.3 WHEN the user is on the WelcomeConnect screen THEN the system SHALL provide a clear, tappable "Skip" or "Back" action that is reachable via touch gesture (not only via hardware power button), preventing the perception of a frozen UI

2.4 WHEN the user exits the WelcomeConnect screen by any means (power button, tap on Skip/Connect, or swipe gesture) THEN the system SHALL ensure `kPrefSetupDone` is persisted as `true` and the device transitions to the reader (Paused state) without requiring a reboot

2.5 WHEN the user pauses reading (state == Paused) and the device subsequently enters standby, sleep, or power-off THEN the system SHALL have already saved the reading position (either via periodic autosave during Playing, or via forced save on the Playing→Paused transition), ensuring no progress is lost

### Unchanged Behavior (Regression Prevention)

3.1 WHEN the device boots for the very first time (fresh NVS, `kPrefSetupDone` never set) THEN the system SHALL CONTINUE TO show the welcome wizard (WelcomeLanguage → WelcomeTheme → WelcomeConnect)

3.2 WHEN the user selects "Connect" or "Skip" on the WelcomeConnect screen (indices 3 or 4) THEN the system SHALL CONTINUE TO call `finishWelcomeWizard()` and transition to the reader as before

3.3 WHEN the user is actively reading (state == Playing) and `kProgressSaveIntervalMs` (15s) elapses THEN the system SHALL CONTINUE TO periodically autosave the reading position via `maybeSaveReadingPosition()`

3.4 WHEN the user transitions from Playing to Paused (via touch release or sentence-end pause) THEN the system SHALL CONTINUE TO force-save the reading position immediately

3.5 WHEN the user presses the power button while in the Main menu THEN the system SHALL CONTINUE TO transition to Paused state (reader view)

3.6 WHEN the user presses the power button while in a non-welcome settings screen (SettingsHome, SettingsDisplay, etc.) THEN the system SHALL CONTINUE TO navigate to Main menu without altering `kPrefSetupDone`

3.7 WHEN `enterStandby()`, `enterPowerOff()`, or `enterSleep()` is called while state == Playing THEN the system SHALL CONTINUE TO force-save the reading position before state transition

---

## Bug Condition (Formal)

### Bug Condition Function

```pascal
FUNCTION isBugCondition(X)
  INPUT: X of type BootOrNavigationEvent
  OUTPUT: boolean

  // Bug 1+2: User exits WelcomeConnect via power button (not via Connect/Skip)
  RETURN (X.currentScreen = WelcomeConnect AND X.exitMethod = PowerButton)
END FUNCTION
```

### Property: Fix Checking

```pascal
// Property: Fix Checking — WelcomeConnect exit always persists setup_done
FOR ALL X WHERE isBugCondition(X) DO
  result ← handlePowerButtonExit'(X)
  ASSERT preferences.getBool("setup_done") = true
    AND result.nextState IN {Paused, Playing}
    AND result.menuScreen = Main (only transiently, before state change)
END FOR
```

### Property: Preservation Checking

```pascal
// Property: Preservation Checking — Non-buggy exits unchanged
FOR ALL X WHERE NOT isBugCondition(X) DO
  ASSERT handleNavigation(X) = handleNavigation'(X)
END FOR
```

This ensures that for all non-buggy navigation events (normal wizard completion, settings navigation, reader state transitions), the fixed code behaves identically to the original.
