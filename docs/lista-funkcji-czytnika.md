# Lista Funkcji Konfigurowalnych w Czytniku Flower

## 1. WYŚWIETLANIE

### 1.1 Motyw (theme)

**Typ:** Segmented Control  
**Możliwe wartości:**

- **Jasny** (light) - jasny motyw interfejsu
- **Ciemny** (dark) - ciemny motyw interfejsu
- **Nocny** (night) - nocny motyw z zwiększonym kontrastem

**Wartość domyślna:** Ciemny

---

### 1.2 Jasność (brightness)

**Typ:** Suwak  
**Zakres:** 10-100 (w krokach co 5)  
**Jednostka:** %  
**Wartość domyślna:** 70%

**Opis:** Reguluje jasność ekranu czytnika. Wartości w interfejsie użytkownika (0-100%) są automatycznie przekształcane na wartości firmware'u (0-4).

---

### 1.3 Dłoń (readerHand)

**Typ:** Segmented Control  
**Możliwe wartości:**

- **Prawa** (right) - układ dla prawej ręki
- **Lewa** (left) - układ dla lewej ręki

**Wartość domyślna:** Prawa

**Opis:** Określa, którą ręką użytkownik trzyma urządzenie, co wpływa na rozmieszczenie elementów sterowania.

---

## 2. CZYTANIE

### 2.1 Tryb czytnika (readerMode)

**Typ:** Segmented Control  
**Możliwe wartości:**

- **RSVP** (rsvp) - Rapid Serial Visual Presentation, słowa wyświetlane pojedynczo
- **Przewijanie** (scroll) - tradycyjne przewijanie tekstu

**Wartość domyślna:** RSVP

---

### 2.2 Zachowanie pauzy (pauseBehaviour)

**Typ:** Segmented Control  
**Możliwe wartości:**

- **Tap** (tap) - pauza poprzez krótkie dotknięcie
- **Przytrzymanie** (long-press) - pauza poprzez długie przytrzymanie
- **Auto** (auto) - automatyczna pauza

**Wartość domyślna:** Tap

**Szczegóły firmware:**

- "auto" → `pauseMode: "instant"`
- "tap" / "long-press" → `pauseMode: "sentence_end"`

---

### 2.3 Tempo czytania (baseWpm)

**Typ:** Suwak  
**Zakres:** 50-1000 (w krokach co 25)  
**Jednostka:** WPM (Words Per Minute - słów na minutę)  
**Wartość domyślna:** 300 WPM

**Opis:** Podstawowa prędkość wyświetlania słów w trybie RSVP.

---

### 2.4 Opóźnienie dla długich słów (longWordDelayMs)

**Typ:** Suwak  
**Zakres:** 0-600 (w krokach co 50)  
**Jednostka:** ms (milisekundy)  
**Wartość domyślna:** 150 ms

**Opis:** Dodatkowy czas wyświetlania dla słów o większej długości, aby dać użytkownikowi więcej czasu na przeczytanie.

---

### 2.5 Opóźnienie dla złożonych słów (complexWordDelayMs)

**Typ:** Suwak  
**Zakres:** 0-600 (w krokach co 50)  
**Jednostka:** ms (milisekundy)  
**Wartość domyślna:** 100 ms

**Opis:** Dodatkowy czas wyświetlania dla słów o złożonej strukturze (np. zawierających myślniki, liczby, itp.).

---

### 2.6 Opóźnienie dla interpunkcji (punctuationDelayMs)

**Typ:** Suwak  
**Zakres:** 0-600 (w krokach co 50)  
**Jednostka:** ms (milisekundy)  
**Wartość domyślna:** 200 ms

**Opis:** Dodatkowy czas pauzy po znakach interpunkcyjnych (kropka, przecinek, wykrzyknik, itp.), symulujący naturalne przerwy w czytaniu.

---

## 3. HUD PODCZAS CZYTANIA

### 3.1 Wyświetlanie baterii (showBatteryWhileReading)

**Typ:** Przełącznik (toggle)  
**Możliwe wartości:**

- **Włączone** (true) - pokazuje poziom baterii podczas czytania
- **Wyłączone** (false) - ukrywa poziom baterii

**Wartość domyślna:** Włączone

---

### 3.2 Wyświetlanie rozdziału (showChapterWhileReading)

**Typ:** Przełącznik (toggle)  
**Możliwe wartości:**

- **Włączone** (true) - pokazuje nazwę bieżącego rozdziału podczas czytania
- **Wyłączone** (false) - ukrywa nazwę rozdziału

**Wartość domyślna:** Włączone

---

### 3.3 Wyświetlanie procentu (showPercentWhileReading)

**Typ:** Przełącznik (toggle)  
**Możliwe wartości:**

- **Włączone** (true) - pokazuje procent ukończenia książki podczas czytania
- **Wyłączone** (false) - ukrywa procent ukończenia

**Wartość domyślna:** Włączone

---

## 4. JĘZYK

### 4.1 Język interfejsu (language)

**Typ:** Lista rozwijana (select)  
**Możliwe wartości:**

- **Polski** (pl)
- **English** (en)
- **Deutsch** (de)
- **Español** (es)
- **Français** (fr)
- **Italiano** (it)

**Wartość domyślna:** Polski

**Opis:** Określa język interfejsu użytkownika aplikacji i urządzenia. Firmware przechowuje język jako indeks (0-5), który jest automatycznie mapowany na kod języka w aplikacji.

---

## 5. TRYB DEVELOPERA (ukryte)

### 5.1 Tryb developera (devMode)

**Typ:** Przełącznik (toggle)  
**Możliwe wartości:**

- **Włączone** (true) - odblokowuje zaawansowane funkcje
- **Wyłączone** (false) - ukrywa zaawansowane funkcje

**Wartość domyślna:** Wyłączone

**Sposób aktywacji:** 10-krotne kliknięcie na logo "Flower" w panelu ustawień

**Opis:** Ukryta funkcja dla developerów i zaawansowanych użytkowników. Po włączeniu:

- Odblokowuje dostęp do aktualizacji OTA (Over-The-Air)
- Pokazuje zaawansowane ustawienia w aplikacji
- Umożliwia edycję RSS feeds
- Daje dostęp do innych funkcji deweloperskich

Po wyłączeniu trybu wszystkie zaawansowane opcje znikają zarówno z urządzenia, jak i z aplikacji.

---

## PODSUMOWANIE

**Łącznie:** 14 konfigurowalnych funkcji  
**Funkcje podstawowe (widoczne zawsze):** 13  
**Funkcje ukryte (wymagające odblokowania):** 1

### Podział według typu kontrolki:

- **Przełączniki (toggle):** 4 funkcje
- **Suwaki (slider):** 5 funkcji
- **Segmented controls:** 4 funkcje
- **Lista rozwijana:** 1 funkcja

### Podział według kategorii:

- **Wyświetlanie:** 3 funkcje
- **Czytanie:** 6 funkcji
- **HUD:** 3 funkcje
- **Język:** 1 funkcja
- **Developer:** 1 funkcja (ukryta)
