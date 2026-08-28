#pragma once

#include <Arduino.h>

#ifndef FLOWER_BLE_ENABLED
#define FLOWER_BLE_ENABLED 0
#endif

class App;

/**
 * BleApi v2 — Flower GATT peripheral with chunked framing protocol.
 *
 * Architecture:
 *   Service UUID: f10e7e10-f10e-7e10-f10e-7e10f10e7e10
 *   CMD char (write):  f10e7e11-... — phone → reader, JSON Lines + chunked framing
 *   EVT char (notify): f10e7e12-... — reader → phone, JSON Lines + chunked framing
 *
 * Chunked Framing (1-byte header per BLE packet):
 *   bit 0 (MORE):  1 = more chunks follow, 0 = last chunk
 *   bit 1 (START): 1 = first chunk of new message, 0 = continuation
 *   0x02 = single complete message
 *   0x03 = first of multi-chunk
 *   0x01 = middle chunk
 *   0x00 = last chunk
 *
 * Auth: persistent token in NVS. Phone must send {cmd:auth, token:...}
 * before any other command is accepted.
 *
 * Always-on: BLE advertising runs whenever reader is not in deep sleep.
 * No "Sync mode" required for daily use — only for initial QR pairing.
 */
class BleApi {
 public:
  BleApi();
  ~BleApi();

  /// Initialize NimBLE stack, register service, start advertising.
  /// Idempotent — second call is no-op.
  void begin(App *app);

  /// Stop advertising, disconnect clients, deinit NimBLE.
  void stop();

  /// Must be called from main loop (App::update). Drains queued BLE
  /// commands and processes them in main task context.
  void update();

  bool isActive() const;
  bool isConnected() const;
  bool isAuthenticated() const;
  String deviceName() const;  // e.g. "Flower-A1B2C3"

  /// Send a JSON event to connected+authenticated client.
  /// Handles chunked framing automatically. Safe to call without client.
  void emitEvent(const String &json);

  /// Atomic test-and-clear: has BLE connection state changed?
  bool consumeMenuDirty();

  // --- Token management ---

  /// Generate a new auth token (32 random hex bytes) and save to NVS.
  /// Called when user enters "Pair with phone" screen.
  void generateNewToken();

  /// Get current token (for QR display). Empty if never generated.
  String currentToken() const;

  /// Check if a token has been generated (device has been set up for pairing).
  bool hasToken() const;

  /// Clear token from NVS (factory reset / "forget pairing").
  void clearToken();

  /// Build QR payload string: flower://pair?t=<token>&n=<deviceName>
  String qrPayload() const;

 private:
#if FLOWER_BLE_ENABLED
  struct Impl;
  Impl *impl_;
#endif
};
