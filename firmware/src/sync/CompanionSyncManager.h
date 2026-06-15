#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <FS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFiUdp.h>

class CompanionSyncManager {
 public:
  struct Config {
    String wifiSsid;
    String wifiPassword;
  };

  bool begin(const Config &config);
  void update();
  void end();
  bool active() const;
  String statusLine1() const;
  String statusLine2() const;
  String baseUrl() const;
  bool hasQrCode() const;
  const bool *qrCodeData() const;
  uint8_t qrCodeSize() const;

  /** Set device status values (called from App on each update cycle). */
  void setDeviceStatus(uint8_t batteryPercent, uint32_t sdFreeKb, uint32_t sdTotalKb);

 private:
  enum class NetworkMode : uint8_t {
    None,
    Station,
    AccessPoint,
  };

  struct RsvpMetadata {
    String title;
    String author;
  };

  struct RsvpChapter {
    String title;
    size_t startWord = 0;
  };

  static void handleInfoStatic();
  static void handleHelloStatic();
  static void handleRootStatic();
  static void handleBooksListStatic();
  static void handleSettingsStatic();
  static void handleWifiStatic();
  static void handleRssFeedsStatic();
  static void handleBookDeleteStatic();
  static void handleBooksStatic();
  static void handleBookUploadStatic();
  static void handleOtaStatic();
  static void handleOtaUploadStatic();
  static void handleCapabilitiesStatic();
  static void handlePluginsStatic();
  static void handlePluginsDeleteStatic();
  static void handlePowerWifiTimeoutStatic();
  static void handleOptionsStatic();
  static void handleNotFoundStatic();
  static void handleStateStatic();
  static void handleLogTailStatic();
  static void handleLangCodesStatic();
  static void handleBookPositionStatic();
  static void handleLogClearStatic();

  bool startAccessPoint();
  bool startServer();
  void stopServer();
  void handleInfo();
  void handleHello();
  void handleRoot();
  void handleBooksList();
  void handleSettings();
  void handleWifi();
  void handleRssFeeds();
  void handleBookDelete();
  void handleBooks();
  void handleBookUpload();
  void handleOta();
  void handleOtaUpload();
  void handleCapabilities();
  void handlePlugins();
  void handlePluginsDelete();
  void handlePowerWifiTimeout();
  void handleOptions();
  void handleNotFound();
  void handleState();
  void handleLogTail();
  void handleLangCodes();
  void handleBookPosition();
  void handleLogClear();
  String settingsJson();
  bool applySettingsJson(const String &body, String &error);
  String wifiJson();
  bool applyWifiJson(const String &body, String &error);
  String rssFeedsJson();
  bool writeRssFeedsJson(const String &body, String &error);
  String deviceSuffix() const;
  String jsonEscape(const String &value) const;
  String sanitizeFilename(const String &name) const;
  RsvpMetadata readRsvpMetadata(const String &path) const;
  std::vector<RsvpChapter> readRsvpChapters(const String &path) const;
  bool progressPercentForPath(const String &path, uint8_t &percent);
  String bookPositionKey(const String &bookPath) const;
  String bookWordCountKey(const String &bookPath) const;
  uint32_t hashBookPath(const String &path) const;
  void finishUpload(bool success);
  void sendCorsHeaders();

  static CompanionSyncManager *instance_;

  DNSServer dnsServer_;
  WiFiUDP udpBroadcast_;
  uint32_t lastBroadcastMs_ = 0;
  WebServer server_{80};
  File uploadFile_;
  String uploadFinalPath_;
  String uploadTmpPath_;
  String uploadError_;
  String otaError_;
  String pairingCode_;
  String networkSsid_;
  Preferences preferences_;
  String statusLine1_ = "Idle";
  String statusLine2_;
  NetworkMode networkMode_ = NetworkMode::None;
  bool active_ = false;
  bool serverStarted_ = false;
  bool qrData_[64 * 64] = {};  // Bufor dla QR kodu (max 64x64 moduły)
  uint8_t qrSize_ = 0;
  uint32_t wifiTimeoutMs_ = 0;  // 0 = brak timeoutu

  // Device status (updated from App)
  uint8_t deviceBatteryPercent_ = 0;
  uint32_t deviceSdFreeKb_ = 0;
  uint32_t deviceSdTotalKb_ = 0;

  // Ring buffer for firmware log tail
  static constexpr size_t kLogRingSize = 100;
  String logRing_[kLogRingSize];
  size_t logRingHead_ = 0;
  size_t logRingCount_ = 0;

 public:
  /** Append a line to the internal log ring buffer (call from anywhere). */
  void logLine(const String &line);
};
