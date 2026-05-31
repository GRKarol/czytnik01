# Requirements Document

## Introduction

Rozdzielenie obecnej aplikacji na dwie niezależne strony:

1. **Aplikacja PWA „Flower"** (`/app/`) — strona do łączenia się z czytnikiem (WiFi / Bluetooth / USB), zarządzania biblioteką, konwerterem i pluginami. Ma działać jako pełnoprawna Progressive Web App z natywnym promptem instalacyjnym „Czy chcesz pobrać aplikację Flower na swoje urządzenie?".
2. **Web Flasher** (`/`) — osobna strona wyłącznie do flashowania firmware. Nie jest PWA, nie ma promptu instalacyjnego.

Obecny komponent pobierania/instalacji aplikacji (`czytnik-install-prompt` + `renderInstallBanner`) nie działa poprawnie — zostanie usunięty i zastąpiony natywnym mechanizmem PWA z automatycznym promptem `beforeinstallprompt`.

Projekt jest deployowany na Netlify.

## Glossary

- **PWA**: Progressive Web App — aplikacja webowa instalowalna na urządzeniu mobilnym/desktopowym, działająca offline dzięki Service Worker.
- **Flasher**: Strona webowa do wgrywania firmware na urządzenie ESP32-S3 przez Web Serial.
- **Reader_App**: Aplikacja klienta (`/app/`) do komunikacji z czytnikiem Flower przez WiFi, Bluetooth lub USB.
- **Install_Prompt**: Natywny mechanizm przeglądarki (`beforeinstallprompt`) pozwalający zainstalować PWA na urządzeniu.
- **Service_Worker**: Skrypt działający w tle, umożliwiający cache'owanie zasobów i pracę offline.
- **Netlify**: Platforma hostingowa, na której deployowany jest projekt.
- **Onboarding_Wizard**: Pełnoekranowy wizard pierwszego uruchomienia w Reader_App.

## Requirements

### Requirement 1: Separacja stron — Reader App jako samodzielna PWA

**User Story:** Jako użytkownik czytnika Flower, chcę aby strona do łączenia się z czytnikiem (`/app/`) była pełnoprawną aplikacją webową (PWA), którą mogę pobrać na telefon, tak abym mógł korzystać z niej jak z natywnej aplikacji.

#### Acceptance Criteria

1. WHEN a user navigates to `/app/`, THE Reader_App SHALL serve a Progressive Web App with a Web App Manifest containing at minimum: `name`, `short_name`, `start_url`, `display` set to `standalone`, `icons` (at least one 192×192 px and one 512×512 px), and `scope` set to `/app/`.
2. THE Reader_App SHALL register a Service Worker that caches all application assets (HTML, CSS, JavaScript, icons, SVG) for offline use, excluding firmware binary files (`.bin`).
3. WHEN the Reader_App is launched from the home screen, THE Reader_App SHALL display in standalone mode without browser chrome (address bar, tabs).
4. THE Reader_App SHALL include reader connection functionality via Web Serial (USB) transport, with selection and connection UI.
5. THE Reader_App SHALL NOT include any firmware flashing functionality.
6. IF the user's browser does not support Service Worker registration or the PWA install prompt, THEN THE Reader_App SHALL still function as a standard web application accessible via the browser.
7. WHEN the Reader_App is launched while the device has no network connectivity, THE Reader_App SHALL load the cached application shell and display the connection UI within 3 seconds.

### Requirement 2: Separacja stron — Flasher jako osobna strona

**User Story:** Jako serwisant/developer, chcę aby strona do flashowania firmware (`/`) była osobną stroną niezależną od PWA, tak abym mógł wgrywać firmware bez instalowania aplikacji.

#### Acceptance Criteria

1. WHEN a user navigates to `/`, THE Flasher SHALL render an `esp-web-install-button` component pointing to the firmware manifest, with a visible button allowing the user to initiate firmware installation.
2. THE Flasher SHALL NOT register a Service Worker and SHALL NOT include a `<link rel="manifest">` tag in the page source.
3. THE Flasher SHALL NOT include any reader connection functionality (WiFi configuration, Bluetooth pairing, or USB serial pairing with the Reader_App).
4. THE Flasher SHALL contain a visible, clickable hyperlink element directing users to the Reader_App at `/app/`.
5. IF the user's browser does not support the Web Serial API, THEN THE Flasher SHALL display a text message informing the user that Chrome or Edge on desktop is required.

### Requirement 3: Natywny prompt instalacyjny PWA

**User Story:** Jako użytkownik, chcę aby aplikacja sama zapytała mnie „Czy chcesz pobrać aplikację Flower na swoje urządzenie?", tak abym mógł ją łatwo zainstalować jednym kliknięciem.

#### Acceptance Criteria

1. WHEN the browser fires the `beforeinstallprompt` event and the user has been on the page for at least 30 seconds, THE Reader_App SHALL intercept the event and display a dismissible modal dialog containing the app name "Flower", an install button, and a close button.
2. WHEN the user clicks the install button in the install dialog, THE Reader_App SHALL trigger the native browser installation flow by calling `prompt()` on the stored `beforeinstallprompt` event.
3. WHEN the user dismisses the install dialog by clicking the close button, THE Reader_App SHALL hide the dialog within 300ms and store a suppression timestamp in localStorage so that the dialog is not shown again for 7 days.
4. WHILE the Reader_App is already running in standalone mode (detected via `display-mode: standalone` media query), THE Reader_App SHALL NOT display the install dialog.
5. IF the user is on iOS Safari (where `beforeinstallprompt` is not supported), THEN THE Reader_App SHALL display step-by-step instructions with numbered steps explaining how to install via the Share Sheet (Add to Home Screen), including a visual indicator of the Share button location.
6. IF the suppression timestamp stored in localStorage is older than 7 days, THEN THE Reader_App SHALL allow the install dialog to be displayed again on the next qualifying page visit.

### Requirement 4: Usunięcie niedziałającego komponentu instalacji

**User Story:** Jako developer, chcę usunąć obecny niedziałający komponent pobierania aplikacji, tak aby zastąpić go poprawnie działającym mechanizmem PWA.

#### Acceptance Criteria

1. THE Reader_App SHALL NOT contain the `czytnik-install-prompt` custom element definition file (`install-prompt.element.ts`) in the source tree.
2. THE Reader_App SHALL NOT contain any import statement referencing the `install-prompt.element` module, any `<czytnik-install-prompt>` tag in templates, or the `renderInstallBanner()` method.
3. THE Reader_App SHALL NOT contain the `triggerInstall()` method or any code that queries the `czytnik-install-prompt` element at runtime.
4. THE Reader_App SHALL delegate all install-prompt functionality to the unified install mechanism defined in Requirement 3, with no residual install logic in the main application component.
5. WHEN the Reader_App is built after removal of the old install component, THE Reader_App SHALL compile with zero TypeScript errors and render the home view without runtime exceptions in the browser console.

### Requirement 5: Konfiguracja PWA Manifest

**User Story:** Jako użytkownik, chcę aby zainstalowana aplikacja wyglądała profesjonalnie z poprawną nazwą i ikoną, tak abym łatwo ją rozpoznał na ekranie głównym.

#### Acceptance Criteria

1. THE Reader_App SHALL provide a Web App Manifest with `name` set to "Flower" and `short_name` set to "Flower".
2. THE Reader_App SHALL provide a Web App Manifest with `display` set to "standalone" and `orientation` set to "portrait".
3. THE Reader_App SHALL provide a Web App Manifest with PNG icons in sizes 192x192 (purpose: "any") and 512x512 (purpose: "any"), and an additional 512x512 PNG icon with purpose set to "maskable".
4. THE Reader_App SHALL provide a Web App Manifest with `start_url` pointing to `/app/` and `scope` set to `/app/`.
5. THE Reader_App SHALL provide a Web App Manifest with `theme_color` set to "#2e8eff" and `background_color` set to "#e8f4fd".
6. THE Reader_App SHALL provide a Web App Manifest with `lang` set to "pl".
7. THE Reader_App SHALL link the Web App Manifest file via a `<link rel="manifest">` element in the `<head>` of `/app/index.html`.

### Requirement 6: Deployment na Netlify

**User Story:** Jako developer, chcę aby obie strony (PWA i Flasher) były poprawnie deployowane na Netlify, tak aby użytkownicy mogli do nich dotrzeć przez odpowiednie URL-e.

#### Acceptance Criteria

1. WHEN the build command (`npx vite build`) is executed, THE build system SHALL produce a `dist/` directory containing both `dist/index.html` (Flasher) and `dist/app/index.html` (Reader_App).
2. THE Netlify configuration SHALL specify `dist` as the publish directory and `npx vite build` as the build command.
3. WHEN a user accesses the Netlify domain root (`/`), THE Netlify server SHALL serve the Flasher page (`index.html`).
4. WHEN a user accesses any path under `/app/` on the Netlify domain (including deep links such as `/app/library` or `/app/reader`), THE Netlify server SHALL serve `app/index.html` to enable client-side routing within the Reader_App PWA.
5. THE Netlify configuration SHALL include a `Cache-Control: no-cache` header for the `/sw.js` path so that browsers always revalidate the Service Worker file on each navigation.
6. WHEN a user accesses a path that does not match `/app/*` and does not correspond to an existing static file in `dist/`, THE Netlify server SHALL return an HTTP 404 response.

### Requirement 7: Onboarding Wizard z promptem instalacyjnym

**User Story:** Jako nowy użytkownik, chcę aby wizard pierwszego uruchomienia zawierał krok z instalacją PWA, tak abym od razu wiedział jak dodać aplikację do ekranu głównego.

#### Acceptance Criteria

1. WHEN the Reader_App is loaded AND no onboarding-completed flag exists in localStorage AND the app is not running in standalone mode, THE Onboarding_Wizard SHALL display as a full-screen overlay that includes a dedicated PWA installation step with the app icon, app name, and a step-by-step prompt guiding the user to install the app on their device.
2. IF the `beforeinstallprompt` event has been intercepted (Android/Chrome), THEN THE Onboarding_Wizard SHALL display a button labeled "Zainstaluj" that, when activated, triggers the native browser install prompt via the stored `beforeinstallprompt` event.
3. IF the user dismisses the native install prompt triggered from the Onboarding_Wizard (user choice outcome is "dismissed"), THEN THE Onboarding_Wizard SHALL proceed to the next step or allow the user to continue without blocking.
4. IF the user is on iOS (navigator.standalone is undefined and user agent indicates iOS), THEN THE Onboarding_Wizard SHALL display a step-by-step visual guide showing the Share icon and "Add to Home Screen" option with at least 2 numbered instruction steps.
5. IF the `beforeinstallprompt` event is not available AND the platform is not iOS, THEN THE Onboarding_Wizard SHALL skip the installation step and proceed to the next onboarding step.
6. WHEN the user completes the final step or activates a "Pomiń" (skip) control on any step of the Onboarding_Wizard, THE Onboarding_Wizard SHALL store an onboarding-completed flag in localStorage and SHALL not appear again on subsequent app loads.
7. WHILE the Reader_App is running in standalone mode, THE Onboarding_Wizard SHALL NOT display the PWA installation step even if the onboarding-completed flag is absent.
