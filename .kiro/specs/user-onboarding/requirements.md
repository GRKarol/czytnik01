# Requirements Document

## Introduction

Czytnik Flower posiada wiele zaawansowanych funkcji i ustawień (ponad 26 parametrów konfiguracyjnych, tryby RSVP/scroll, typografię, HUD itp.), których nazwy i przeznaczenie nie są oczywiste dla nowych użytkowników. Problem ujawnił się podczas testów z zewnętrznym testerem — osoba nie rozumiała co robią poszczególne funkcje, jak z nich korzystać i po co istnieją. System onboardingu ma rozwiązać ten problem poprzez edukację użytkownika przy pierwszym uruchomieniu oraz dostęp do pomocy kontekstowej w trakcie używania aplikacji.

## Glossary

- **System_Onboardingu**: Moduł aplikacji PWA odpowiedzialny za edukację nowego użytkownika i udostępnianie pomocy kontekstowej
- **Tutorial_Powitalny**: Interaktywny przewodnik uruchamiany przy pierwszym otwarciu aplikacji po połączeniu z urządzeniem
- **Tooltip_Kontekstowy**: Krótki komunikat wyjaśniający przeznaczenie danej funkcji lub ustawienia, wyświetlany w kontekście danego elementu UI
- **Strona_Pomocy**: Ekran z pełnym opisem wszystkich funkcji i ustawień, dostępny z poziomu menu ustawień
- **Użytkownik**: Osoba korzystająca z aplikacji PWA czytnika Flower
- **Panel_Ustawień**: Ekran ustawień aplikacji PWA (settings-panel)
- **Tryb_RSVP**: Rapid Serial Visual Presentation — główny tryb czytania, wyświetlający jedno słowo na raz
- **ORP**: Optimal Recognition Point — środkowa litera słowa podświetlona kolorem fokusowym
- **Słowa_Widma**: Przyciemnione słowa kontekstu wyświetlane przed i po bieżącym słowie w trybie RSVP

## Requirements

### Wymaganie 1: Tutorial powitalny przy pierwszym połączeniu

**User Story:** Jako nowy użytkownik, chcę zobaczyć interaktywny tutorial po pierwszym połączeniu z urządzeniem, abym zrozumiał podstawowe funkcje czytnika i mógł z nich efektywnie korzystać.

#### Kryteria Akceptacji

1. WHEN Użytkownik łączy się z urządzeniem i w localStorage nie istnieje zapis o ukończeniu ani pominięciu Tutorial_Powitalny, THE System_Onboardingu SHALL wyświetlić Tutorial_Powitalny składający się z sekwencji 5 ekranów edukacyjnych
2. THE Tutorial_Powitalny SHALL prezentować kluczowe funkcje czytnika w następującej kolejności: tryb czytania (RSVP vs scroll), sterowanie pauzą, regulacja tempa (WPM), zmiana motywu i jasności
3. THE Tutorial_Powitalny SHALL na każdym ekranie zawierać przycisk "Dalej" umożliwiający przejście do następnego ekranu, a na ostatnim ekranie przycisk "Zakończ" zamykający tutorial
4. WHEN Użytkownik ukończy lub pominie Tutorial_Powitalny, THE System_Onboardingu SHALL zapisać stan (ukończony lub pominięty) w localStorage, aby tutorial nie wyświetlał się ponownie przy kolejnych połączeniach
5. THE Tutorial_Powitalny SHALL zawierać przycisk "Pomiń" widoczny na każdym ekranie, umożliwiający zamknięcie tutorialu bez konieczności przeglądania pozostałych ekranów
6. THE Tutorial_Powitalny SHALL wyświetlać wskaźnik postępu w formacie "krok X z 5", gdzie X oznacza numer bieżącego ekranu
7. WHEN Użytkownik pominie Tutorial_Powitalny, THE System_Onboardingu SHALL oznaczyć tutorial jako pominięty (oddzielnie od stanu ukończony), umożliwiając ponowne uruchomienie tutorialu z poziomu panelu ustawień
8. IF zapis stanu Tutorial_Powitalny w localStorage jest niedostępny lub uszkodzony, THEN THE System_Onboardingu SHALL traktować tutorial jako niewyświetlony i wyświetlić go ponownie

### Wymaganie 2: Ekrany edukacyjne tutorialu

**User Story:** Jako nowy użytkownik, chcę aby każdy ekran tutorialu jasno wyjaśniał jedną funkcję z wizualizacją i krótkim opisem, abym zapamiętał przeznaczenie danej funkcji.

#### Kryteria Akceptacji

1. THE Tutorial_Powitalny SHALL prezentować każdą funkcję na osobnym ekranie zawierającym: nazwę funkcji, opis składający się z maksymalnie 2 zdań (każde zdanie nie dłuższe niż 120 znaków), oraz wizualizację lub ikonę przedstawiającą efekt działania
2. THE Tutorial_Powitalny SHALL prezentować ekrany edukacyjne w następującej kolejności: (1) tryb RSVP, (2) sterowanie tempem WPM, (3) tryby pauzy, (4) elementy HUD — łącznie 4 ekrany edukacyjne
3. THE Tutorial_Powitalny SHALL wyjaśniać tryb RSVP prezentując: nazwę „RSVP", opis mechanizmu wyświetlania słowo-po-słowie, informację o podświetlonej literze fokusowej (ORP) jako punkcie skupienia wzroku, oraz wizualizację przedstawiającą pojedyncze słowo z wyróżnioną literą środkową
4. THE Tutorial_Powitalny SHALL wyjaśniać sterowanie tempem czytania prezentując: nazwę „Tempo (WPM)", informację o zakresie prędkości od 50 do 1000 słów na minutę, oraz sposób zmiany tempa w ustawieniach aplikacji
5. THE Tutorial_Powitalny SHALL wyjaśniać tryby pauzy prezentując: nazwę „Pauza", opis trzech trybów (tap — pauza krótkim dotknięciem zatrzymująca na końcu zdania, przytrzymanie — pauza długim dotknięciem zatrzymująca na końcu zdania, auto — pauza natychmiastowa w momencie dotyku), oraz wizualizację różnicującą te tryby
6. THE Tutorial_Powitalny SHALL wyjaśniać elementy HUD prezentując: nazwę „HUD", listę trzech konfigurowalnych elementów widocznych podczas czytania (bateria, rozdział, procent postępu), oraz informację że każdy element można niezależnie włączyć lub wyłączyć w ustawieniach
7. WHEN Użytkownik przechodzi między ekranami tutorialu, THE System_Onboardingu SHALL umożliwiać nawigację do przodu i do tyłu za pomocą przycisków „Dalej" i „Wstecz" oraz gestów przesunięcia w lewo i w prawo
8. IF Użytkownik znajduje się na pierwszym ekranie edukacyjnym, THEN THE System_Onboardingu SHALL ukryć przycisk „Wstecz" oraz zablokować gest przesunięcia w prawo
9. IF Użytkownik znajduje się na ostatnim ekranie edukacyjnym, THEN THE System_Onboardingu SHALL zastąpić przycisk „Dalej" przyciskiem „Zakończ" umożliwiającym przejście do następnego kroku onboardingu

### Wymaganie 3: Tooltips kontekstowe w panelu ustawień

**User Story:** Jako użytkownik przeglądający ustawienia, chcę widzieć krótkie wyjaśnienia przy każdym ustawieniu, abym rozumiał co zmieniam bez konieczności szukania pomocy zewnętrznej.

#### Kryteria Akceptacji

1. THE Panel_Ustawień SHALL wyświetlać ikonę pomocy (ikona znaku zapytania) obok każdego ustawienia, które posiada przypisany Tooltip_Kontekstowy
2. WHEN Użytkownik dotknie ikonę pomocy, THE System_Onboardingu SHALL wyświetlić Tooltip_Kontekstowy z opisem danego ustawienia w czasie nie dłuższym niż 200 milisekund od dotknięcia, o maksymalnej długości 200 znaków
3. THE Tooltip_Kontekstowy SHALL zawierać: opis funkcji danego ustawienia, efekt zmiany wartości, oraz wartość domyślną jako sugerowaną wartość początkową
4. WHEN Użytkownik dotknie obszar poza Tooltip_Kontekstowy, THE System_Onboardingu SHALL ukryć tooltip w czasie nie dłuższym niż 150 milisekund
5. THE System_Onboardingu SHALL zapewnić Tooltip_Kontekstowy dla wszystkich ustawień z kategorii: Czytanie (Reading mode, Pause behaviour, Base speed, Long words, Complexity, Punctuation), Typografia (Font size, Typeface, Phantom words, Focus highlight, Tracking, Anchor, Guide width, Guide gap) i Wyświetlanie (Theme, Brightness, Reader hand, Footer label, Battery label, Screensaver, Reading battery, Reading chapter, Reading percent, Focus color, Save btn)
6. WHEN Użytkownik dotknie ikonę pomocy przy innym ustawieniu podczas gdy Tooltip_Kontekstowy jest widoczny, THE System_Onboardingu SHALL ukryć bieżący tooltip i wyświetlić nowy Tooltip_Kontekstowy dla dotkniętego ustawienia
7. WHILE Tooltip_Kontekstowy jest widoczny, THE System_Onboardingu SHALL wyświetlać jednocześnie nie więcej niż jeden Tooltip_Kontekstowy na ekranie

### Wymaganie 4: Strona pomocy dostępna z ustawień

**User Story:** Jako użytkownik, chcę mieć dostęp do pełnej dokumentacji funkcji czytnika z poziomu aplikacji, abym mógł w dowolnym momencie sprawdzić jak działa dana funkcja.

#### Kryteria Akceptacji

1. THE Panel_Ustawień SHALL zawierać pozycję "Pomoc / Przewodnik" prowadzącą do Strony_Pomocy
2. THE Strona_Pomocy SHALL prezentować listę wszystkich konfigurowalnych funkcji czytnika pogrupowanych według kategorii: Wyświetlanie, Czytanie, HUD podczas czytania, Język i Połączenie
3. WHEN Użytkownik wybierze kategorię na Stronie_Pomocy, THE System_Onboardingu SHALL wyświetlić rozwijaną listę funkcji w danej kategorii, gdzie każda pozycja zawiera nazwę funkcji, opis jej działania oraz dostępne wartości lub zakres
4. THE Strona_Pomocy SHALL zawierać sekcję "Szybki start" na górze, zawierającą instrukcję pierwszego połączenia z urządzeniem, opis jak wysłać książkę oraz opis jak rozpocząć czytanie
5. THE Strona_Pomocy SHALL działać offline (bez połączenia z urządzeniem i bez dostępu do internetu), korzystając wyłącznie z zasobów zapisanych w cache Service Workera
6. WHEN Użytkownik nawiguje do Strony_Pomocy, THE Strona_Pomocy SHALL wyświetlić przycisk powrotu do Panel_Ustawień widoczny na górze ekranu
7. WHEN Użytkownik rozwinie opis funkcji w kategorii, THE Strona_Pomocy SHALL zwinąć poprzednio otwarty opis w tej samej kategorii, tak aby w danej kategorii maksymalnie 1 opis był rozwinięty jednocześnie

### Wymaganie 5: Ponowne uruchomienie tutorialu

**User Story:** Jako użytkownik, który pominął tutorial lub chce odświeżyć wiedzę, chcę mieć możliwość ponownego uruchomienia przewodnika, abym mógł wrócić do materiału edukacyjnego w dowolnym momencie.

#### Kryteria Akceptacji

1. THE Strona_Pomocy SHALL zawierać przycisk "Uruchom tutorial ponownie" umożliwiający ponowne wyświetlenie Tutorial_Powitalnego
2. WHEN Użytkownik uruchomi tutorial ponownie, THE System_Onboardingu SHALL wyświetlić pełną sekwencję ekranów tutorialu, identyczną jak przy pierwszym uruchomieniu
3. WHEN Użytkownik ukończy ponownie uruchomiony tutorial, THE System_Onboardingu SHALL zaktualizować stan w localStorage na "ukończony"

### Wymaganie 6: Wskazówki przy pierwszym użyciu funkcji

**User Story:** Jako nowy użytkownik, chcę otrzymywać kontekstowe wskazówki gdy po raz pierwszy korzystam z zaawansowanej funkcji, abym rozumiał co się dzieje na ekranie.

#### Kryteria Akceptacji

1. WHEN Użytkownik otwiera ekran czytania po raz pierwszy (brak odpowiedniego wpisu w localStorage), THE System_Onboardingu SHALL wyświetlić overlay o maksymalnie 150 znakach tekstu, wyjaśniający podstawowe sterowanie (pauza, zmiana tempa, powrót do menu), blokując interakcję z elementami pod overlayem do momentu zamknięcia
2. WHEN Użytkownik otwiera konwerter plików po raz pierwszy (brak odpowiedniego wpisu w localStorage), THE System_Onboardingu SHALL wyświetlić overlay informujący o obsługiwanych formatach (EPUB, PDF, TXT, MD, HTML) i procesie konwersji do formatu .rsvp, blokując interakcję z elementami pod overlayem do momentu zamknięcia
3. THE System_Onboardingu SHALL zapisywać w localStorage osobny klucz dla każdego ekranu, na którym wyświetlono wskazówkę, aby przy kolejnej wizycie na tym ekranie wskazówka nie była wyświetlana ponownie
4. WHEN Użytkownik wyświetli wskazówkę, THE System_Onboardingu SHALL wyświetlić przycisk zamknięcia o minimalnym obszarze dotyku 44×44 piksele, umożliwiający zamknięcie wskazówki jednym dotknięciem
5. IF localStorage jest niedostępny lub zapis kończy się błędem, THEN THE System_Onboardingu SHALL nie wyświetlać wskazówki i nie blokować dostępu do funkcjonalności ekranu
6. WHEN Użytkownik zamknie wskazówkę, THE System_Onboardingu SHALL usunąć overlay i odblokować interakcję z ekranem w czasie nie dłuższym niż 300 milisekund

### Wymaganie 7: Wielojęzyczność systemu onboardingu

**User Story:** Jako użytkownik korzystający z czytnika w swoim języku, chcę aby system onboardingu wyświetlał treści w wybranym przeze mnie języku interfejsu.

#### Kryteria Akceptacji

1. THE System_Onboardingu SHALL wyświetlać wszystkie treści (tutorial, tooltips, stronę pomocy) w języku odpowiadającym bieżącej wartości ustawienia języka interfejsu (klucz NVS `ui_lang`)
2. THE System_Onboardingu SHALL obsługiwać wszystkie 6 języków dostępnych w aplikacji: angielski (0), hiszpański (1), francuski (2), niemiecki (3), rumuński (4) i polski (5)
3. WHEN Użytkownik zmieni język interfejsu, THE System_Onboardingu SHALL zaktualizować język wszystkich widocznych treści onboardingu w czasie nie dłuższym niż 500 milisekund od momentu zmiany
4. IF tłumaczenie treści onboardingu dla wybranego języka jest niedostępne, THEN THE System_Onboardingu SHALL wyświetlić treść w języku angielskim (fallback)

### Wymaganie 8: Dostosowanie do ekranu e-ink

**User Story:** Jako użytkownik korzystający z czytnika z ekranem e-ink, chcę aby elementy onboardingu wyświetlane na urządzeniu były czytelne i nie powodowały ghostingu.

#### Kryteria Akceptacji

1. THE System_Onboardingu SHALL wyświetlać treści edukacyjne wyłącznie w aplikacji PWA (na telefonie), nie wysyłając żadnych komend wyświetlania ani odświeżania do wyświetlacza e-ink urządzenia podczas trwania onboardingu
2. THE System_Onboardingu SHALL renderować treści tutorialu czcionką o rozmiarze nie mniejszym niż 16px oraz ze współczynnikiem kontrastu tekstu do tła wynoszącym co najmniej 4.5:1 zgodnie z WCAG AA
3. THE System_Onboardingu SHALL dostosowywać kolorystykę treści edukacyjnych do aktualnie wybranego motywu aplikacji (jasny, ciemny, nocny), stosując kolory tła i tekstu zdefiniowane dla danego motywu
4. THE System_Onboardingu SHALL wyświetlać treści edukacyjne bez użycia animacji i przejść CSS trwających dłużej niż 200 milisekund, aby zapewnić komfort odczytu na ekranach o niskiej częstotliwości odświeżania
