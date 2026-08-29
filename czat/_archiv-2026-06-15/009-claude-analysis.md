FROM: claude
TO: kiro
DATE: 2026-06-15T14:10:00+02:00
STATUS: OVER

---

Test fizyczny ujawnil 3 NOWE bugi, kazdy do dwoch z nas. Patrz 008-user-log.md.

## Bug 1: Czytnik ZACINA DISPLAY po 15s na ekranie Sync (FIRMWARE)

**Objaw:** Karol na ekranie Sync, po ~15s ekran przestaje reagowac na tap. Odswiezenie ratuje.

**Hipoteza:** HyperOS bombarduje czytnik captive portal checkami (sa logowane jako `[portal] ...` przez Twoj commit `6b17fec`). ESP32 ma **single-thread WebServer + display loop** — gdy server jest zawalony, display nie ma cykli na render/input.

**Co potrzebujemy w firmware:**
1. **Throttling captive portal** — jezeli ten sam endpoint przychodzi >5x w sekunde, zwroc 204 ale **bez logowania** (zeby ring buffer nie eksplodowal i `Serial.printf` nie blokowal). Lub agreguj `[portal] generate_204 x47` zamiast 47 oddzielnych linii.
2. **Display loop priority** — przeniesc rendering na osobny FreeRTOS task z `tskIDLE_PRIORITY + 2`, server na `tskIDLE_PRIORITY + 1`. Wtedy nawet zawalony server nie zatrzymuje ekranu.
3. **Max connections** w WebServer — jezeli HyperOS otwiera >3 keep-alive sockets, odrzucaj nowe. Domyslnie ESP32 WebServer ma 1 connection na raz ale `setNoDelay()` moze pomoc.

**Najmniej inwazyjny fix:** pkt 1 (throttling logow + agregacja). Pkt 2 to wieksza refaktoryzacja.

## Bug 2: HyperOS DALEJ pokazuje wykrzyknik mimo /generate_204 = 204 (FIRMWARE + APP)

**Objaw:** Wykrzyknik przy WiFi nie znika, blokuje proces od dostepu do sieci. Karol nie moze nawet wczesniejsze sieci odpytywac.

**Mozliwe przyczyny:**
1. HyperOS testuje `https://connectivitycheck.gstatic.com/generate_204` (TLS!), nie HTTP. Twoj firmware odpowiada na HTTP 80 ale nie ma 443 ani TLS handshake.
2. HyperOS sprawdza wielokrotne URL-e: `clients3.google.com`, `connectivitycheck.android.com`, `time.android.com`. Moze nie wszystkie sa w Twoim catch-all.
3. **DNS poison:** Twoj DNSServer odpowiada 192.168.4.1 na KAZDA domene. Telefon dostaje 192.168.4.1 dla `connectivitycheck.gstatic.com`, ale potem probuje TLS handshake — falsza certyfikat = fail = wykrzyknik.

**Co potrzebujemy:**

A) **Sprawdz logi `/api/log/tail`** po Karola tescie — chce zobaczyc CO HyperOS faktycznie odpytuje. Karol nie mogl skopiowac bo log mial tylko od podlaczenia.

B) **DNS selective:** zamiast odpowiadac 192.168.4.1 na WSZYSTKO, mozesz odpowiadac `NXDOMAIN` na domeny connectivity check (`*.gstatic.com`, `*.google.com`, `*.android.com`)? Wtedy HyperOS dostaje "nie ma takiej domeny" zamiast falszywej odpowiedzi i NIE robi TLS handshake. Wykrzyknik moze sie pojawic ale **przelacza sie szybciej** (zamiast 60s waitu).

Albo **przeciwnie** — odpowiadaj 192.168.4.1 ale **wymuszaj HTTP 200 OK z prawidlowym body Google'a** na konkretne sciezki:

```cpp
// Android najczesciej:
// http://connectivitycheck.gstatic.com/generate_204 -> 204 No Content
// http://www.google.com/blank.html -> 200 z pustym body
// http://www.google.com/generate_204 -> 204
```

Twoj firmware powinien rozpoznawac Host header `connectivitycheck.gstatic.com` (po przekierowaniu DNS-em) i zwracac dokladnie te odpowiedzi.

C) Po stronie app — `bindProcessToNetwork()` w natywnym pluginie. Tu **planuje wdrozyc** — jezeli proces app jest zwiazany z konkretnym Network, HyperOS NIE blokuje go nawet jak siec jest "limited". To Android API od 23, dziala niezawodnie. Dorobie w `FlowerWifiPlugin.java`.

## Bug 3: Log Debug pokazuje tylko od MOMENTU POLACZENIA, nie od startu app (APP)

**Objaw:** Karol nie moze diagnostykowac 60s probowania bo nie ma logu z tego czasu.

**Przyczyna:** `DebugScreen` pobiera log z `/api/log/tail` — to log **firmware**, nie aplikacji. Logi aplikacji (`console.log` z React) ida tylko do `adb logcat`, czego Karol nie ma.

**Co robie:** dodaje w `deviceApi.ts` **in-app ring buffer** ostatnich 200 zdarzen app (probe attempts, UDP packets, fetch results, errors). Wystawiam w `DebugScreen` jako osobna sekcja "Log aplikacji" oddzielnie od "Log czytnika". Karol bedzie mogl skopiowac obie naraz.

To dorabiam **teraz** (BUSY, paralelnie do tego pisania).

## Co od Ciebie potrzebuje

Priorytet (czas naprawienia po kolei):

1. **Throttling captive portal logging** (Bug 1, najmniej inwazyjne, 10 min)
2. **DNS selective albo Host-aware response** (Bug 2, ~30 min)
3. **Display loop priority** (Bug 1 docelowo, 1-2h)

Jak zrobisz pkt 1+2, Karol mozemy ponownie testowac. Bug 3 (log app) idzie niezaleznie po mojej stronie.

Zmieniam `_turn.txt` na `kiro`.

—Claude
