#include "WifiQrCode.h"

#include <qrcode.h>

String WifiQrCode::escapeString(const String &str) {
  String escaped = "";
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    // Escapuj specjalne znaki w formacie WiFi QR
    if (c == '\\' || c == ';' || c == ',' || c == ':' || c == '"') {
      escaped += '\\';
    }
    escaped += c;
  }
  return escaped;
}

uint8_t WifiQrCode::generate(const String &ssid, const String &password, bool *qrData,
                             uint8_t maxSize) {
  // Format WiFi QR: WIFI:T:<type>;S:<ssid>;P:<password>;;
  // T - typ zabezpieczenia (WPA, WEP, nopass)
  String wifiString = "WIFI:T:";
  if (password.isEmpty()) {
    wifiString += "nopass";
  } else {
    wifiString += "WPA";
  }
  wifiString += ";S:" + escapeString(ssid) + ";";
  if (!password.isEmpty()) {
    wifiString += "P:" + escapeString(password) + ";";
  }
  wifiString += ";";

  Serial.printf("[QR] WiFi string: %s\n", wifiString.c_str());

  // Utwórz QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];  // Wersja 3 = 29x29 modułów

  int8_t result = qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, wifiString.c_str());

  if (result != 0) {
    Serial.printf("[QR] Failed to generate QR code: %d\n", result);
    return 0;
  }

  uint8_t size = qrcode.size;
  if (size > maxSize) {
    Serial.printf("[QR] QR code too large: %d > %d\n", size, maxSize);
    return 0;
  }

  // Kopiuj dane QR do bufora wyjściowego (stride = size, nie maxSize)
  for (uint8_t y = 0; y < size; y++) {
    for (uint8_t x = 0; x < size; x++) {
      qrData[y * size + x] = qrcode_getModule(&qrcode, x, y);
    }
  }

  Serial.printf("[QR] Generated QR code: %dx%d\n", size, size);
  return size;
}

String WifiQrCode::renderAscii(const bool *qrData, uint8_t size) {
  String result = "";
  for (uint8_t y = 0; y < size; y++) {
    for (uint8_t x = 0; x < size; x++) {
      result += qrData[y * size + x] ? "██" : "  ";
    }
    result += "\n";
  }
  return result;
}
