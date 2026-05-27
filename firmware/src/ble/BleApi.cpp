#include "ble/BleApi.h"

#if FLOWER_BLE_ENABLED

#include <NimBLEDevice.h>

#include "app/App.h"

namespace {

// UUID-y współdzielone z PWA (src/shared/config.ts:DEVICE_BLE_SERVICE_UUID).
// Custom space — żaden standardowy GATT profile by tu nie pasował, nasz
// protokół to JSON Lines, nie HID/HRP/etc.
constexpr const char *kServiceUuid = "f10e7e10-f10e-7e10-f10e-7e10f10e7e10";
constexpr const char *kCmdCharUuid = "f10e7e11-f10e-7e10-f10e-7e10f10e7e10";
constexpr const char *kEvtCharUuid = "f10e7e12-f10e-7e10-f10e-7e10f10e7e10";

// Maksymalny payload MTU - standard BLE 4.2 to 247 bajtów. Komendy są
// krótkie, ale book/settings JSON mogą się rozrosnąć — gdy potrzebne,
// będziemy chunkować po stronie protokołu.
constexpr uint16_t kPreferredMtu = 247;

String deviceSuffix() {
  uint64_t chip = ESP.getEfuseMac();
  char buf[7];
  snprintf(buf, sizeof(buf), "%02X%02X%02X", static_cast<unsigned>((chip >> 16) & 0xFF),
           static_cast<unsigned>((chip >> 8) & 0xFF), static_cast<unsigned>(chip & 0xFF));
  return String(buf);
}

// Helper: zbuduj JSON event z prostych kluczy. Bez ArduinoJson —
// utrzymujemy zero dependencies poza NimBLE.
String jsonEvent(const String &ev, const String &extraKeyVal = "") {
  String out = "{\"ev\":\"" + ev + "\"";
  if (extraKeyVal.length() > 0) {
    out += "," + extraKeyVal;
  }
  out += "}";
  return out;
}

}  // namespace

struct BleApi::Impl : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
  App *app = nullptr;
  NimBLEServer *server = nullptr;
  NimBLEService *service = nullptr;
  NimBLECharacteristic *cmdChar = nullptr;
  NimBLECharacteristic *evtChar = nullptr;
  String name;
  bool active = false;
  bool clientConnected = false;
  String rxBuffer;  // bufor dla JSON Lines (komendy mogą przyjść w kawałkach)

  // NimBLEServerCallbacks ─────────────────────────────────────────────────
  void onConnect(NimBLEServer *s) override {
    clientConnected = true;
    Serial.println("[ble] client connected");
    // Wyślij hello tak jak GET /api/hello po WiFi.
    sendEvent(jsonEvent("hello", "\"name\":\"Flower\",\"api\":1"));
  }

  void onDisconnect(NimBLEServer *s) override {
    clientConnected = false;
    rxBuffer = "";
    Serial.println("[ble] client disconnected, restart advertising");
    // Po rozłączeniu reklamuj się znowu, żeby kolejny klient mógł połączyć.
    NimBLEDevice::startAdvertising();
  }

  // NimBLECharacteristicCallbacks ─────────────────────────────────────────
  void onWrite(NimBLECharacteristic *ch) override {
    const std::string &v = ch->getValue();
    rxBuffer += String(v.c_str(), v.length());
    Serial.printf("[ble] cmd bytes=%u total=%u\n", static_cast<unsigned>(v.length()),
                  static_cast<unsigned>(rxBuffer.length()));

    // Komendy są rozdzielane "\n" — możemy dostać wiele w jednym write
    // lub jedną pociętą na kilka writes (BLE chunkuje powyżej MTU).
    int nl;
    while ((nl = rxBuffer.indexOf('\n')) >= 0) {
      String line = rxBuffer.substring(0, nl);
      rxBuffer = rxBuffer.substring(nl + 1);
      line.trim();
      if (line.length() > 0) {
        handleCommand(line);
      }
    }
  }

  // ──────────────────────────────────────────────────────────────────────

  void sendEvent(const String &json) {
    if (!evtChar || !clientConnected) return;
    String line = json + "\n";
    evtChar->setValue(reinterpret_cast<const uint8_t *>(line.c_str()), line.length());
    evtChar->notify();
  }

  void handleCommand(const String &line) {
    Serial.printf("[ble] cmd: %s\n", line.c_str());
    // Mini-parser, na razie obsługujemy proste przypadki bez ArduinoJson.
    // Dispatch będzie się rozrastał — patrz docs/roadmap.md.

    if (line.indexOf("\"ping\"") >= 0) {
      sendEvent(jsonEvent("pong", "\"ts\":" + String(millis())));
      return;
    }
    if (line.indexOf("\"hello\"") >= 0) {
      sendEvent(jsonEvent("hello", "\"name\":\"Flower\",\"api\":1"));
      return;
    }
    if (line.indexOf("\"get-version\"") >= 0) {
      const String v = app ? app->firmwareVersionLabel() : String("dev");
      sendEvent(jsonEvent("version", "\"value\":\"" + v + "\""));
      return;
    }
    if (line.indexOf("\"set-dev-mode\"") >= 0) {
      // body: {"cmd":"set-dev-mode","enabled":true}
      const bool enabled = line.indexOf("\"enabled\":true") >= 0;
      if (app) app->setDevModeEnabled(enabled);
      sendEvent(jsonEvent("dev-mode", String("\"enabled\":") + (enabled ? "true" : "false")));
      return;
    }

    // Nieznana komenda — odpowiedz oszczędnie.
    sendEvent(jsonEvent("error", "\"reason\":\"unknown-cmd\""));
  }
};

BleApi::BleApi() : impl_(nullptr) {}

BleApi::~BleApi() { stop(); }

void BleApi::begin(App *app) {
  if (impl_ != nullptr) return;  // idempotent

  impl_ = new Impl();
  impl_->app = app;
  impl_->name = "Flower-" + deviceSuffix();

  NimBLEDevice::init(impl_->name.c_str());
  NimBLEDevice::setPower(ESP_PWR_LVL_P7);  // ~+7 dBm — dobry zasięg w mieszkaniu
  NimBLEDevice::setMTU(kPreferredMtu);

  impl_->server = NimBLEDevice::createServer();
  impl_->server->setCallbacks(impl_);

  impl_->service = impl_->server->createService(kServiceUuid);

  impl_->cmdChar = impl_->service->createCharacteristic(
      kCmdCharUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  impl_->cmdChar->setCallbacks(impl_);

  impl_->evtChar = impl_->service->createCharacteristic(
      kEvtCharUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  impl_->service->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(kServiceUuid);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMinPreferred(0x12);
  NimBLEDevice::startAdvertising();

  impl_->active = true;
  Serial.printf("[ble] advertising as %s\n", impl_->name.c_str());
}

void BleApi::stop() {
  if (impl_ == nullptr) return;
  NimBLEDevice::stopAdvertising();
  if (impl_->server != nullptr) {
    // Rozłącz wszystkich klientów żeby nie zostali z otwartym GATT po
    // wyłączeniu — niektóre OS-y wtedy pokazują "podłączone do urządzenia".
    auto peers = impl_->server->getPeerDevices();
    for (auto handle : peers) impl_->server->disconnect(handle);
  }
  NimBLEDevice::deinit(true);
  delete impl_;
  impl_ = nullptr;
  Serial.println("[ble] stopped");
}

void BleApi::update() {
  // Reserved — NimBLE pracuje na własnych taskach, nie musimy nic robić
  // z głównej pętli. Funkcja istnieje żeby callsite było odporne na
  // ewentualne dodanie heartbeatu albo timeoutu w przyszłości.
}

bool BleApi::isActive() const { return impl_ != nullptr && impl_->active; }
bool BleApi::isConnected() const { return impl_ != nullptr && impl_->clientConnected; }
String BleApi::deviceName() const { return impl_ != nullptr ? impl_->name : String(""); }

void BleApi::emitEvent(const String &json) {
  if (impl_) impl_->sendEvent(json);
}

#else  // FLOWER_BLE_ENABLED == 0 — stub do compile-out

BleApi::BleApi() {}
BleApi::~BleApi() {}
void BleApi::begin(App *) {}
void BleApi::stop() {}
void BleApi::update() {}
bool BleApi::isActive() const { return false; }
bool BleApi::isConnected() const { return false; }
String BleApi::deviceName() const { return String(""); }
void BleApi::emitEvent(const String &) {}

#endif  // FLOWER_BLE_ENABLED
