# Dokument wymagań

## Wprowadzenie

Reorganizacja aplikacji wielostronicowej Vite wdrożonej na Netlify. Aplikacja PWA "Flower" (obecnie pod `/app/`) zostanie przeniesiona na root (`/`), a strona flashera firmware (obecnie na root) zostanie przeniesiona pod `/flash/`. Dodatkowo istniejący komponent instalacji PWA zostanie zastąpiony pełnoekranowym dialogiem instalacyjnym z poprawną obsługą `beforeinstallprompt`.

## Słownik

- **Aplikacja_Flower**: Progresywna aplikacja webowa (PWA) do sterowania czytnikiem e-booków, zarządzania biblioteką, konwersji plików i instalacji pluginów. Zbudowana w Lit (web components).
- **Flasher**: Samodzielna strona HTML do flashowania firmware ESP32-S3 przez USB za pomocą esp-web-tools. Nie jest częścią PWA.
- **Vite_Config**: Plik konfiguracyjny `vite.config.ts` definiujący punkty wejścia (rollupOptions.input), pluginy i ustawienia PWA.
- **Service_Worker**: Skrypt generowany przez vite-plugin-pwa (strategia generateSW) odpowiedzialny za cache'owanie zasobów i działanie offline.
- **Manifest_PWA**: Plik `manifest.webmanifest` opisujący metadane aplikacji PWA (nazwa, ikony, scope, start_url).
- **Dialog_Instalacyjny**: Pełnoekranowy komponent modalny zachęcający użytkownika do zainstalowania PWA na urządzeniu.
- **Tryb_Standalone**: Tryb wyświetlania aplikacji po zainstalowaniu jako PWA (display-mode: standalone).
- **Netlify_Config**: Plik `netlify.toml` definiujący konfigurację budowania i reguły routingu na platformie Netlify.

## Wymagania

### Wymaganie 1: Przeniesienie aplikacji PWA na root

**User Story:** Jako użytkownik, chcę aby aplikacja Flower była dostępna pod adresem root (`/`), aby mieć prostszy URL i lepsze doświadczenie instalacji PWA.

#### Kryteria akceptacji

1. WHEN użytkownik odwiedza adres root (`/`) THEN Aplikacja_Flower SHALL wyświetlić główny interfejs aplikacji PWA (element `czytnik-app`)
2. THE Vite_Config SHALL definiować punkt wejścia aplikacji jako `index.html` w katalogu głównym projektu
3. THE Manifest_PWA SHALL mieć `scope` ustawiony na `"/"`
4. THE Manifest_PWA SHALL mieć `start_url` ustawiony na `"/"`
5. THE Service_Worker SHALL mieć `navigateFallback` ustawiony na `"/index.html"`
6. THE plik konfiguracyjny `src/shared/config.ts` SHALL definiować `APP_URL` jako `"/"`
7. THE plik konfiguracyjny `src/shared/config.ts` SHALL definiować `FLASHER_URL` jako `"/flash/"`

### Wymaganie 2: Przeniesienie flashera pod /flash/

**User Story:** Jako użytkownik, chcę aby strona flashera firmware była dostępna pod `/flash/`, aby nie kolidowała z główną aplikacją PWA.

#### Kryteria akceptacji

1. WHEN użytkownik odwiedza adres `/flash/` THEN Flasher SHALL wyświetlić interfejs flashowania firmware (element `czytnik-flasher`)
2. THE Vite_Config SHALL definiować punkt wejścia flashera jako `flash/index.html`
3. THE Flasher SHALL działać jako samodzielna strona bez rejestracji Service_Worker
4. THE Flasher SHALL nie zawierać odniesienia do Manifest_PWA w kodzie HTML
5. THE plugin `stripPwaFromFlasher` w Vite_Config SHALL usuwać znaczniki PWA z pliku `flash/index.html` zamiast z root `index.html`

### Wymaganie 3: Pełnoekranowy dialog instalacyjny PWA

**User Story:** Jako użytkownik odwiedzający aplikację po raz pierwszy, chcę zobaczyć czytelny dialog instalacyjny, abym mógł łatwo zainstalować aplikację na swoim urządzeniu.

#### Kryteria akceptacji

1. WHEN użytkownik odwiedza aplikację po raz pierwszy AND aplikacja nie jest w Tryb_Standalone THEN Dialog_Instalacyjny SHALL wyświetlić się jako pełnoekranowa nakładka modalna
2. WHILE aplikacja jest w Tryb_Standalone THEN Dialog_Instalacyjny SHALL nie wyświetlać się
3. WHEN Dialog_Instalacyjny jest widoczny THEN Dialog_Instalacyjny SHALL wyświetlić ikonę aplikacji (kwiat SVG), nazwę "Flower" oraz komunikat "Czy chcesz pobrać aplikację Flower na swoje urządzenie?"
4. WHEN użytkownik kliknie przycisk "Zainstaluj" THEN Dialog_Instalacyjny SHALL wywołać natywny mechanizm `beforeinstallprompt`
5. WHEN użytkownik kliknie przycisk "Nie teraz" THEN Dialog_Instalacyjny SHALL zamknąć się i zapisać datę odrzucenia w localStorage
6. WHEN użytkownik odrzucił dialog w ciągu ostatnich 7 dni THEN Dialog_Instalacyjny SHALL nie wyświetlać się ponownie
7. WHEN użytkownik korzysta z iOS THEN Dialog_Instalacyjny SHALL wyświetlić instrukcje instalacji (Udostępnij → Dodaj do ekranu głównego) zamiast przycisku "Zainstaluj"
8. THE Dialog_Instalacyjny SHALL być stylizowany jako nakładka modalna z gradientem nieba, zaokrąglonymi kartami i akcentem niebieskim (#2e8eff), zgodnie z istniejącym systemem designu

### Wymaganie 4: Usunięcie starego komponentu instalacji

**User Story:** Jako deweloper, chcę usunąć stary komponent `czytnik-install-prompt` i powiązany kod, aby uniknąć duplikacji i konfliktów z nowym dialogiem instalacyjnym.

#### Kryteria akceptacji

1. THE plik `src/app/components/install-prompt.element.ts` SHALL zostać usunięty z projektu
2. THE `app.element.ts` SHALL nie importować modułu `install-prompt.element`
3. THE `app.element.ts` SHALL nie renderować elementu `<czytnik-install-prompt>`
4. THE `app.element.ts` SHALL nie zawierać metody `renderInstallBanner()`
5. THE `app.element.ts` SHALL nie zawierać metody `triggerInstall()`

### Wymaganie 5: Poprawne działanie PWA offline

**User Story:** Jako użytkownik, chcę aby aplikacja Flower działała offline po zainstalowaniu, abym mógł korzystać z niej bez połączenia z internetem.

#### Kryteria akceptacji

1. THE Manifest_PWA SHALL być dostępny pod ścieżką root (`/manifest.webmanifest`)
2. THE Service_Worker SHALL cache'ować zasoby aplikacji (JS, CSS, HTML, SVG, PNG) do użytku offline
3. WHEN aplikacja jest zainstalowana AND brak połączenia z internetem THEN Aplikacja_Flower SHALL wyświetlić interfejs użytkownika z cache'owanych zasobów
4. THE ikony aplikacji (`icon-192.png`, `icon-512.png`, `icon-maskable-512.png`) SHALL być dostępne z poziomu root (`/icons/`)
5. THE Service_Worker SHALL nie cache'ować plików firmware (`.bin`)

### Wymaganie 6: Aktualizacja konfiguracji Netlify

**User Story:** Jako deweloper, chcę aby konfiguracja Netlify poprawnie obsługiwała nowy routing, aby aplikacja działała prawidłowo po wdrożeniu.

#### Kryteria akceptacji

1. THE Netlify_Config SHALL kierować żądania do `/flash/` na stronę flashera (`/flash/index.html`)
2. THE Netlify_Config SHALL kierować żądania do `/flash/*` na stronę flashera (`/flash/index.html`)
3. THE Netlify_Config SHALL kierować wszystkie pozostałe żądania (poza statycznymi plikami) na `/index.html` (zachowanie SPA dla PWA)
4. IF żądanie dotyczy istniejącego pliku statycznego THEN Netlify_Config SHALL serwować ten plik bezpośrednio bez przekierowania
