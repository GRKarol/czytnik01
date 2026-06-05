# Pełna Lista Wszystkich Ustawień Czytnika Flower — SZCZEGÓŁOWA

## LEGENDA

- 📱 = dostępne w aplikacji PWA (telefon)
- 🔧 = dostępne na urządzeniu (menu ekranowe)
- 🔐 = tylko w trybie developera
- **Klucz NVS** = nazwa pod którą ustawienie jest zapisane w pamięci nieulotnej urządzenia (ESP32 Preferences / NVS)

---

## 1. JĘZYK INTERFEJSU

**Nazwa w menu:** Language / Jezyk / Sprache / Langue / Idioma / Limba  
**Dostępność:** 📱🔧  
**Klucz NVS:** `ui_lang`  
**Typ:** enum uint8_t, wartości 0–5  
**Wartość domyślna:** 0 (English)  
**Gdzie w menu:** Settings → Display → Language (pozycja 10)

### Dostępne języki:

| Indeks | Kod | Nazwa wyświetlana w menu |
| ------ | --- | ------------------------ |
| 0      | en  | English                  |
| 1      | es  | Espanol                  |
| 2      | fr  | Francais                 |
| 3      | de  | Deutsch                  |
| 4      | ro  | Romana                   |
| 5      | pl  | Polski                   |

**Szczegół:** Firmware zawiera pełne tłumaczenia WSZYSTKICH etykiet menu, komunikatów, przycisków i statusów w 6 językach. Zmiana języka dotyczy całego interfejsu urządzenia (nie tylko czytnika). Niedostępne są języki z diakrytykami (embedded font nie obsługuje znaków specjalnych — np. polskie tłumaczenie ma "Jasnosc" zamiast "Jasność").

---

## 2. MOTYW (THEME)

**Nazwa w menu:** Theme / Motyw / Thema / Theme / Tema / Tema  
**Dostępność:** 📱🔧  
**Klucz NVS:** `dark` (bool) + `night` (bool) — dwa osobne flagi  
**Typ:** dwa bool; logika: `nightMode=true` → Night; `darkMode=true, nightMode=false` → Dark; obydwa false → Light  
**Wartość domyślna:** Dark (`dark=true`, `night=false`)  
**Gdzie w menu:** Settings → Display → Theme (pozycja 1)  
**Na urządzeniu:** przytrzymanie przycisku Power przez 900 ms przełącza motyw cyklicznie

### Dostępne wartości:

| Wartość                                               | dark  | night | Opis                                                           |
| ----------------------------------------------------- | ----- | ----- | -------------------------------------------------------------- |
| **Light** / Jasny / Hell / Clair / Claro / Deschis    | false | false | Jasne tło, ciemny tekst                                        |
| **Dark** / Ciemny / Dunkel / Sombre / Oscuro / Inchis | true  | false | Ciemne tło, jasny tekst — domyślny                             |
| **Night** / Nocny / Nacht / Nuit / Noche / Noapte     | true  | true  | Nocny: ciepłe żółte litery na czarnym tle, zredukowana jasność |

**Szczegół Night mode:** W trybie nocnym kolor podświetlenia liter (focus color) również zmienia się na ciepłą wersję — np. czerwony staje się ciepło-czerwony (RGB565: 0xFA80 zamiast 0xF800).

---

## 3. JASNOŚĆ (BRIGHTNESS)

**Nazwa w menu:** Brightness / Jasnosc / Helligkeit / Luminosite / Brillo / Luminoz.  
**Dostępność:** 📱🔧  
**Klucz NVS:** `bright` (uint8_t)  
**Typ:** indeks 0–4 (5 stopni)  
**Wartość domyślna:** 4 (100% / maksimum)  
**Gdzie w menu:** Settings → Display → Brightness (pozycja 2)  
**Na urządzeniu:** krótkie naciśnięcie przycisku Power (poza menu) przełącza cyklicznie przez 5 poziomów

### Poziomy jasności:

| Indeks NVS | Tryb Light/Dark | Tryb Night |
| ---------- | --------------- | ---------- |
| 0          | 40%             | 35%        |
| 1          | 55%             | 40%        |
| 2          | 70%             | 45%        |
| 3          | 85%             | 50%        |
| 4          | 100%            | 55%        |

**Szczegół:** W trybie nocnym maksymalna jasność jest ograniczona do 55%, żeby chronić wzrok. Wartości są zdefiniowane w firmware jako dwie osobne tablice: `kBrightnessLevels[]` i `kNightBrightnessLevels[]`.

---

## 4. DŁOŃ (READER HAND / HANDEDNESS)

**Nazwa w menu:** Reader hand / Dlon / Hand / Main / Mano / Mana  
**Dostępność:** 📱🔧  
**Klucz NVS:** `handed` (uint8_t)  
**Typ:** enum HandednessMode (0=Right, 1=Left)  
**Wartość domyślna:** 0 (Right / Prawa)  
**Gdzie w menu:** Settings → Display → Reader hand (pozycja 3)

### Dostępne wartości:

| Wartość NVS | Etykieta                                            | Opis efektu                                                                                           |
| ----------- | --------------------------------------------------- | ----------------------------------------------------------------------------------------------------- |
| 0           | Right / Prawa / Rechts / Droite / Derecha / Dreapta | Elementy sterowania po lewej stronie ekranu, tekst zakotwiczony normalnie                             |
| 1           | Left / Lewa / Links / Gauche / Izquierda / Stanga   | Elementy sterowania po prawej stronie ekranu, pozycja kotwicy przesuwa się o +20 (anchor offset = 20) |

**Szczegół:** Zmiana dłoni wpływa na `anchorPercent` — dla lewej ręki do wartości bazowej dodawane jest 20, zakres przesuwa się z 30–40% na 50–60% ekranu.

---

## 5. METRYKA STOPKI (FOOTER METRIC MODE)

**Nazwa w menu:** Footer label / Stopka / Fusszeile / Pied / Pie / Subsol  
**Dostępność:** 🔧 (tylko urządzenie)  
**Klucz NVS:** `prog_md` (uint8_t)  
**Typ:** enum FooterMetricMode (0–2)  
**Wartość domyślna:** 0 (Percentage)  
**Gdzie w menu:** Settings → Display → Footer label (pozycja 4)  
**Jak zmienić:** dotknięcie obszaru stopki podczas czytania (tap na dół ekranu) przełącza cyklicznie

### Dostępne wartości:

| Wartość NVS | Etykieta                                                                            | Opis                                                  |
| ----------- | ----------------------------------------------------------------------------------- | ----------------------------------------------------- |
| 0           | Percent read / Procent / Prozent / Pourcentage / Porcentaje / Procent               | Procent przeczytanego tekstu (np. "42%")              |
| 1           | Chapter time / Czas rozdz. / Kapitelzeit / Temps chap. / Tiempo cap. / Timp capitol | Szacowany czas pozostały do końca bieżącego rozdziału |
| 2           | Book time / Czas ksiazki / Buchzeit / Temps livre / Tiempo libro / Timp carte       | Szacowany czas pozostały do końca całej książki       |

---

## 6. ETYKIETA BATERII (BATTERY LABEL MODE)

**Nazwa w menu:** Battery label / Bateria / Akku / Batterie / Bateria / Baterie  
**Dostępność:** 🔧 (tylko urządzenie)  
**Klucz NVS:** `bat_md` (uint8_t)  
**Typ:** enum BatteryLabelMode (0–2)  
**Wartość domyślna:** 0 (Percent)  
**Gdzie w menu:** Settings → Display → Battery label (pozycja 5)  
**Jak zmienić:** dotknięcie ikony baterii podczas czytania przełącza cyklicznie

### Dostępne wartości:

| Wartość NVS | Etykieta                                                                              | Opis                                                                                                        |
| ----------- | ------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------- |
| 0           | Percentage / Procent / Prozent / Pourcentage / Porcentaje / Procent                   | Procent naładowania baterii (np. "87%")                                                                     |
| 1           | Time remaining / Czas pracy / Restzeit / Temps restant / Tiempo restante / Timp ramas | Szacowany czas pracy do rozładowania (np. "3h 20min"). Szacowanie wymaga min. 10 min pracy i min. 3% spadku |
| 2           | Voltage / Napiecie / Spannung / Tension / Voltaje / Tensiune                          | Napięcie baterii (np. "3.85V")                                                                              |

---

## 7. WYGASZACZ EKRANU (SCREENSAVER MODE)

**Nazwa w menu:** Screensaver / Wygaszacz / Bildschirmsch. / Ecran veille / Salvapant. / Screensaver  
**Dostępność:** 🔧 (tylko urządzenie)  
**Klucz NVS:** `scrn_sv` (uint8_t)  
**Typ:** enum ScreensaverMode  
**Wartość domyślna:** Life (0)  
**Gdzie w menu:** Settings → Display → Screensaver (pozycja 6)

### Dostępne wartości:

| Wartość NVS | Etykieta                                                                         | Opis animacji                                                                                                                                 |
| ----------- | -------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| 0           | Life / Zycie / Leben / Vie / Vida / Viata                                        | Conway's Game of Life — komórkowy automat. Zawiera wbudowane wzorce: Glider, Lightweight Spaceship, Pentadecathlon, Pulsar, Gosper Glider Gun |
| 2           | Maze / Labirynt / Labyrinth / Labyrinthe / Laberinto / Labirint                  | Generowanie labiryntu w czasie rzeczywistym (algorytm DFS)                                                                                    |
| 3           | Voronoi                                                                          | Diagram Woronoja z poruszającymi się punktami                                                                                                 |
| 6           | Screen off / Wylacz / Bildschirm aus / Ecran eteint / Apagar pant. / Ecran oprit | Ekran wyłączony podczas standby — oszczędza energię                                                                                           |

**Uwaga:** Wartości NVS nie są ciągłe (0, 2, 3, 6) — to wynik usunięcia niektórych trybów w trakcie rozwoju.

---

## 8. BATERIA PODCZAS CZYTANIA (READING BATTERY VISIBLE)

**Nazwa w menu:** Reading battery / Bateria w czyt. / Akku beim Lesen / Batterie en lect. / Bateria en lect. / Baterie in citire  
**Dostępność:** 📱🔧  
**Klucz NVS:** `read_bat` (bool)  
**Wartość domyślna:** true (włączone)  
**Gdzie w menu:** Settings → Display → Reading battery (pozycja 7)

| Wartość                            | Opis                                                                          |
| ---------------------------------- | ----------------------------------------------------------------------------- |
| true / Tak / Ja / Oui / Si / Da    | Ikona/etykieta baterii jest widoczna w górnym rogu podczas aktywnego czytania |
| false / Nie / Nein / Non / No / Nu | Ikona baterii ukryta podczas czytania (pełny ekran dla tekstu)                |

---

## 9. ROZDZIAŁ PODCZAS CZYTANIA (READING CHAPTER VISIBLE)

**Nazwa w menu:** Reading chapter / Rozdz. w czyt. / Kapitel beim Lesen / Chap. en lect. / Cap. en lect. / Capitol in citire  
**Dostępność:** 📱🔧  
**Klucz NVS:** `read_ch` (bool)  
**Wartość domyślna:** false (wyłączone)  
**Gdzie w menu:** Settings → Display → Reading chapter (pozycja 8)

| Wartość     | Opis                                                   |
| ----------- | ------------------------------------------------------ |
| true / Tak  | Nazwa bieżącego rozdziału wyświetlana podczas czytania |
| false / Nie | Nazwa rozdziału ukryta podczas czytania                |

---

## 10. POSTĘP PODCZAS CZYTANIA (READING PROGRESS VISIBLE)

**Nazwa w menu:** Reading percent / Procent w czyt. / Prozent beim Lesen / Pourcent. en lect. / Porcent. en lect. / Procent in citire  
**Dostępność:** 📱🔧  
**Klucz NVS:** `read_pct` (bool)  
**Wartość domyślna:** false (wyłączone)  
**Gdzie w menu:** Settings → Display → Reading percent (pozycja 9)

| Wartość     | Opis                                            |
| ----------- | ----------------------------------------------- |
| true / Tak  | Procent ukończenia wyświetlany podczas czytania |
| false / Nie | Procent ukryty                                  |

---

## 11. KOLOR LITERY FOKUSOWEJ (FOCUS COLOR)

**Nazwa w menu:** Focus color / Kolor litery / (brak tłumaczenia w 6 językach — tylko PL/EN)  
**Dostępność:** 🔧 (tylko urządzenie; w PWA brak osobnej opcji)  
**Klucz NVS:** `foc_clr` (uint8_t)  
**Typ:** indeks 0–5  
**Wartość domyślna:** 0 (Red / Czerwony)  
**Gdzie w menu:** Settings → Display → Focus color (pozycja 11)  
**Gdzie jeszcze:** Welcome Wizard → "Highlight color" (pierwsze uruchomienie)

### Dostępne kolory:

| Indeks | Nazwa PL     | Nazwa EN | RGB565 (tryb normalny) | RGB565 (tryb nocny)             |
| ------ | ------------ | -------- | ---------------------- | ------------------------------- |
| 0      | Czerwony     | Red      | 0xF800                 | 0xFA80 (ciepły czerwony)        |
| 1      | Niebieski    | Blue     | 0x001F                 | 0x541F (przytłumiony niebieski) |
| 2      | Zielony      | Green    | 0x07E0                 | 0x37E0 (zielony)                |
| 3      | Zolty        | Yellow   | 0xFFE0                 | 0xFEA0 (ciepły żółty)           |
| 4      | Pomaranczowy | Orange   | 0xFC40                 | 0xFB60 (pomarańczowy)           |
| 5      | Fioletowy    | Purple   | 0xA01F                 | 0x9817 (fioletowy)              |

**Szczegół:** Kolor litery fokusowej to podświetlenie ORP (Optimal Recognition Point) — środkowej litery słowa, na której oko skupia się przy czytaniu. Kolor używany jest również dla: ikony "SP" (save point), belki wyboru w menu, prowadnic typograficznych (guide ticks).

---

## 12. PRZYCISK ZAPISU (SAVE POINT BUTTON VISIBLE)

**Nazwa w menu:** Save btn / Przycisk zapisu  
**Dostępność:** 🔧 (tylko urządzenie)  
**Klucz NVS:** `sp_btn` (bool)  
**Wartość domyślna:** true (włączone)  
**Gdzie w menu:** Settings → Display → Save btn (pozycja 12)

| Wartość     | Opis                                                                                                               |
| ----------- | ------------------------------------------------------------------------------------------------------------------ |
| true / Tak  | Przycisk "SP" widoczny w górnym rogu ekranu podczas czytania. Dotknięcie go tworzy punkt zapisu w bieżącym miejscu |
| false / Nie | Przycisk "SP" ukryty (punkty zapisu można tworzyć tylko przez menu)                                                |

---

## 13. TRYB CZYTNIKA (READING MODE)

**Nazwa w menu:** Reading mode / Tryb czyt. / Lesemodus / Mode lecture / Modo lectura / Mod citire  
**Dostępność:** 📱🔧  
**Klucz NVS:** `read_mode` (uint8_t)  
**Typ:** enum ReaderMode (0=RSVP, 1=Scroll)  
**Wartość domyślna:** 0 (RSVP)  
**Gdzie w menu:** Settings → Reading → Reading mode (pozycja 1)

### Dostępne wartości:

| Wartość NVS | Etykieta                                                                                        | Opis                                                                                                                                            |
| ----------- | ----------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| 0           | RSVP                                                                                            | Rapid Serial Visual Presentation — jedno słowo na raz wyświetlane w centrum ekranu z podświetloną literą fokusową. Tempo kontrolowane przez WPM |
| 1           | Page scroll / Scroll strony / Seiten-Scroll / Defilement page / Scroll pagina / Derulare pagina | Klasyczne przewijanie — tekst wyświetlany stronicami, nawigacja gestami                                                                         |

**Uwaga historyczna:** Istniał trzeci tryb "word scroll" (wartość 2), który został usunięty. Urządzenia z zapisaną wartością 2 są automatycznie migrowane do Page scroll (1).

---

## 14. ZACHOWANIE PAUZY (PAUSE BEHAVIOUR)

**Nazwa w menu:** Pause behaviour / Pauza / Pause / Pause / Pausa / Pauza  
**Dostępność:** 📱🔧  
**Klucz NVS:** `pause_md` (uint8_t)  
**Typ:** enum PauseMode (0=SentenceEnd, 1=Instant)  
**Wartość domyślna:** 0 (SentenceEnd)  
**Gdzie w menu:** Settings → Reading → Pause behaviour (pozycja 2)

### Dostępne wartości:

| Wartość NVS | Etykieta                                                         | Opis                                                                                                             |
| ----------- | ---------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| 0           | Sentence / Zdanie / Satz / Phrase / Oracion / Propozitie         | Pauza zatrzymuje się na końcu bieżącego zdania (naturalne miejsce przerwy). W PWA odpowiada "Tap" i "Long-press" |
| 1           | Instant / Natychm. / Sofort / Instantane / Instantaneo / Instant | Pauza natychmiastowa — czytanie zatrzymuje się dokładnie w momencie dotyku. W PWA odpowiada "Auto"               |

---

## 15. TEMPO CZYTANIA (WPM — BASE READING SPEED)

**Nazwa w menu:** Base speed / Tempo / Tempo / Vitesse / Velocidad / Viteza  
**Dostępność:** 📱🔧  
**Klucz NVS:** `wpm` (uint16_t)  
**Wartość domyślna:** 300 WPM  
**Gdzie w menu:** Settings → Reading → Base speed (pozycja 3)

### Zakresy i kroki:

| Zakres       | Krok      | Opis                                                     |
| ------------ | --------- | -------------------------------------------------------- |
| 10–100 WPM   | co 10 WPM | Zakres wolny, dokładne kroki dla bardzo wolnego czytania |
| 100–1000 WPM | co 25 WPM | Zakres normalny i szybki                                 |

**Pełny zakres:** 10–1000 WPM  
**Jak zmienić na urządzeniu:** przesunięcie palcem w górę/dół podczas pauzy zmienia tempo; feedback WPM wyświetlany przez 900 ms po zmianie  
**Jak zmienić w PWA:** suwak 50–1000 WPM w krokach co 25 (zakres dolny uproszczony)

---

## 16. OPÓŹNIENIE DLA DŁUGICH SŁÓW (LONG WORD DELAY)

**Nazwa w menu:** Long words / Dlugie slowa / Lange Worter / Mots longs / Palabras largas / Cuvinte lungi  
**Dostępność:** 📱🔧  
**Klucz NVS:** `pace_lms` (uint16_t) — migracja ze starszego klucza `pace_len`  
**Wartość domyślna:** 200 ms  
**Gdzie w menu:** Settings → Reading → Long words (pozycja 4)

### Zakres:

| Min  | Max    | Krok  | Jednostka   |
| ---- | ------ | ----- | ----------- |
| 0 ms | 600 ms | 50 ms | milisekundy |

**Dostępne wartości:** 0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600 ms  
**Opis:** Dodatkowy czas wyświetlania dla słów powyżej określonej długości. Wartość 0 = brak dodatkowego opóźnienia.

---

## 17. OPÓŹNIENIE DLA ZŁOŻONYCH SŁÓW (COMPLEX WORD DELAY)

**Nazwa w menu:** Complexity / Zlozonosc / Komplexitat / Complexite / Complejidad / Complexitate  
**Dostępność:** 📱🔧  
**Klucz NVS:** `pace_cms` (uint16_t) — migracja ze starszego klucza `pace_cpx`  
**Wartość domyślna:** 200 ms  
**Gdzie w menu:** Settings → Reading → Complexity (pozycja 5)

### Zakres:

| Min  | Max    | Krok  | Jednostka   |
| ---- | ------ | ----- | ----------- |
| 0 ms | 600 ms | 50 ms | milisekundy |

**Opis:** Dodatkowy czas dla słów zawierających myślniki, ukośniki, cyfry, znaki specjalne (np. "state-of-the-art", "HTTP/2", "99%").

---

## 18. OPÓŹNIENIE DLA INTERPUNKCJI (PUNCTUATION DELAY)

**Nazwa w menu:** Punctuation / Interpunk. / Zeichen / Ponctuation / Puntuacion / Punctuatie  
**Dostępność:** 📱🔧  
**Klucz NVS:** `pace_pms` (uint16_t) — migracja ze starszego klucza `pace_pnc`  
**Wartość domyślna:** 200 ms  
**Gdzie w menu:** Settings → Reading → Punctuation (pozycja 6)

### Zakres:

| Min  | Max    | Krok  | Jednostka   |
| ---- | ------ | ----- | ----------- |
| 0 ms | 600 ms | 50 ms | milisekundy |

**Opis:** Dodatkowy czas pauzy po wyświetleniu słowa kończącego się znakiem interpunkcyjnym (`.`, `,`, `!`, `?`, `:`, `;`). Symuluje naturalne przerwy w mówieniu.

**Resetowanie opóźnień:** Settings → Reading → Reset pacing — przywraca wszystkie trzy opóźnienia (poz. 16, 17, 18) do domyślnych 200 ms

---

## 19. ROZMIAR CZCIONKI (FONT SIZE)

**Nazwa w menu:** Font size / Rozmiar / Schriftgrad / Taille / Tamano / Marime  
**Dostępność:** 📱🔧  
**Klucz NVS:** `font_size` (uint8_t)  
**Typ:** indeks 0–2  
**Wartość domyślna:** 0 (Small / Maly)  
**Gdzie w menu:** Settings → Typography → Font size (pozycja 1)

### Dostępne wartości:

| Indeks | Etykieta                                           | Opis                                                             |
| ------ | -------------------------------------------------- | ---------------------------------------------------------------- |
| 0      | Small / Maly / Klein / Petit / Pequeno / Mic       | Mała czcionka — więcej słów widocznych w podglądzie kontekstowym |
| 1      | Medium / Sredni / Mittel / Moyen / Mediano / Mediu | Średnia czcionka                                                 |
| 2      | Large / Duzy / Gross / Grand / Grande / Mare       | Duża czcionka — mniej słów w podglądzie, ale lepsza czytelność   |

**Efekt na phantom words:** Rozmiar czcionki określa ile znaków może być widocznych w "słowach widma" przed i po bieżącym słowie:

- Small: 64 znaki przed, 96 znaków po
- Medium: 96 znaków przed, 144 znaków po
- Large: 144 znaków przed, 208 znaków po

---

## 20. KRÓJ CZCIONKI (TYPEFACE)

**Nazwa w menu:** Typeface / Kroj / Schriftart / Police / Fuente / Font  
**Dostępność:** 📱🔧  
**Klucz NVS:** `typeface` (uint8_t)  
**Typ:** enum ReaderTypeface (0–2)  
**Wartość domyślna:** 0 (Standard)  
**Gdzie w menu:** Settings → Typography → Typeface (pozycja 2)

### Dostępne wartości:

| Wartość NVS | Etykieta              | Opis                                                                                                                                                                         |
| ----------- | --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0           | Standard              | Domyślna czcionka szeryfowa (Iowan Old Style / Georgia) — zoptymalizowana pod ekran e-ink                                                                                    |
| 1           | Atkinson Hyperlegible | Czcionka zaprojektowana przez Braille Institute dla maksymalnej czytelności, szczególnie dla osób z osłabionym wzrokiem. Plik zawarty w `third_party/atkinson-hyperlegible/` |
| 2           | OpenDyslexic          | Czcionka z obciążonymi podstawami liter, pomagająca osobom z dysleksją. Plik zawarty w `third_party/opendyslexic/`                                                           |

**Cykl przełączania:** Standard → Atkinson Hyperlegible → OpenDyslexic → Standard

---

## 21. SŁOWA WIDMA (PHANTOM WORDS)

**Nazwa w menu:** Phantom words / Slowa widma / Phantomworter / Mots fantomes / Palabras fantasma / Cuvinte fantoma  
**Dostępność:** 📱🔧  
**Klucz NVS:** `phantom_on` (bool)  
**Wartość domyślna:** true (włączone)  
**Gdzie w menu:** Settings → Typography → Phantom words (pozycja 3)

| Wartość     | Opis                                                                                                                           |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------ |
| true / On   | Przed i po bieżącym słowie widoczne są przyciemnione słowa z kontekstu. Liczba znaków zależy od rozmiaru czcionki (patrz p.19) |
| false / Off | Tylko jedno słowo widoczne na ekranie — czysty tryb RSVP                                                                       |

---

## 22. PODŚWIETLENIE FOKUSOWE (FOCUS HIGHLIGHT)

**Nazwa w menu:** Red highlight / Czerwony / Rotfokus / Accent rouge / Rojo / Accent rosu  
**Dostępność:** 📱🔧  
**Klucz NVS:** `type_hlt` (bool)  
**Wartość domyślna:** true (włączone)  
**Gdzie w menu:** Settings → Typography → Focus highlight (pozycja 4)

| Wartość     | Opis                                                                                                                |
| ----------- | ------------------------------------------------------------------------------------------------------------------- |
| true / On   | Litera ORP (Optimal Recognition Point — środkowa litera słowa) podświetlona wybranym kolorem fokusowym (patrz p.11) |
| false / Off | Wszystkie litery jednakowego koloru — brak podświetlenia                                                            |

**Szczegół techniczny:** ORP (focus index) wyliczany jest jako `(długość_słowa - 1) / 2`, czyli środkowa litera. Podświetlenie działa zarówno w trybie RSVP jak i scroll.

---

## 23. ODSTĘPY MIĘDZY LITERAMI (TRACKING)

**Nazwa w menu:** Tracking / Odstepy / Laufweite / Espacement / Espaciado / Spatiere  
**Dostępność:** 📱🔧  
**Klucz NVS:** `type_trk` (int8_t — może być ujemny)  
**Wartość domyślna:** 0  
**Gdzie w menu:** Settings → Typography → Tracking (pozycja 5)

### Zakres:

| Min | Max | Krok | Jednostka                                         |
| --- | --- | ---- | ------------------------------------------------- |
| -2  | +3  | 1    | piksele dodane do odstępu między każdą parą liter |

| Wartość | Efekt wizualny                                         |
| ------- | ------------------------------------------------------ |
| -2      | Litery bardzo ściśnięte — więcej tekstu w jednej linii |
| -1      | Litery nieco ściśnięte                                 |
| 0       | Domyślne odstępy                                       |
| +1      | Nieco więcej powietrza                                 |
| +2      | Wyraźnie szerzej rozmieszczone litery                  |
| +3      | Litery bardzo rozstrzelone                             |

---

## 24. POZYCJA KOTWICY (ANCHOR PERCENT)

**Nazwa w menu:** Anchor / Kotwica / Anker / Ancre / Ancla / Ancora  
**Dostępność:** 📱🔧  
**Klucz NVS:** `type_anc` (uint8_t)  
**Wartość domyślna:** 30%  
**Gdzie w menu:** Settings → Typography → Anchor (pozycja 6)

### Zakres (tryb prawa ręka):

| Min | Max | Opis                                                                  |
| --- | --- | --------------------------------------------------------------------- |
| 30% | 40% | Procent wysokości ekranu od góry, gdzie wyświetlane jest główne słowo |

### Zakres (tryb lewa ręka):

| Min | Max | Opis                                   |
| --- | --- | -------------------------------------- |
| 50% | 60% | Przesuniecie o +20 od wartości bazowej |

**Efekt:** 30% = słowo w górnej części ekranu (więcej miejsca na phantom words poniżej); 40% = słowo w środku.

---

## 25. SZEROKOŚĆ PROWADNICY (GUIDE WIDTH)

**Nazwa w menu:** Guide width / Szer. guide / Guidebreite / Largeur guide / Ancho guia / Latime ghid  
**Dostępność:** 📱🔧  
**Klucz NVS:** `type_wid` (uint8_t)  
**Wartość domyślna:** 30 px  
**Gdzie w menu:** Settings → Typography → Guide width (pozycja 7)

### Zakres:

| Min   | Max   | Krok |
| ----- | ----- | ---- |
| 12 px | 30 px | 2 px |

**Dostępne wartości:** 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 px  
**Opis:** Szerokość poziomych linii prowadzących (guide lines) wyświetlanych po obu stronach bieżącego słowa — pomagają oku skupić się na słowie.

---

## 26. PRZERWA PROWADNICY (GUIDE GAP)

**Nazwa w menu:** Guide gap / Przerwa guide / Guidespalt / Ecart guide / Hueco guia / Spatiu ghid  
**Dostępność:** 📱🔧  
**Klucz NVS:** `type_gap` (uint8_t)  
**Wartość domyślna:** 5 px  
**Gdzie w menu:** Settings → Typography → Guide gap (pozycja 8)

### Zakres:

| Min  | Max  | Krok |
| ---- | ---- | ---- |
| 2 px | 8 px | 1 px |

**Dostępne wartości:** 2, 3, 4, 5, 6, 7, 8 px  
**Opis:** Pionowy odstęp między słowem a liniami prowadzącymi. Mniejsza wartość = prowadnice bliżej słowa.

**Resetowanie typografii:** Settings → Typography → Reset — przywraca wszystkie 8 ustawień typograficznych (p.19–26) do wartości domyślnych jednocześnie.

---

## 27. WI-FI — SIEĆ DOMOWA (HOME Wi-Fi SSID + PASSWORD)

**Nazwa w menu:** Home Wi-Fi / Wi-Fi domowe / Heim-Wi-Fi / Wi-Fi maison / Wi-Fi hogar / Wi-Fi acasa  
**Dostępność:** 🔧🔐 (urządzenie, wymaga trybu developera)  
**Klucze NVS:** `wifi_ssid` (String, max 63 zn.) + `wifi_pass` (String, max 63 zn., maskowane)  
**Wartość domyślna:** pusty (brak sieci)  
**Gdzie w menu:** Settings → Connectivity → Wi-Fi → Network (pozycja 1)

### Sposoby ustawienia sieci:

1. **Wpisanie ręczne** — Settings → Connectivity → Wi-Fi → Network → wpisanie SSID, następnie hasła (klawiatura ekranowa: małe/duże/symbole)
2. **Skanowanie sieci** — Settings → Connectivity → Wi-Fi → Choose network → lista dostępnych sieci posortowana wg siły sygnału (RSSI); sieci bezpieczne oznaczone "Secure", otwarte "Open"
3. **Usuwanie** — Settings → Connectivity → Wi-Fi → Forget network — usuwa oba klucze z NVS

**Klawiatura ekranowa** zawiera 3 tryby:

- Lowercase: `qwertyuiop` / `asdfghjkl` / `zxcvbnm` + spacja/backspace/clear/zapisz/anuluj
- Uppercase: `QWERTYUIOP` / `ASDFGHJKL` / `ZXCVBNM`
- Symbols: `1234567890` / `!@#$%^&*?` / `-_=+/:;.,`

---

## 28. AUTOMATYCZNE SPRAWDZANIE AKTUALIZACJI (OTA AUTO CHECK)

**Nazwa w menu:** Firmware update (auto) — pełna etykieta zmienna  
**Dostępność:** 🔧🔐 (urządzenie, wymaga trybu developera)  
**Klucz NVS:** `ota_auto` (bool)  
**Wartość domyślna:** false (wyłączone; jeśli klucz nie istnieje, używana wartość z `ota.conf` na karcie SD)  
**Gdzie w menu:** Settings → Connectivity → Wi-Fi (advanced) → Auto-update (pozycja 4)

| Wartość | Opis                                                                                               |
| ------- | -------------------------------------------------------------------------------------------------- |
| true    | Urządzenie sprawdza GitHub przy każdym starcie — jeśli dostępna jest nowsza wersja, pokazuje monit |
| false   | Brak automatycznych sprawdzeń — aktualizacja tylko ręczna                                          |

---

## 29. ŹRÓDŁO OTA (OTA OWNER / GITHUB OWNER)

**Nazwa w menu:** OTA Source / GitHub owner  
**Dostępność:** 🔧🔐 (urządzenie, wymaga trybu developera)  
**Klucz NVS:** `ota_owner` (String, max 39 zn.)  
**Wartość domyślna:** pusty (używa domyślnego właściciela z `ota.conf` lub wbudowanego)  
**Gdzie w menu:** Settings → Connectivity → Wi-Fi (advanced) → OTA owner (pozycja 5)

**Opis:** Nazwa właściciela repozytorium GitHub z którego pobierane są pliki `.bin` aktualizacji OTA. Umożliwia używanie własnych forków firmware. Jeśli pusty — reset do domyślnego.

---

## 30. BLUETOOTH / BLE (PHONE SYNC — BLUETOOTH TOGGLE)

**Nazwa w menu:** Phone sync / Sync z tel. / Handy-Sync / Sync tel. / Sync movil / Sync telefon  
**Dostępność:** 🔧 (tylko urządzenie; widoczne zawsze w Connectivity)  
**Klucz NVS:** `ble_on` (bool)  
**Wartość domyślna:** false (wyłączone)  
**Gdzie w menu:** Settings → Connectivity → Bluetooth (pozycja 2, jeśli BLE enabled) / Phone sync (pozycja 2/3)

| Wartość     | Opis                                                                                                                                                                        |
| ----------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| true / On   | BLE Peripheral uruchomiony — urządzenie widoczne dla aplikacji Flower na telefonie. Stan zapamiętywany między restartami — BLE startuje automatycznie przy każdym włączeniu |
| false / Off | BLE wyłączony — urządzenie niewidoczne dla telefonu                                                                                                                         |

**Jak zmienić:** wybranie opcji toggleuje stan i natychmiast uruchamia/zatrzymuje BLE stos  
**Kompilacja warunkowa:** obecność opcji w menu zależy od flagi `FLOWER_BLE_ENABLED` w build config

---

## 31. TRYB DEVELOPERA (DEV MODE)

**Nazwa w menu:** Developer mode: ON (turn off) / Tryb dev: WL (wylacz) / Dev-Modus: AN (ausschalten) / Mode dev: ON (desactiver) / Modo dev: ON (desactivar) / Mod dev: ON (dezactiveaza)  
**Dostępność:** 📱🔧  
**Klucz NVS:** `dev_mode` (bool)  
**Wartość domyślna:** false (wyłączony)  
**Gdzie w menu:** Settings → About / Info → Dev mode (pozycja 4 — WIDOCZNA TYLKO gdy dev mode jest WŁĄCZONY)

### Sposoby aktywacji:

| Sposób          | Opis                                                                                                                                            |
| --------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| Na urządzeniu   | 10-krotne dotknięcie pozycji "Version" w Settings → About w ciągu 1,5 sekundy (licznik `aboutTapCount_`, reset po 1500 ms)                      |
| W aplikacji PWA | 10-krotne kliknięcie logo "Flower" w panelu Settings aplikacji mobilnej (licznik `tapCount`, reset po 1500 ms) → synchronizacja przez BLE/Wi-Fi |

### Co odblokowuje:

- Settings → Connectivity → Wi-Fi (advanced) — zaawansowane opcje OTA
- Settings → Connectivity → Wi-Fi → Auto-update (p.28)
- Settings → Connectivity → Wi-Fi → OTA owner (p.29)
- Settings (Home) → Wi-Fi (advanced) — skrót do zaawansowanego Wi-Fi
- Settings (Home) → Typography — ukryta sekcja typografii
- Settings (Home) → Firmware update — aktualizacja firmware
- Opcja wyłączenia dev mode w Settings → About

### Wyłączanie:

Po włączeniu opcja "Developer mode: ON (turn off)" pojawia się w Settings → About. Wybranie jej wyłącza tryb developerski, a wszystkie zaawansowane opcje znikają z menu.

---

## 32. SZACOWANIE CZASU — TRYB (TIME ESTIMATE MODE)

**Dostępność:** 🔧 (tylko urządzenie; brak w PWA)  
**Klucz NVS:** `time_est_a` (bool)  
**Wartość domyślna:** true (Accurate)  
**Gdzie w menu:** Stopka podczas czytania → dotknięcie obszaru stopki gdy Footer Metric = Chapter Time lub Book Time

| Wartość | Etykieta                                               | Opis                                                                                                                                                                                                           |
| ------- | ------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| true    | Accurate / Dokladny / Genau / Precis / Preciso / Exact | Precyzyjne szacowanie — uwzględnia opóźnienia pace (długie słowa, złożone, interpunkcja). Wymaga wstępnego przeskanowania tekstu (budowane w tle, blok po bloku — kBTimeEstimateBlockWords = 256 słów na blok) |
| false   | Fast / Szybki / Schnell / Rapide / Rapido / Rapid      | Szybkie szacowanie — stała nominalna prędkość czytania bez uwzględniania opóźnień pacing                                                                                                                       |

**Uwaga:** Po podłączeniu PWA i wysłaniu ustawień, tryb jest zawsze resetowany do "Accurate" (`preferences_.putBool(kPrefAccurateTime, true)` w CompanionSyncManager).

---

## 33. KANAŁY RSS (RSS FEEDS)

**Dostępność:** 🔧 (tylko urządzenie, przez menu Connectivity)  
**Gdzie w menu:** Settings → Connectivity → RSS feeds (pozycja 3 lub 4 zależnie od BLE)  
**Konfiguracja:** plik `/config/rss.conf` na karcie SD (max 24 kanały)  
**Edycja przez PWA:** tak — zakładka RSS w Web Companion (interfejs webowy gdy urządzenie w trybie Wi-Fi AP)

**Opis:** Urządzenie pobiera artykuły z RSS i zapisuje je jako pliki `.rsvp` w katalogu artykułów. Pobieranie wymaga skonfigurowanego Wi-Fi (p.27). Uruchamiane ręcznie przez menu lub automatycznie przy auto-check OTA.

---

## 34. TRANSFER USB

**Dostępność:** 🔧 (tylko urządzenie; widoczne gdy `RSVP_USB_TRANSFER_ENABLED = 1`)  
**Gdzie w menu:** Settings → Connectivity → USB (ostatnia pozycja)  
**Klucz NVS:** brak (stan tymczasowy)

**Opis:** Montuje kartę SD jako dysk USB Mass Storage. Użytkownik może kopiować pliki z komputera. Po zakończeniu należy wysunąć dysk z systemu, a następnie przytrzymać przycisk zasilania przez 1200 ms aby wyjść z trybu USB.

**Tryb auto-start:** jeśli `RSVP_USB_TRANSFER_AUTO_START = 1` (flaga build), tryb USB startuje automatycznie przy każdym uruchomieniu urządzenia.

---

## PEŁNA TABELA WSZYSTKICH KLUCZY NVS

Wszystkie ustawienia zapisywane są w ESP32 NVS (Non-Volatile Storage) pod przestrzenią nazw `"rsvp"`.

| Klucz NVS    | Typ              | Domyślna | Co przechowuje                          |
| ------------ | ---------------- | -------- | --------------------------------------- |
| `ui_lang`    | uint8 (0–5)      | 0        | Język interfejsu                        |
| `dark`       | bool             | true     | Motyw ciemny                            |
| `night`      | bool             | false    | Motyw nocny                             |
| `bright`     | uint8 (0–4)      | 4        | Poziom jasności                         |
| `handed`     | uint8 (0–1)      | 0        | Dłoń (prawy/lewy)                       |
| `prog_md`    | uint8 (0–2)      | 0        | Metryka stopki                          |
| `bat_md`     | uint8 (0–2)      | 0        | Etykieta baterii                        |
| `scrn_sv`    | uint8 (0/2/3/6)  | 0        | Wygaszacz ekranu                        |
| `read_bat`   | bool             | true     | Bateria podczas czytania                |
| `read_ch`    | bool             | false    | Rozdział podczas czytania               |
| `read_pct`   | bool             | false    | Postęp podczas czytania                 |
| `sp_btn`     | bool             | true     | Przycisk zapisu                         |
| `foc_clr`    | uint8 (0–5)      | 0        | Kolor litery fokusowej                  |
| `read_mode`  | uint8 (0–1)      | 0        | Tryb czytnika (RSVP/Scroll)             |
| `pause_md`   | uint8 (0–1)      | 0        | Tryb pauzy                              |
| `wpm`        | uint16 (10–1000) | 300      | Tempo WPM                               |
| `pace_lms`   | uint16 (0–600)   | 200      | Opóźnienie długich słów [ms]            |
| `pace_cms`   | uint16 (0–600)   | 200      | Opóźnienie złożonych słów [ms]          |
| `pace_pms`   | uint16 (0–600)   | 200      | Opóźnienie interpunkcji [ms]            |
| `font_size`  | uint8 (0–2)      | 0        | Rozmiar czcionki                        |
| `typeface`   | uint8 (0–2)      | 0        | Krój czcionki                           |
| `phantom_on` | bool             | true     | Słowa widma                             |
| `type_hlt`   | bool             | true     | Podświetlenie fokusowe                  |
| `type_trk`   | int8 (-2 do +3)  | 0        | Tracking (odstępy liter)                |
| `type_anc`   | uint8 (30–40)    | 30       | Pozycja kotwicy [%]                     |
| `type_wid`   | uint8 (12–30)    | 30       | Szerokość prowadnicy [px]               |
| `type_gap`   | uint8 (2–8)      | 5        | Przerwa prowadnicy [px]                 |
| `wifi_ssid`  | String (max 63)  | ""       | SSID sieci Wi-Fi                        |
| `wifi_pass`  | String (max 63)  | ""       | Hasło Wi-Fi                             |
| `ota_auto`   | bool             | false    | Auto-sprawdzanie aktualizacji           |
| `ota_owner`  | String (max 39)  | ""       | GitHub owner dla OTA                    |
| `dev_mode`   | bool             | false    | Tryb developera                         |
| `ble_on`     | bool             | false    | Bluetooth włączony                      |
| `time_est_a` | bool             | true     | Dokładne szacowanie czasu               |
| `setup_done` | bool             | false    | Wizard pierwszego uruchomienia          |
| `book`       | String           | ""       | Ścieżka ostatniej książki               |
| `seq`        | uint32           | 0        | Licznik kolejności ostatnio otwieranych |

---

## USTAWIENIA Z KARTY SD (PLIK ota.conf)

Plik `/config/ota.conf` lub `/ota.conf` na karcie SD. Format: klucz=wartość, jeden per linia. Używany gdy NVS nie ma własnych ustawień OTA.

| Klucz           | Opis                           |
| --------------- | ------------------------------ |
| `wifi_ssid`     | Sieć Wi-Fi dla OTA             |
| `wifi_password` | Hasło Wi-Fi dla OTA            |
| `github_owner`  | Właściciel repozytorium GitHub |
| `github_repo`   | Nazwa repozytorium GitHub      |
| `asset_name`    | Nazwa pliku .bin do pobrania   |
| `auto_check`    | true/false — auto-sprawdzanie  |

---

## WELCOME WIZARD (pierwsze uruchomienie)

Przy pierwszym uruchomieniu (gdy `setup_done = false`) urządzenie przeprowadza przez 5-ekranowy wizard:

1. **Wybór języka** — identyczny z p.1 (6 języków)
2. **Wybór motywu** — identyczny z p.2 (Light/Dark/Night)
3. **Wybór koloru podświetlenia** — identyczny z p.11 (6 kolorów)
4. **Wybór tempa czytania** — uproszczone: Slow/Medium/Fast (mapowane na konkretne WPM)
5. **Połączenie z telefonem** — opcjonalne, informacje o BLE/Wi-Fi

Po zakończeniu wizard `setup_done` ustawiane na true i wizard nie pokazuje się ponownie.
