#pragma once

#include <Arduino.h>

#ifndef FLOWER_BLE_ENABLED
#define FLOWER_BLE_ENABLED 0
#endif

class App;

/**
 * Minimalne API BLE peripheral dla Flowera. GATT service z dwoma
 * characteristics:
 *
 *   CMD   (write)  — telefon wysyła JSON Lines: {"cmd":"ping"}\n
 *   EVENT (notify) — urządzenie odsyła JSON Lines: {"ev":"pong"}\n
 *
 * UUID-y współdzielone z PWA (`src/shared/config.ts`).
 *
 * Duże transfery (książki, OTA) zostają w torze WiFi (Companion Sync).
 * BLE ~50 kB/s, dla 2 MB firmware to ~40 s — nie ma sensu. BLE służy
 * krótkim komendom: status, settings get/set, toggle dev mode.
 *
 * Cały moduł jest "feature-flagged" — `FLOWER_BLE_ENABLED=0` daje stub
 * z pustymi metodami, dzięki czemu nawet bez NimBLE w lib_deps firmware
 * się zbuduje (przydatne do bisectowania jak coś się posypie).
 */
class BleApi {
 public:
  BleApi();
  ~BleApi();

  /// Inicjalizuje NimBLE stack, rejestruje service, zaczyna advertising.
  /// `app` jest używane do dispatchu komend (np. set theme/lang).
  /// Idempotentne — drugie wywołanie nie robi nic.
  void begin(App *app);

  /// Zatrzymuje advertising + zamyka aktywne połączenia + zwalnia stack.
  void stop();

  /// Wywoływać z głównej pętli (cheap call). Aktualnie no-op — NimBLE
  /// pracuje na własnych taskach. Zostawiamy dla przyszłych potrzeb
  /// (rate-limit notifications, watchdog itp.).
  void update();

  bool isActive() const;
  bool isConnected() const;
  String deviceName() const;  // np. "Flower-A1B2C3"

  /// Wysyła event JSON do połączonego klienta. Bezpieczne wywołanie
  /// nawet bez klienta — wtedy ignoruje.
  void emitEvent(const String &json);

 private:
#if FLOWER_BLE_ENABLED
  struct Impl;
  Impl *impl_;
#endif
};
