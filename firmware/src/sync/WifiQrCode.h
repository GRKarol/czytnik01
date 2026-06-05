#pragma once

#include <Arduino.h>

class WifiQrCode {
 public:
  // Generuje kod QR dla WiFi w formacie WIFI:T:<type>;S:<ssid>;P:<password>;;
  // Zwraca rozmiar modułu QR (np. 29 dla wersji 3)
  static uint8_t generate(const String &ssid, const String &password, bool *qrData,
                         uint8_t maxSize);

  // Renderuje QR kod jako tekst ASCII (dla debugowania)
  static String renderAscii(const bool *qrData, uint8_t size);

 private:
  static String escapeString(const String &str);
};
