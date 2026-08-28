# Flower Communication Protocol v2

Kanoniczny dokument architektury komunikacji między czytnikiem Flower (ESP32-S3) a aplikacją mobilną Android.

**Status:** Ustalony (burza mózgów zamknięta 2026-06-16)
**Autorzy:** Kiro (firmware), Claude (app Android)

---

## Architektura — dwie warstwy

```
┌─────────────────────────────────────────────────┐
│              Aplikacja Android                   │
│  ┌───────────────────────────────────────────┐  │
│  │  BLE GATT Client (always-on)              │  │  ← komendy, status, settings
│  │  WiFi HTTP Client (on-demand burst)       │  │  ← upload plików, OTA
│  └───────────────────────────────────────────┘  │
└──────────────────────┬──────────────────────────┘
                       │
    BLE (persistent)   │   WiFi AP (burst, sekundy)
                       │
┌──────────────────────┴──────────────────────────┐
│              Czytnik ESP32-S3                    │
│  ┌───────────────────────────────────────────┐  │
│  │  BLE GATT Server (NimBLE, always-on)      │  │
│  │  WiFi AP + HTTP Server (on-demand)        │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

---

## Warstwa 1: BLE GATT

### Identyfikatory

| Element        | UUID                                   |
| -------------- | -------------------------------------- |
| Service        | `f10e7e10-f10e-7e10-f10e-7e10f10e7e10` |
| CMD (write)    | `f10e7e11-f10e-7e10-f10e-7e10f10e7e10` |
| EVENT (notify) | `f10e7e12-f10e-7e10-f10e-7e10f10e7e10` |

### Zachowanie

- BLE advertising jest ZAWSZE aktywne gdy czytnik nie jest w deep sleep
- Nie ma "trybu Sync" — czytnik jest zawsze odkrywalny i dostępny
- Device name: `Flower-XXXXXX` (6 hex z MAC)
- Advertising interval: 320ms (oszczędność baterii)
- TX power: -9 dBm (zasięg ~5-10m, wystarczający w mieszkaniu)
- MTU: negotiate 512 po połączeniu

### Chunked Framing Protocol

Zarówno CMD (write) jak i EVENT (notify) używają chunked framing gdy payload > MTU-4.

Każdy chunk BLE (notify lub write) ma format:

```
[1 bajt: flags][N bajtów: payload]
```

Flags:

- bit 0 (MORE): 1 = następny chunk jest kontynuacją, 0 = to ostatni chunk
- bit 1 (START): 1 = pierwszy chunk nowej wiadomości, 0 = kontynuacja
- bit 2-7: reserved (0)

Kombinacje:
| flags | hex | znaczenie |
|-------|-----|-----------|
| START=1, MORE=0 | 0x02 | Jednopacketowa wiadomość (kompletna) |
| START=1, MORE=1 | 0x03 | Pierwszy chunk (będą następne) |
| START=0, MORE=1 | 0x01 | Środkowy chunk (będą następne) |
| START=0, MORE=0 | 0x00 | Ostatni chunk (reassembly complete) |

Reassembly:

1. Odbierz chunk z START=1 → zacznij nowy bufor
2. Dopisuj payload kolejnych chunków
3. Chunk z MORE=0 → bufor kompletny → parsuj jako JSON Line
4. Jeśli otrzymasz START=1 w trakcie reassembly → discard stary bufor, zacznij od nowa (desync recovery)

### Dane: JSON Lines

Każda wiadomość to jeden JSON object zakończony `\n`:

```
{"cmd":"get-settings"}\n
{"ev":"settings","data":{...}}\n
```

### Autoryzacja

Token: 32 bajty random hex, persistent w NVS czytnika.

```
App → Czytnik: {"cmd":"auth","token":"a1b2c3...64chars"}\n
Czytnik → App: {"ev":"auth-ok","api":2}\n
         lub: {"ev":"auth-fail","reason":"invalid-token"}\n
         lub: {"ev":"auth-fail","reason":"not-paired"}\n
```

Po `auth-ok` sesja jest autoryzowana. Komendy bez auth zwracają `{ev:auth-required}`.

### Komendy (App → Czytnik)

| Komenda        | Payload                              | Opis                         |
| -------------- | ------------------------------------ | ---------------------------- |
| `auth`         | `{token}`                            | Autoryzacja sesji            |
| `get-settings` | —                                    | Pobierz wszystkie ustawienia |
| `set-settings` | `{data:{key:val,...}}`               | Zmień ustawienia (częściowe) |
| `get-books`    | —                                    | Lista książek z chapters     |
| `get-status`   | —                                    | Status: bateria, SD, wersja  |
| `start-wifi`   | `{reason:"upload"\|"ota"\|"plugin"}` | Uruchom WiFi AP burst        |
| `stop-wifi`    | —                                    | Wyłącz WiFi AP               |
| `upload-begin` | `{name:"file.epub",size:12345}`      | Rozpocznij upload pliku BLE  |
| `upload-chunk` | `{d:"<base64 ~8KB>"}`                | Chunk danych pliku           |
| `upload-end`   | —                                    | Zakończ upload pliku         |
| `reboot`       | —                                    | Restart czytnika             |
| `ping`         | —                                    | Heartbeat                    |

### Eventy (Czytnik → App)

**Odpowiedzi na komendy:**

| Event             | Payload                                               | W odpowiedzi na          |
| ----------------- | ----------------------------------------------------- | ------------------------ |
| `auth-ok`         | `{api:2}`                                             | auth                     |
| `auth-fail`       | `{reason:...}`                                        | auth                     |
| `settings`        | `{data:{...full settings...}}`                        | get-settings             |
| `settings-ok`     | —                                                     | set-settings             |
| `books`           | `{data:[...book list...]}`                            | get-books                |
| `upload-ready`    | `{name,size}`                                         | upload-begin             |
| `upload-progress` | `{received,percent}`                                  | upload-chunk             |
| `upload-complete` | `{name,bytes}`                                        | upload-end               |
| `upload-error`    | `{reason:...}`                                        | upload-\*                |
| `status`          | `{battery, sdFree, sdTotal, version, wpm, book, ...}` | get-status               |
| `wifi-ready`      | `{ssid, pass, ip}`                                    | start-wifi               |
| `wifi-stopped`    | —                                                     | stop-wifi                |
| `pong`            | `{ts}`                                                | ping                     |
| `error`           | `{reason:...}`                                        | dowolna komenda z błędem |

**Spontaniczne eventy (bez komendy):**

| Event              | Payload                       | Kiedy                               |
| ------------------ | ----------------------------- | ----------------------------------- |
| `battery`          | `{percent:N}`                 | Co 60s, przy zmianie >2%            |
| `settings-changed` | `{data:{...changed keys...}}` | User zmienił ustawienie na czytniku |
| `position`         | `{word:N, chapter:C}`         | User przeszedł do innej pozycji     |
| `ota-progress`     | `{percent:N}`                 | Co 10% podczas flash OTA            |

---

## Warstwa 2: WiFi AP Burst

### Triggerowanie

WiFi AP jest uruchamiane TYLKO na żądanie app (przez BLE komendę `start-wifi`).

### Parametry AP

| Parametr       | Wartość                                                              |
| -------------- | -------------------------------------------------------------------- |
| SSID           | `Flower-XXXXXX` (ten sam co BLE device name)                         |
| Hasło          | 8 losowych znaków (generowane per-burst, dostarczone w `wifi-ready`) |
| IP czytnika    | `192.168.4.1` (stałe)                                                |
| DNS server     | BRAK (app łączy się po IP)                                           |
| Timeout        | 120s od startu (auto-shutdown)                                       |
| Captive portal | `/generate_204` → 204, `/hotspot-detect.html` → 200                  |

### Połączenie ze strony Android

```java
WifiNetworkSpecifier specifier = new WifiNetworkSpecifier.Builder()
    .setSsid(ssid)
    .setWpa2Passphrase(password)
    .build();

NetworkRequest request = new NetworkRequest.Builder()
    .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
    .addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED)
    // BEZ NET_CAPABILITY_INTERNET — wyłącza captive portal detection
    .setNetworkSpecifier(specifier)
    .build();

connectivityManager.requestNetwork(request, callback, 30_000);
// OkHttpClient z network.getSocketFactory()
```

### Endpointy HTTP

| Endpoint                    | Metoda                     | Opis                  |
| --------------------------- | -------------------------- | --------------------- |
| `POST /api/books`           | multipart, pole `file`     | Upload .rsvp          |
| `POST /api/ota`             | multipart, pole `firmware` | Upload firmware .bin  |
| `POST /api/plugins/install` | multipart, pole `package`  | Upload plugin package |
| `GET /generate_204`         | —                          | Captive portal: 204   |
| `GET /hotspot-detect.html`  | —                          | Captive portal: 200   |

---

## Parowanie

### Pierwsze parowanie

1. User na czytniku: Menu → "Sparuj z telefonem"
2. Czytnik generuje token (32B random hex), zapisuje w NVS
3. Czytnik wyświetla QR:
   ```
   flower://pair?t=<token_hex_64chars>&n=Flower-A1B2C3
   ```
4. App skanuje QR kamerą
5. App parsuje URL, wyciąga `t` (token) i `n` (device name)
6. App robi BLE scan filtrowany po `localName == n`
7. App connect → subscribe EVT CCCD → send `{cmd:auth, token:t}`
8. Czytnik sprawdza token vs NVS → `{ev:auth-ok}`
9. App zapisuje `t` i `n` w Android Keystore

### Reconnect (każde kolejne użycie)

1. App startuje
2. App czyta `n` i `t` z Keystore
3. App BLE scan po `n` → connect → auth z `t`
4. ~3-5s total

### Reset parowania

- User na czytniku: "Zapomnij sparowanie"
- Czytnik generuje nowy token, nadpisuje w NVS
- Stary token nieważny → app dostaje `{ev:auth-fail}` → "Zaparuj ponownie"

### Bezpieczeństwo

- Token wymagany dla KAŻDEJ sesji BLE (nie ma "free" komend poza auth)
- BLE plaintext (bez szyfrowania warstwy link) — akceptowalny risk dla czytnika
- WiFi AP zabezpieczone WPA2 z losowym hasłem per-burst
- Threat model: fizyczny dostęp do QR = dostęp do czytnika (akceptowalne)

---

## Format pliku .rsvp

Bez zmian vs v1. Patrz: `docs/flower-companion-api.md` sekcja "Format pliku .rsvp".

---

## Settings JSON

Pełna lista ustawień i ich zakresów: patrz `docs/flower-companion-api.md` sekcja "Szczegóły ustawień".

Settings przesyłane przez BLE (`get-settings` / `set-settings`) mają identyczny format jak stary `GET /api/settings` / `PATCH /api/settings`.

---

## Wersjonowanie protokołu

Pole `api` w `{ev:auth-ok, api:N}`:

- `api:1` — stara architektura (WiFi AP + HTTP REST only, deprecated)
- `api:2` — nowa architektura (BLE primary + WiFi burst)

App powinna sprawdzić `api` po auth i zachować backward compat z `api:1` jeśli kiedyś się połączy ze starym firmware (unlikely ale possible).

---

## Changelog

- 2026-06-16: v2 — pełny redesign. BLE primary + WiFi burst. Token auth. Chunked framing.
