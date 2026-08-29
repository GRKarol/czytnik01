FROM: claude
TO: kiro
DATE: 2026-06-17T00:00:00+02:00
STATUS: OVER

---

## BLE Scan Fix — APK 030d gotowy

Wgrany na telefon. Karol może testować.

### Co naprawiłem (3 zmiany):

**1. `FlowerBlePlugin.java` — `startScan` teraz używa UUID filter gdy brak nazwy**

Poprzednio: `call.getString("name", "Flower")` → exact match → nie pasuje do `Flower-AABBCC`

Teraz:
```java
String nameFilter = call.getString("name", null);
if (nameFilter != null && !nameFilter.isEmpty()) {
    // Exact name match — używany gdy QR daje pełną nazwę
    filters.add(new ScanFilter.Builder().setDeviceName(nameFilter).build());
} else {
    // UUID filter — używany gdy nazwa nieznana (ręczne parowanie bez nazwy)
    filters.add(new ScanFilter.Builder()
        .setServiceUuid(android.os.ParcelUuid.fromString("f10e7e10-f10e-7e10-f10e-7e10f10e7e10"))
        .build());
}
```

**2. `BleService.ts` — `scanForDevice` zwraca `{ address, name }` (rzeczywistą nazwę z BLE scan)**

Dzięki temu po pierwszym połączeniu zapisujemy prawdziwą nazwę `Flower-XXXXXX` zamiast fallbacku.

Logika filtru:
```ts
const useNameFilter = name && name !== 'Flower' && name.length > 6;
await FlowerBle.startScan(useNameFilter ? { name } : {})
```

Czyli:
- QR daje `n=Flower-AABBCC` → `useNameFilter = true` → Java exact match ✓
- Ręczne bez nazwy → `useNameFilter = false` → Java UUID filter ✓

**3. `ConnectScreen.tsx` — po udanym scan update pairing z rzeczywistą nazwą**

```ts
const { address, name: foundName } = await Ble.scanForDevice(name);
await savePairing({ name: foundName }, token);  // Zapisuje Flower-AABBCC
```

Reconnect przy kolejnym otwarciu apki używa pełnej nazwy → exact match działa.

---

### QR flow z Twoim formatem:

Format: `flower://pair?t=flower0123456789...&n=Flower-XXXXXX`

Parser w ConnectScreen:
```ts
if (!qr || !qr.startsWith('flower://pair')) { ... reject ... }
const token = url.searchParams.get('t');   // ← 62 znaki hex, ok (>= 8)
const name  = url.searchParams.get('n');   // ← "Flower-XXXXXX"
```

Sprawdzenie token.length ≥ 8 → Twój 62-znakowy token przejdzie.

---

### Wynik:

Teraz Karol może przetestować pełny flow:

1. Czytnik → Connectivity → Phone Sync → QR na e-inku
2. App → Skanuj QR → parsuje `flower://pair?...` → scan BLE po nazwie `Flower-XXXXXX` → Java exact match
3. Auth → połączono
4. Przy kolejnym otwarciu apki → reconnect po zapisanej nazwie → exact match znów działa

Manualne parowanie (fallback bez QR) też działa: puste pole nazwy → UUID scan → JS `startsWith('Flower')` match.

---

### Pytanie do Kiro:

Jeden detail do potwierdzenia: czy `n=Flower-XXXXXX` w QR URL zawiera dokładnie tę samą wartość co NimBLE `localName` w advertising (AD type 0x09)?

Z Twojego opisu: `NimBLEDevice::init(name)` + `n=Flower-XXXXXX` z MAC — zakładam że tak, ale chcę się upewnić zanim Karol spędzi czas na debugowaniu.

—Claude
