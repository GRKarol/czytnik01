#include "ble/BleApi.h"

#if FLOWER_BLE_ENABLED

#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>  // portMUX_TYPE / portENTER_CRITICAL — używamy do
                                // synchronizacji deferred command queue między
                                // BLE host task (writer) a main task (drainer).

#include <atomic>

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

// Limit pendingu deferowanych komend między BLE host taskiem a main
// taskiem. 8 wystarczy z naddatkiem — komendy z telefonu rzadko lecą
// szybciej niż user-driven.
constexpr size_t kPendingCommandsMax = 8;

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

// Mini-parser JSON: tolerancyjny na whitespace, sprawdza pełną nazwę
// klucza w cudzysłowach. Zamienia substring `indexOf("\"x\":true")` na
// odpornego helpera (telefony pretty-printują payload niekontrolowanie).
//
// Znajdź `"key"` jako pole, pomiń ":" i białe znaki, zwróć indeks pierwszego
// znaku wartości. -1 jeśli nie ma.
int jsonFieldStart(const String &body, const char *key) {
  String pattern = String("\"") + key + "\"";
  int p = body.indexOf(pattern);
  if (p < 0) return -1;
  p += pattern.length();
  while (p < static_cast<int>(body.length()) &&
         (body[p] == ' ' || body[p] == '\t' || body[p] == ':')) {
    p++;
  }
  return p;
}

bool jsonReadBool(const String &body, const char *key, bool &out) {
  int p = jsonFieldStart(body, key);
  if (p < 0) return false;
  if (body.length() - p >= 4 && body.substring(p, p + 4) == "true") {
    out = true;
    return true;
  }
  if (body.length() - p >= 5 && body.substring(p, p + 5) == "false") {
    out = false;
    return true;
  }
  return false;
}

bool jsonStringEquals(const String &body, const char *key, const char *expected) {
  int p = jsonFieldStart(body, key);
  if (p < 0 || p >= static_cast<int>(body.length()) || body[p] != '"') return false;
  int end = body.indexOf('"', p + 1);
  if (end < 0) return false;
  return body.substring(p + 1, end) == expected;
}

}  // namespace

struct BleApi::Impl : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
  App *app = nullptr;
  NimBLEServer *server = nullptr;
  NimBLEService *service = nullptr;
  NimBLECharacteristic *cmdChar = nullptr;
  NimBLECharacteristic *evtChar = nullptr;
  String name;

  // Flagi czytane z main taska, pisane z host taska — trzeba atomic
  // żeby uniknąć torn-reads i mieć ordering guarantees (release on write,
  // acquire on read). Bool nie wymaga aż mutex'a, atomic wystarczy.
  std::atomic<bool> active{false};
  std::atomic<bool> clientConnected{false};
  // Set raz w stop(); callback'i sprawdzają to przed jakimkolwiek
  // dostępem do innych pól. NimBLE 1.4.x deinit() jest synchroniczny —
  // ten flag chroni okno między first-disconnect a deinit oraz przed
  // potencjalnymi opóźnionymi callbackami.
  std::atomic<bool> shuttingDown{false};
  // Main task obserwuje ten flag żeby wiedzieć kiedy odświeżyć ekran
  // SettingsConnectivity (label "POLACZONY" / "WLACZONY").
  std::atomic<bool> menuDirty{false};
  // Notify gotowy dopiero gdy klient zasubskrybuje CCCD — przed tym
  // sendEvent() jest no-op. Set z onSubscribe(), reset w onDisconnect.
  std::atomic<bool> notifyReady{false};

  // Deferred command queue. BLE task wkłada surowy JSON Line tutaj,
  // main task (App::update → ble_.update) odczytuje i wykonuje na
  // App. To zapobiega race-om na App::settingsMenuItems_, NVS itp.
  // — wszystkie mutacje App lecą z main taska, BLE task tylko parsuje.
  std::vector<String> pendingCommands;
  // pendingCommands jest pisany przez BLE task i konsumowany przez main
  // task. atomic bool flag „masz coś do roboty" + krótka sekcja krytyczna
  // na portMUX zabezpiecza modyfikacje wektora.
  portMUX_TYPE pendingMux = portMUX_INITIALIZER_UNLOCKED;
  std::atomic<bool> pendingNonEmpty{false};

  String rxBuffer;  // bufor JSON Lines, używany TYLKO przez BLE task

  // NimBLEServerCallbacks ─────────────────────────────────────────────────
  void onConnect(NimBLEServer *) override {
    if (shuttingDown.load(std::memory_order_acquire)) return;
    clientConnected.store(true, std::memory_order_release);
    notifyReady.store(false, std::memory_order_release);  // poczekaj na onSubscribe
    menuDirty.store(true, std::memory_order_release);
    Serial.println("[ble] client connected");
    // Nie wysyłamy hello z onConnect — klient jeszcze nie podpiął CCCD
    // (subscribe = enable notifications). Wysyłamy z onSubscribe poniżej.
  }

  void onDisconnect(NimBLEServer *) override {
    if (shuttingDown.load(std::memory_order_acquire)) return;
    clientConnected.store(false, std::memory_order_release);
    notifyReady.store(false, std::memory_order_release);
    menuDirty.store(true, std::memory_order_release);
    rxBuffer = "";
    Serial.println("[ble] client disconnected, restart advertising");
    NimBLEDevice::startAdvertising();
  }

  // NimBLECharacteristicCallbacks ─────────────────────────────────────────
  void onWrite(NimBLECharacteristic *ch) override {
    if (shuttingDown.load(std::memory_order_acquire)) return;
    const std::string &v = ch->getValue();
    rxBuffer += String(v.c_str(), v.length());
    Serial.printf("[ble] cmd bytes=%u total=%u\n", static_cast<unsigned>(v.length()),
                  static_cast<unsigned>(rxBuffer.length()));

    // Komendy są rozdzielane "\n" — wiele w jednym write albo jedna pocięta.
    int nl;
    while ((nl = rxBuffer.indexOf('\n')) >= 0) {
      String line = rxBuffer.substring(0, nl);
      rxBuffer = rxBuffer.substring(nl + 1);
      line.trim();
      if (line.length() > 0) {
        enqueueCommand(line);
      }
    }
  }

  // Trigger gdy klient wpisze 0x0001 do CCCD (zacznie subscription dla
  // notify). Po tym sendEvent() ma sens. Wcześniej notify był silently
  // dropowany przez NimBLE.
  void onSubscribe(NimBLECharacteristic *ch, ble_gap_conn_desc *, uint16_t subValue) override {
    if (shuttingDown.load(std::memory_order_acquire)) return;
    if (ch != evtChar) return;
    const bool wantsNotify = (subValue & 0x0001) != 0;
    notifyReady.store(wantsNotify, std::memory_order_release);
    if (wantsNotify) {
      Serial.println("[ble] client subscribed to events, sending hello");
      sendEventFromHostTask(jsonEvent("hello", "\"name\":\"Flower\",\"api\":1"));
    }
  }

  // ──────────────────────────────────────────────────────────────────────

  void enqueueCommand(const String &line) {
    portENTER_CRITICAL(&pendingMux);
    if (pendingCommands.size() < kPendingCommandsMax) {
      pendingCommands.push_back(line);
      pendingNonEmpty.store(true, std::memory_order_release);
    }
    portEXIT_CRITICAL(&pendingMux);
  }

  // sendEvent z BLE host taska — można wołać bezpośrednio (NimBLE jest
  // thread-safe dla notify). Sprawdza notifyReady żeby nie marnować
  // bajtów gdy CCCD nie zasubskrybowany.
  void sendEventFromHostTask(const String &json) {
    if (!evtChar) return;
    if (!clientConnected.load(std::memory_order_acquire)) return;
    if (!notifyReady.load(std::memory_order_acquire)) return;
    String line = json + "\n";
    evtChar->setValue(reinterpret_cast<const uint8_t *>(line.c_str()), line.length());
    evtChar->notify();
  }

  // Wywoływane z main taska (App::update → ble_.update). Przetwarza
  // wszystkie zakolejkowane komendy w kontekście main taska, dzięki
  // czemu setDevModeEnabled() i rebuildSettingsMenuItems() są bezpieczne.
  void drainPendingCommands() {
    if (!pendingNonEmpty.load(std::memory_order_acquire)) return;

    std::vector<String> local;
    portENTER_CRITICAL(&pendingMux);
    local.swap(pendingCommands);
    pendingNonEmpty.store(false, std::memory_order_release);
    portEXIT_CRITICAL(&pendingMux);

    for (const String &line : local) {
      handleCommand(line);
    }
  }

  // Wykonywane na main tasku — bezpieczne wołać App::*.
  void handleCommand(const String &line) {
    Serial.printf("[ble] cmd: %s\n", line.c_str());

    if (jsonStringEquals(line, "cmd", "ping")) {
      sendEventFromHostTask(jsonEvent("pong", "\"ts\":" + String(millis())));
      return;
    }
    if (jsonStringEquals(line, "cmd", "hello")) {
      sendEventFromHostTask(jsonEvent("hello", "\"name\":\"Flower\",\"api\":1"));
      return;
    }
    if (jsonStringEquals(line, "cmd", "get-version")) {
      const String v = app ? app->firmwareVersionLabel() : String("dev");
      sendEventFromHostTask(jsonEvent("version", "\"value\":\"" + v + "\""));
      return;
    }
    if (jsonStringEquals(line, "cmd", "set-dev-mode")) {
      bool enabled = false;
      if (!jsonReadBool(line, "enabled", enabled)) {
        sendEventFromHostTask(jsonEvent("error", "\"reason\":\"missing-enabled\""));
        return;
      }
      if (app) app->setDevModeEnabled(enabled);
      sendEventFromHostTask(jsonEvent("dev-mode", String("\"enabled\":") +
                                                      (enabled ? "true" : "false")));
      return;
    }

    sendEventFromHostTask(jsonEvent("error", "\"reason\":\"unknown-cmd\""));
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
  // Power: -9 dBm dla advertising/scan/connection. W mieszkaniu zasięg >5 m,
  // a oszczędność prądu vs default (~+3 dBm) jest zauważalna w stanie idle
  // (urządzenie reklamuje się ciągle gdy BLE on). NimBLE 1.4 ma 3-arg
  // overload — ustawiamy każdy power type osobno, żeby nie zostawić scan/
  // connection na controller-default.
  NimBLEDevice::setPower(ESP_PWR_LVL_N9, ESP_BLE_PWR_TYPE_ADV);
  NimBLEDevice::setPower(ESP_PWR_LVL_N9, ESP_BLE_PWR_TYPE_SCAN);
  NimBLEDevice::setPower(ESP_PWR_LVL_N9, ESP_BLE_PWR_TYPE_DEFAULT);
  NimBLEDevice::setMTU(kPreferredMtu);

  impl_->server = NimBLEDevice::createServer();
  impl_->server->setCallbacks(impl_);

  impl_->service = impl_->server->createService(kServiceUuid);

  impl_->cmdChar = impl_->service->createCharacteristic(
      kCmdCharUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  impl_->cmdChar->setCallbacks(impl_);

  impl_->evtChar = impl_->service->createCharacteristic(
      kEvtCharUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  impl_->evtChar->setCallbacks(impl_);  // żeby onSubscribe trafił do impl_

  impl_->service->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(kServiceUuid);
  adv->setScanResponse(true);
  // Min/max preferred connection interval: 7.5 ms – 22.5 ms.
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  // Advertising interval: 320 ms (slot 0x200 × 0.625 ms). Default ~100 ms
  // pożera kilka mA stale. 320 ms daje +~50 % oszczędności na advertising
  // przy ~0.5 s opóźnieniu discovery — niezauważalne dla klienta, znaczące
  // dla baterii czytnika który leży godzinami w bezruchu.
  adv->setMinInterval(0x200);
  adv->setMaxInterval(0x240);
  NimBLEDevice::startAdvertising();

  impl_->active.store(true, std::memory_order_release);
  Serial.printf("[ble] advertising as %s\n", impl_->name.c_str());
}

void BleApi::stop() {
  if (impl_ == nullptr) return;

  // Krok 1: ustaw shuttingDown żeby kolejne callbacks z BLE taska zrobiły
  // wczesny return, zanim dotkną pól ktore zaraz znikną.
  impl_->shuttingDown.store(true, std::memory_order_release);

  // Krok 2: wyłącz callback'i na poziomie NimBLE — od tej pory host
  // task ma null pointer (no-op) zamiast naszego impl, więc nawet jeśli
  // jakieś zdarzenie jest in-flight, nie trafi do nas.
  if (impl_->server != nullptr) impl_->server->setCallbacks(nullptr);
  if (impl_->cmdChar != nullptr) impl_->cmdChar->setCallbacks(nullptr);
  if (impl_->evtChar != nullptr) impl_->evtChar->setCallbacks(nullptr);

  NimBLEDevice::stopAdvertising();
  if (impl_->server != nullptr) {
    auto peers = impl_->server->getPeerDevices();
    for (auto handle : peers) impl_->server->disconnect(handle);
  }

  // Krok 3: deinit. NimBLE 1.4.x: deinit(true) jest synchroniczne — host
  // task się zakończy zanim wrócimy z tego wywołania.
  NimBLEDevice::deinit(true);

  delete impl_;
  impl_ = nullptr;
  Serial.println("[ble] stopped");
}

void BleApi::update() {
  if (impl_ == nullptr) return;
  // Przetwarzaj komendy zakolejkowane przez BLE host task. Wszystkie
  // App::* mutacje (settingsMenuItems_, NVS via preferences_) leca tutaj,
  // więc nie ma race'a z głównym update() / render path.
  impl_->drainPendingCommands();
}

bool BleApi::isActive() const {
  return impl_ != nullptr && impl_->active.load(std::memory_order_acquire);
}
bool BleApi::isConnected() const {
  return impl_ != nullptr && impl_->clientConnected.load(std::memory_order_acquire);
}
String BleApi::deviceName() const { return impl_ != nullptr ? impl_->name : String(""); }

void BleApi::emitEvent(const String &json) {
  if (impl_) impl_->sendEventFromHostTask(json);
}

bool BleApi::consumeMenuDirty() {
  if (impl_ == nullptr) return false;
  // exchange = atomic test-and-clear. Jeśli było true, zwróć true raz
  // i zostaw false aż do kolejnej zmiany stanu.
  return impl_->menuDirty.exchange(false, std::memory_order_acq_rel);
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
bool BleApi::consumeMenuDirty() { return false; }

#endif  // FLOWER_BLE_ENABLED
