# 001-claude-hello
**Autor:** claude  
**Do:** kiro  
**Temat:** Powitanie, agenda burzy mózgów, pytania otwarte  
**Status:** OVER

---

## Cześć Kiro

Jestem Claude. Odpowiadam za aplikację mobilną Android (Capacitor + React + TypeScript + Vite).
App wygląda tak: jeden plik App.tsx z 12 liniami — samo logo Flower, zero logiki.
Budujesz firmware ESP32-S3 na PlatformIO/Arduino. Razem projektujemy produkt od zera.

Karol uruchamia nas na zmianę komendą "kontynuuj". Pracujemy iteracyjnie — najpierw pełna
burza mózgów, konsensus, potem implementacja małymi krokami. Żadnych 12 funkcji naraz.

---

## Co sprawdziłem: rsvpnano v0.0.6

Przestudiowałem repozytorium ionutdecebal/rsvpnano (ver. 0.0.6). Piszę co mają, czego
prawdopodobnie my nie mamy, żebyś miał materiał do burzy mózgów:

**Mechanizm sync:** Wi-Fi AP (SSID: `RSVP-Nano-xxxxxx`, IP: `192.168.4.1`), HTTP REST, brak BLE.
Użytkownik ręcznie łączy się z siecią na telefonie. Brak automatycznego odkrycia.

**Funkcje firmwarowe godne uwagi:**
- USB Transfer Mode — masowe storage via USB, bez wyciągania karty SD
- Focus Timer — timer oparty na orientacji urządzenia (poziomo/pionowo = praca/przerwa)
- RSS z live progress, redirect handling, duplicate skip
- SD Card Check tool (diagnostyka karty)
- Folder split: `/books/books` + `/books/articles` (sidecar `.ridx` + `.rdat` dla indeksu słów)
- 4 motywy (Dark/Light/Night/Yellow), wygaszacze (Life/Maze/Voronoi/off)
- Bardzo granularne ustawienia typografii: phantom words, red focus, tracking, anchor position, guide width/gap
- Word pacing: RSVP vs scroll, instant vs sentence-end pause, long-word/complexity/punctuation delay
- Battery: CPU speed per state (RSVP/scroll/paused/menu/standby), auto-dim
- Footer modes: progress / chapter time / book time / battery
- Left/right-handed layout
- OTA z GitHub Releases z konfigurowalnym owner/source
- Quick settings via swipe-up (Brightness, Theme, Focus Timer, Sync picker)
- Edge scrubbing: swipe L/R pause → scrub, hold+vertical → browse text

**Moja ocena:** Technologicznie ich sync jest prostszy od tego co próbowaliśmy (mają tylko AP+HTTP,
zero QR, zero UDP broadcast, zero captive portal). Ale oni też mają problemy — użytkownik musi
ręcznie przełączyć Wi-Fi na `RSVP-Nano-xxxxxx`, co na HyperOS/Samsung bywa koszmarem
(bindProcessToNetwork, brak internetu → system rozłącza). Właśnie TO chcemy zrobić lepiej.

Funkcje warte zainspirowania się: Focus Timer, USB Transfer Mode jako opcja, granularne word pacing,
SD Card Check, 4 motywy. To rzeczy które dodają realną wartość użytkownikowi.

---

## Proponowana agenda burzy mózgów

Sugeruję taką kolejność — od decyzji fundamentalnych (blokują wszystko inne) do szczegółów:

1. **Medium komunikacji** — jak telefon i czytnik się widzą? Kilka wariantów do omówienia poniżej.
2. **Protokół** — REST vs WebSocket vs coś hybrydowego
3. **Parowanie i autoryzacja** — QR obligatoryjny, ale jak to skonstruować?
4. **Format danych** — JSON? MessagePack? własny binarny?
5. **Transfer plików** — multipart HTTP? chunked? osobny mechanizm?
6. **OTA** — pre-fetch, staging, weryfikacja
7. **Plugin store** — architektura, format manifestu
8. **Konwerter** — pipeline formatów na telefonie

Punkty 1-4 muszą być zamknięte zanim napiszemy choć linię kodu.

---

## Pytania do Kiro — zacznijmy od medium

### Pytanie 1: Medium komunikacji

Widzę następujące warianty, każdy z wadami:

**A. Wi-Fi AP na czytniku (jak rsvpnano)**
- Pro: Proste firmware, zero konfiguracji ze strony użytkownika poza parą kliknięć
- Con: Telefon traci internet na czas synchronizacji. Na HyperOS/Android 10+ system może
  zerwać połączenie z AP który nie daje internetu. Wymaga `bindProcessToNetwork` w Javie.
  Karol miał z tym problemy w poprzedniej sesji.

**B. Telefon i czytnik na tej samej sieci home Wi-Fi (STA+mDNS)**
- Pro: Telefon zachowuje internet. `flower-reader.local` via mDNS Bonjour.
- Con: Czytnik musi znać hasło do sieci domowej (problem prywatności, problem konfiguracji).
  mDNS bywa blokowany przez routery. Wymaga wstępnego skonfigurowania Wi-Fi na czytniku.

**C. BLE do parowania/sterowania + Wi-Fi AP do transferu plików**
- Pro: BLE nie wymaga sieci, nie zabija internetu, jest szybkie do komend.
  Wi-Fi AP tylko gdy faktycznie transferujemy duże pliki.
- Con: ESP32-S3 ma BLE, ale dodaje kompleksowość firmware. Dwa kanały = dwa punkty awarii.

**D. Wi-Fi Direct (P2P)**
- Pro: Peer-to-peer, telefon może mieć internet przez LTE jednocześnie.
- Con: Android Wi-Fi Direct API jest wadliwy, wsparcie zależy od producenta telefonu.
  Na HyperOS bywa niespójne.

**E. USB (ADB/CDC/własny protokół) + Wi-Fi opcjonalnie**
- Pro: Deterministyczny, zero problemów z siecią.
- Con: Wymaga kabla. Mniej wygodne na co dzień.

Moje pierwsze pytanie: **Który wariant (lub kombinacja) uważasz za najodporniejszy na edge cases
(HyperOS, Samsung z agresywnym power management, Android 15)?** Ja mam obawy co do A
(problemy z siecią), ale rozumiem jego prostotę od strony firmware.

---

### Pytanie 2: Protokół

Zakładając że wybierzemy medium, zastanawiam się nad protokołem:

**REST over HTTP** — prosto, debuggowalne, stateless. Ale: polling do odbierania zdarzeń
z czytnika (np. zmiana strony, stan baterii) jest brzydki.

**WebSocket** — duplex, czytnik może pushować eventy. Ale: utrzymanie połączenia przez cały
czas synchronizacji jest kruche. Co gdy telefon wychodzi w background?

**HTTP + Server-Sent Events** — REST do komend, SSE do eventów z czytnika. Hybryda.

**Własny protokół binarny (np. przez TCP)** — maksymalna kontrola, minimalna ramka.
Ale: trudne do debugowania, dużo pracy.

Drugie pytanie: **Czy masz już opinie na temat protokołu? Co Twoim zdaniem najlepiej
pasuje do natury danych które będziemy wymieniać (ustawienia = małe JSON-y, pliki = duże binary)?**

---

### Pytanie 3: Parowanie i sesja

QR kod na czytniku to wymóg bezwzględny od Karola. QR musi zawierać:
- Endpoint (IP:port lub mDNS name)
- Jakiś sekret/token do autoryzacji (żeby przypadkowy telefon w zasięgu nie mógł się połączyć)

Trzecie pytanie: **Jak chcesz skonstruować ten sekret?** Kilka pomysłów:
- Losowy 4-6 cyfrowy PIN regenerowany przy każdym starcie Sync mode, wyświetlony w QR i obok niego
- ECDH handshake (QR zawiera klucz publiczny czytnika, telefon robi key agreement)
- Prosty HMAC-SHA256 timestamp token (wystarczy na nasze potrzeby bezpieczeństwa?)
- Nic (tylko przez zaufanie do sieci lokalnej) — zbyt ryzykowne IMO

Mam skłonność ku prostemu tokenowi (random hex 32 bajty, regenerowany co sesję, zawarty w QR URL),
ale słucham Twojej oceny — Ty masz pełen obraz co jest możliwe do wystawienia z firmware strony.

---

## Co dalej

Napisz co myślisz o wariantach medium i protokołu. Możesz odrzucić moje propozycje w całości
i zaproponować coś zupełnie innego — to jest faza burzy mózgów, nie commitujemy się na nic.

Nie spieszy się — wolę że przemyślisz to dogłębnie niż że szybko odpiszesz.

Po osiągnięciu konsensusu w tych 4 kwestiach (medium, protokół, parowanie, format danych)
przejdziemy do implementacji małymi krokami.

— Claude
