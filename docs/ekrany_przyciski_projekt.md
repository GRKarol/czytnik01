# Ekrany i przyciski do projektowania (Wymagane + Dodatkowe)

Rozszerzenie `ekrany_i_przyciski.md`. Dla każdego ekranu firmware (640x172px) i panelu PWA:
- **WYMAGANE** — przyciski, które już istnieją w kodzie (`App.cpp`) i muszą się znaleźć na ekranie, inaczej stracisz działającą funkcję.
- **DODATKOWE (do rozważenia)** — przyciski, których w kodzie nie ma, ale pasują do ekranu i warto je rozważyć przy okazji przechodzenia na pełnoekranowy UI z widocznymi przyciskami (zamiast ukrytych stref/gestów). Traktuj je jako propozycje, nie gotową specyfikację — zaznacz w narzędziu tylko te, które chcesz.

Legenda: [W] = warunkowy (pojawia się tylko w pewnych trybach/ustawieniach), [DEV] = tylko tryb developera.

---

## Ekrany czytnika

### Boot
WYMAGANE: brak (czysta animacja).
DODATKOWE: „Pomiń" — gdyby splash miał trwać dłużej niż teraz.

### Czytnik RSVP (główny widok)
WYMAGANE:
- „<<" poprzednie zdanie
- „SP" zapisz punkt [W]
- Odznaka baterii (tap = cykl etykiety)
- Metryka stopki (tap = cykl etykiety) [W]
- Obszar środkowy (słowo RSVP) — nie jest przyciskiem, zostaw pusty w projekcie

DODATKOWE:
- „>>" następne zdanie — w kodzie jest tylko cofanie, brak symetrycznego skoku do przodu; warto dodać.
- Explicit Play/Pauza — dziś to gest (przytrzymanie), na ekranie z widocznymi przyciskami warto mieć ikonę, żeby nie trzeba było zgadywać gestu.
- WPM +/− jako małe strzałki przy metryce — dziś tylko pionowy swipe.
- Ikona „Menu" w rogu — dziś menu otwiera się wyłącznie fizycznym PWR; przydatne jako zapasowa ścieżka dotykowa.
- Pasek postępu książki (scrub bar) tapalny — szybki skok w dowolne miejsce bez swipe'a.
- Ikona „Spis treści" — skrót do ChapterPicker bez wchodzenia przez Main→Biblioteka→Szczegóły.

### Czytnik — Scroll
WYMAGANE: te same co RSVP (poprzednie zdanie, SP, bateria, metryka stopki).
DODATKOWE: strzałki scroll góra/dół jawne (dziś tylko gest), plus te same propozycje co w RSVP (>>, play/pauza, menu, scrub bar).

### Podgląd kontekstu
WYMAGANE: brak (tap poza obszarem zamyka).
DODATKOWE: jawny „X" zamknij w rogu — łatwiej trafić dotykiem niż „gdziekolwiek poza".

### Nakładka WPM / Przejście rozdziału / Ostrzeżenie o baterii
WYMAGANE: brak, znikają same.
DODATKOWE: nie projektuj jako osobne ekrany z przyciskami — to overlaye, zostaw bez interakcji.

---

## Menu i ustawienia

### Main
WYMAGANE: „>> Update" [W], Czytaj, Biblioteka, Punkty zapisu, Ustawienia, Pluginy, Wyłącz.
DODATKOWE:
- Kafelek „Ostatnio czytane" na górze (od razu tytuł + procent, bez wchodzenia do Biblioteki) — duży skrót do najczęstszej akcji.
- Ikona zegara/baterii w headerze zamiast tylko w czytniku.
- Skrót „Pomoc" bezpośrednio w Main (dziś trzeba iść Ustawienia → O aplikacji).

### SettingsHome
WYMAGANE: Wróć, Czytanie, Wyświetlanie, Typografia, Połączenia, Presety, O aplikacji/Pomoc, Wi-Fi zaawansowane [DEV], Aktualizacja firmware [DEV].
DODATKOWE: „Przywróć ustawienia domyślne" (reset całościowy, dziś jest tylko reset per-sekcja w Pacing/Typography), wyszukiwarka ustawień (jeśli lista urośnie).

### SettingsDisplay
WYMAGANE: Wróć + 14 pozycji (Motyw, Jasność, Ręka czytająca, Etykieta stopki, Etykieta baterii, Wygaszacz, Bateria/Rozdział/Procent podczas czytania, Język, Kolor litery, Przycisk SP, Pomoc, Nawigacja).
DODATKOWE: przycisk „Podgląd" na żywo (jak w SettingsPacing dla trybu Scroll) — dziś zmiany widać dopiero po wyjściu z ustawień.

### SettingsPacing
WYMAGANE: Wróć, Tryb czytania, + (RSVP: Zachowanie pauzy, Tempo bazowe, Długie słowa, Złożoność, Interpunkcja, Reset) lub (Scroll: Rozmiar czcionki, Interlinia, Marginesy, Podgląd).
DODATKOWE: „Podgląd" także dla trybu RSVP (dziś mają go tylko ustawienia Scroll) — duża niespójność wartą naprawienia przy okazji redesignu. „Zapisz jako preset" bezpośrednio stąd, bez przechodzenia do Presets.

### SettingsConnectivity
WYMAGANE: Wróć, Wi-Fi, Bluetooth [W], Synchronizacja z telefonem, Kanały RSS (→ PluginsList), Kopiuj przez USB [W].
DODATKOWE: ikona/wskaźnik siły sygnału przy pozycji Wi-Fi, „Zapomnij sparowany telefon" osobno od Wi-Fi.

### SettingsAbout
WYMAGANE: Wróć, Wersja (10x tap = tryb dev), Etykieta marki (info), Sprawdzenie karty SD, Samouczek, Tryb developera WŁ [DEV].
DODATKOWE: pojemność/wolne miejsce na karcie SD widoczne wprost (bez wchodzenia w diagnostykę), eksport logów diagnostycznych do pliku na SD.

### ScreensaverSettings
WYMAGANE: Wróć, Styl (6 opcji), Czas do wygaszacza, Auto-wyłączenie zasilania, Ochrona snu, Podgląd.
DODATKOWE: intensywność/prędkość animacji, wybór koloru akcentu wygaszacza (spójnie z kolorem litery ORP).

### WifiSettings
WYMAGANE: Wróć, Sieć: <SSID>, Wybierz sieć, Zapomnij sieć, Auto OTA [DEV], OTA Owner [DEV].
DODATKOWE: „Testuj połączenie" (ping/status), ręczne dodanie ukrytej sieci (SSID + hasło wpisane ręcznie).

### WifiNetworks
WYMAGANE: Wróć, lista sieci (tap → hasło lub od razu zapis).
DODATKOWE: widoczny przycisk „Skanuj ponownie" (dziś skanowanie jest niejawne przy wejściu na ekran).

### TextEntry (klawiatura)
WYMAGANE: pole wartości, 3 rzędy liter, abc/ABC/123, space, back, clear/show-hide, save, cancel.
DODATKOWE: „Wklej" (gdy dane przychodzą z telefonu przez sync), podpowiedzi/historia ostatnich nazw przy zapisywaniu punktów/presetów.

### TypographyTuning
WYMAGANE: Wróć, Rozmiar czcionki, Krój pisma, Słowa widma, Podświetlenie fokusowe, Tracking, Pozycja kotwicy, Szerokość prowadnicy, Przerwa prowadnicy, Reset.
DODATKOWE — WAŻNE: dziś nawigacja między pozycjami to wyłącznie pionowy swipe, a zmiana próbki to poziomy swipe. Przy przejściu na UI z widocznymi przyciskami warto dodać jawne strzałki „▲/▼" (zmiana ustawienia) i „◀/▶" (zmiana słowa-próbki) — inaczej ten ekran zostanie jedynym bez żadnego widocznego przycisku nawigacji, co kłóci się z resztą projektu.

---

## Biblioteka i czytanie

### BookPicker
WYMAGANE: Wróć, lista książek (tap → BookDetails).
DODATKOWE: filtr Wszystko/Książki/Artykuły, sortowanie (Ostatnio dodane/Tytuł/Postęp), gwiazdka ulubione, miniatura-okładka z inicjałem — to wszystko już masz gotowe w PWA (`library-panel.element.ts`), warto przenieść ten sam układ na urządzenie dla spójności.

### BookDetails
WYMAGANE: Wróć, Tytuł+autor (info), Procent (info), Czytaj od miejsca, Rozdziały, Od nowa, Usuń książkę.
DODATKOWE: rozmiar pliku i data dodania (info), przycisk ulubione (spójnie z PWA).

### BookDeleteConfirm
WYMAGANE: Wróć, Nie/wróć, Tak/usuń.
DODATKOWE: brak — ekran potwierdzenia powinien zostać maksymalnie prosty.

### ChapterPicker
WYMAGANE: Wróć, lista rozdziałów (gwiazdka przy aktualnym), Od nowa.
DODATKOWE: szybkie wyszukiwanie/skok po numerze rozdziału (przydatne przy bardzo długich książkach).

### SavePointsList
WYMAGANE: Wróć, „+ Dodaj punkt zapisu", nazwa punktu (tap = wczytaj), „Usuń: <nazwa>".
DODATKOWE — WAŻNE: dziś usuwanie punktu zapisu działa natychmiast, bez potwierdzenia (jedyne miejsce w aplikacji z takim zachowaniem — wszędzie indziej usuwanie ma ekran Confirm). Warto dodać krok potwierdzenia albo przynajmniej „cofnij" (toast 3s), żeby nie zgubić przypadkiem ważnego zapisu.

### PluginsList
WYMAGANE: Wróć, zainstalowane pluginy (tap = uruchom), separator (info), „Biblioteka".
DODATKOWE: numer dostępnej aktualizacji przy zainstalowanym pluginie, „Ustawienia pluginu" bez uruchamiania go w pełni.

### PluginLibraryScreen
WYMAGANE: Wróć, lista pluginów online (Zainstaluj/Aktualizuj/Zainstalowany).
DODATKOWE: wyszukiwarka/filtr kategorii, jeśli lista urośnie.

### PluginDetail
WYMAGANE: Wróć, opis (info), Zainstaluj/Aktualizuj.
DODATKOWE: „Odinstaluj" (dziś trzeba wracać do ekranu z pluginem, żeby to zrobić inaczej), link/QR do repo pluginu.

### RestartConfirm / SdCardRepairConfirm / UpdateConfirm
WYMAGANE: nagłówek info + dwie opcje (Nie/Tak odpowiednio).
DODATKOWE: brak. UpdateConfirm to dziś martwy ekran (nic go nie otwiera) — zaprojektuj go tylko jeśli planujesz przywrócić ten flow.

---

## Kreator pierwszego uruchomienia i samouczek

### WelcomeLanguage / WelcomeTheme / WelcomeHighlightColor / WelcomePacing
WYMAGANE: lista opcji do wyboru (tap = wybierz i przejdź dalej).
DODATKOWE — WAŻNE: dziś kreator nie ma przycisku „Wstecz" między krokami 2-5 (tylko krok 1 nie ma wstecz, bo jest pierwszy — ale 2, 3, 4 też go nie mają, mimo że mogłyby). Warto dodać strzałkę wstecz + kropki postępu 1-5 na każdym kroku, żeby użytkownik mógł poprawić wcześniejszy wybór bez restartu urządzenia.

### WelcomeConnect
WYMAGANE: Wi-Fi/IP (info), separator (info), „Połącz z telefonem", „Pomiń na razie".
DODATKOWE: przycisk wstecz do WelcomePacing.

### TutorialStep1-5
WYMAGANE: cały ekran jako przycisk „dalej", licznik N/5.
DODATKOWE: „Pomiń samouczek" widoczny od razu (dziś trzeba przejść wszystkie 5 kroków, żeby się go pozbyć), strzałka wstecz do poprzedniego kroku.

---

## Presety

### Presets
WYMAGANE: Wróć, „+ Zapisz obecne" (lub limit), lista presetów (tap → PresetsDeleteConfirm).
DODATKOWE: sortowanie (data/nazwa), krótki podgląd parametrów presetu na liście (np. „WPM 320, RSVP") bez wchodzenia w szczegóły.

### PresetsDeleteConfirm
WYMAGANE: Wróć, „Zastosuj: <nazwa>", „Usuń: <nazwa>".
DODATKOWE: „Zmień nazwę".

---

## Stany bez interakcji

### Standby / Sleeping
WYMAGANE: brak — to celowe, nie dodawaj przycisków (wybudzenie tylko fizyczną kombinacją BOOT+PWR).

### CompanionSync
WYMAGANE: nagłówek „< Wróć | Wi-Fi" jako pełnoekranowy przycisk wyjścia, kod QR/status (info).
DODATKOWE: „Odśwież QR" jeśli kod wygasa po czasie.

### UsbTransfer
WYMAGANE: nagłówek „USB | Tap = wróć" jako pełnoekranowy przycisk wyjścia.
DODATKOWE: brak — status techniczny, nie rozbudowuj.

---

## Panele PWA (telefon)

### app.element.ts (powłoka)
WYMAGANE: wybór transportu (WiFi/Bluetooth/USB-Serial), po połączeniu — siatka kafelków (Książki/Konwerter/Pluginy/Aktualizacje) + „Rozłącz".
DODATKOWE: status siły sygnału / jakości połączenia widoczny stale w headerze.

### library-panel.element.ts (Książki)
WYMAGANE: „Wyślij plik", „Odśwież", filtry Wszystko/Książki/Artykuły, sortowanie, gwiazdka ulubione, usuń.
DODATKOWE: wyszukiwarka tekstowa po tytule/autorze, wybór wielu plików naraz (batch upload), batch delete.

### converter-panel.element.ts (Konwerter)
WYMAGANE: drag&drop/„Wybierz plik", edycja tytułu/autora, statystyki, „Pobierz .rsvp", „Wyślij na urządzenie" [W].
DODATKOWE: historia ostatnio skonwertowanych plików, przycisk „Konwertuj kolejny" bez przeładowania widoku.

### settings-panel.element.ts (Ustawienia)
WYMAGANE: grupy jak w dokumencie źródłowym (Tryb czytania, RSVP, Typografia, Scroll, Wyświetlanie, HUD, Język, Developer [W]), „Pomoc/Przewodnik".
DODATKOWE: „Zapisz jako preset" bezpośrednio z panelu ustawień PWA (dziś presety tworzy się tylko na urządzeniu).

### help-panel.element.ts
WYMAGANE: Wstecz, Szybki start (3 kroki), akordeon kategorii, restart samouczka.
DODATKOWE: wyszukiwarka w treści pomocy.

### tutorial-wizard.element.ts / onboarding.element.ts
WYMAGANE: Wstecz (poza pierwszym krokiem), Pomiń, Dalej/Zakończ, kropki postępu.
DODATKOWE: brak — te ekrany już mają komplet nawigacji, w przeciwieństwie do wersji na urządzeniu (patrz uwaga w sekcji Welcome*).

### pwa-install-dialog.element.ts / setting-tooltip.element.ts / first-use-hint.element.ts
WYMAGANE: przycisk zamknij/zrozumiano.
DODATKOWE: brak.

---

## Podsumowanie: gdzie jest największa luka spójności

1. **TypographyTuning** — jedyny ekran menu bez żadnego widocznego przycisku nawigacji (tylko gesty). Priorytet #1 przy projektowaniu w narzędziu.
2. **Kreator Welcome*** — brak przycisku wstecz na krokach 2-4, niespójnie z resztą aplikacji (nawet TutorialStep i tutorial-wizard w PWA mają wstecz/pomiń).
3. **SavePointsList** — usuwanie bez potwierdzenia, jedyne takie miejsce w aplikacji.
4. **SettingsPacing** — „Podgląd" istnieje tylko dla trybu Scroll, nie dla RSVP.
5. **Czytnik RSVP/Scroll** — brak jawnego „>>" (tylko „<<"), asymetria warta poprawy przy okazji dodawania widocznych przycisków.
