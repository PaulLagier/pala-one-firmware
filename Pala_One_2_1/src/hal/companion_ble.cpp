#include "src/hal/companion_ble.h"
#ifdef PALA_COMPANION
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <esp_mac.h>
#include <esp_random.h>
#include "src/pure/companion_queue.h"
#include "src/state.h"
#include "src/storage/fs_util.h"
#include "src/storage/library.h"
#include "src/ui/header_title.h"
#include "src/hal/companion_rpc.h"

namespace {
constexpr const char *kService = "a28b1001-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *kIdentity = "a28b1002-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *kControl = "a28b1003-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *kResponse = "a28b1004-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *kData = "a28b1005-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *kSignal = "a28b1006-4d48-4e42-9b82-6b6c50414c41";
String pin;
bool running = false, redraw = false, authenticated = false;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
Companion::Queue queue;
uint8_t identityBytes[Companion::PacketBytes];
size_t identityLength = 0;
NimBLECharacteristic *responseCharacteristic = nullptr;
NimBLECharacteristic *signalCharacteristic = nullptr;

Companion::Session currentSession() {
  portENTER_CRITICAL(&mux);
  const auto result = queue.session();
  portEXIT_CRITICAL(&mux);
  return result;
}
bool current(const Companion::Session &session) {
  return Companion::sameSession(session, currentSession());
}
bool secureOwner(NimBLEConnInfo &peer, const Companion::Session &session) {
  return session.connected() && peer.getConnHandle() == session.owner && peer.isEncrypted() &&
         peer.isAuthenticated();
}
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *server, NimBLEConnInfo &peer) override {
    uint32_t token = esp_random();
    if (!token)
      token = 1;
    portENTER_CRITICAL(&mux);
    const bool accepted = queue.connect(peer.getConnHandle(), token);
    if (accepted) {
      redraw = true;
      authenticated = false;
    }
    portEXIT_CRITICAL(&mux);
    if (!accepted) {
      server->disconnect(peer.getConnHandle());
      return;
    }
    responseCharacteristic->setValue("{\"id\":0,\"ok\":true}");
    server->updateConnParams(peer.getConnHandle(), 12, 24, 0, 400);
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &peer, int) override {
    portENTER_CRITICAL(&mux);
    if (queue.disconnect(peer.getConnHandle())) {
      redraw = true;
      authenticated = false;
    }
    portEXIT_CRITICAL(&mux);
  }
  void onAuthenticationComplete(NimBLEConnInfo &peer) override {
    const auto expected = currentSession();
    if (!secureOwner(peer, expected)) {
      // Revoke queued work immediately; controller disconnect is asynchronous.
      portENTER_CRITICAL(&mux);
      if (queue.disconnect(peer.getConnHandle())) {
        redraw = true;
        authenticated = false;
      }
      portEXIT_CRITICAL(&mux);
      NimBLEDevice::getServer()->disconnect(peer.getConnHandle());
      return;
    }
    portENTER_CRITICAL(&mux);
    if (Companion::sameSession(expected, queue.session())) {
      redraw = true;
      authenticated = true;
    }
    portEXIT_CRITICAL(&mux);
  }
};
ServerCallbacks serverCallbacks;

class IdentityCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic *characteristic, NimBLEConnInfo &peer) override {
    uint8_t bytes[Companion::PacketBytes];
    size_t length;
    Companion::Session session;
    portENTER_CRITICAL(&mux);
    length = identityLength;
    std::memcpy(bytes, identityBytes, length);
    session = queue.session();
    portEXIT_CRITICAL(&mux);
    if (!secureOwner(peer, session)) {
      characteristic->setValue("");
      return;
    }
    JsonDocument doc;
    if (deserializeJson(doc, bytes, length)) {
      characteristic->setValue("");
      return;
    }
    doc["session"] = session.token;
    String value;
    serializeJson(doc, value);
    characteristic->setValue(
        current(session) && value.length() <= Companion::PacketBytes ? value.c_str() : "");
  }
};
IdentityCallbacks identityCallbacks;

class ResponseCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic *characteristic, NimBLEConnInfo &peer) override {
    const auto session = currentSession();
    if (!secureOwner(peer, session)) {
      characteristic->setValue("");
      return;
    }
    // A disconnect can race publication. Never expose a previous session's
    // response to the newly paired owner, even if a controller handle is reused.
    auto value = characteristic->getValue();
    JsonDocument doc;
    if (deserializeJson(doc, value.data(), value.size()) ||
        ((doc["id"] | 0U) && (doc["session"] | 0U) != session.token))
      characteristic->setValue("");
  }
};
ResponseCallbacks responseCallbacks;

class QueueCallbacks : public NimBLECharacteristicCallbacks {
public:
  explicit QueueCallbacks(bool data) : data_(data) {}
  void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &peer) override {
    const auto expected = currentSession();
    if (!secureOwner(peer, expected))
      return;
    const auto value = characteristic->getValue();
    // Capture before reading/copying and recheck under the same lock as
    // connect/disconnect, rather than assigning a new session to stale input.
    portENTER_CRITICAL(&mux);
    queue.push(expected, true, value.data(), value.size(), data_);
    portEXIT_CRITICAL(&mux);
  }

private:
  bool data_;
};
QueueCallbacks controlCallbacks(false), dataCallbacks(true);

void publish(JsonDocument &reply, const Companion::Session &session) {
  reply["session"] = session.token;
  String value;
  serializeJson(reply, value);
  if (value.length() > Companion::PacketBytes) {
    const uint32_t id = reply["id"] | 0U;
    reply.clear();
    reply["id"] = id;
    reply["ok"] = false;
    reply["error"] = "response-too-large";
    reply["session"] = session.token;
    value = "";
    serializeJson(reply, value);
  }
  if (!current(session))
    return;
  responseCharacteristic->setValue(value.c_str());
  const uint32_t id = reply["id"] | 0U;
  const uint8_t signal[4] = {uint8_t(id), uint8_t(id >> 8), uint8_t(id >> 16), uint8_t(id >> 24)};
  if (current(session))
    signalCharacteristic->notify(signal, sizeof(signal));
}
} // namespace

void companionBleRefreshIdentity() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char id[18];
  snprintf(id, sizeof(id), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
           mac[5]);
  JsonDocument doc;
  doc["protocol"] = "pala-companion";
  doc["version"] = 3;
  doc["transport"] = "ble";
  doc["model"] = "pala-one";
  doc["id"] = id;
  doc["name"] = HeaderTitle::current();
  doc["firmware"] = FW_VERSION;
  doc["freeBytes"] = fsFreeBytesSafe();
  auto capabilities = doc["capabilities"].to<JsonArray>();
  capabilities.add("bookmarks-v1");
  capabilities.add("bulk-window-v1");
  capabilities.add("session-v1");
  capabilities.add("pbt1");
  String value;
  serializeJson(doc, value);
  // Reserve room for the session field appended by the authenticated read.
  if (value.length() + 22 > sizeof(identityBytes)) {
    // A legacy web-configured title may contain heavily escaped controls.
    // Keep identity readable rather than retaining a previous session name.
    doc["name"] = "Pala One";
    value = "";
    serializeJson(doc, value);
  }
  if (value.length() + 22 > sizeof(identityBytes))
    return;
  portENTER_CRITICAL(&mux);
  identityLength = value.length();
  std::memcpy(identityBytes, value.c_str(), identityLength);
  redraw = true;
  portEXIT_CRITICAL(&mux);
}
void companionBleBegin() {
  if (running)
    return;
  companionRpcReset();
  loadBooks();
  companionBleRefreshIdentity();
  const uint32_t number = 100000 + esp_random() % 900000;
  pin = String(number);
  if (!NimBLEDevice::init("Pala One"))
    return;
  NimBLEDevice::setMTU(247);
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEDevice::setSecurityPasskey(number);
  auto *server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks, false);
  server->advertiseOnDisconnect(true);
  auto *service = server->createService(kService);
  constexpr auto read =
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN;
  constexpr auto write =
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN;
  auto *identity = service->createCharacteristic(kIdentity, read, Companion::PacketBytes);
  identity->setCallbacks(&identityCallbacks);
  auto *control = service->createCharacteristic(kControl, write, Companion::PacketBytes);
  control->setCallbacks(&controlCallbacks);
  auto *data = service->createCharacteristic(kData, write | NIMBLE_PROPERTY::WRITE_NR,
                                             Companion::PacketBytes);
  data->setCallbacks(&dataCallbacks);
  responseCharacteristic = service->createCharacteristic(kResponse, read, Companion::PacketBytes);
  responseCharacteristic->setCallbacks(&responseCallbacks);
  responseCharacteristic->setValue("{\"id\":0,\"ok\":true}");
  signalCharacteristic = service->createCharacteristic(
      kSignal, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN,
      4);
  auto *advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(kService);
  advertising->enableScanResponse(true);
  advertising->setName("Pala One");
  running = advertising->start();
  if (!running) {
    NimBLEDevice::deinit(true);
    responseCharacteristic = signalCharacteristic = nullptr;
  }
}
void companionBleTick() {
  if (!running)
    return;
  String latest;
  Companion::Session latestSession;
  for (size_t i = 0; i < 4; ++i) {
    portENTER_CRITICAL(&mux);
    const auto work = queue.take();
    portEXIT_CRITICAL(&mux);
    if (work.reset) {
      companionRpcReset();
      latest = "";
    }
    if (work.overflow) {
      JsonDocument reply;
      reply["id"] = 0;
      reply["ok"] = false;
      reply["error"] = "queue-full";
      if (current(work.session))
        publish(reply, work.session);
      return;
    }
    if (!work.hasPacket || !current(work.packet.session))
      break;
    // The operation is accepted at this session check. If disconnect happens
    // during it, its reply is suppressed and the next session resets staging.
    const auto &packet = work.packet;
    String result = packet.data ? companionRpcChunk(packet.bytes, packet.length)
                                : companionRpcControl(packet.bytes, packet.length);
    if (!current(packet.session))
      continue;
    JsonDocument reply;
    if (deserializeJson(reply, result))
      continue;
    if (!(reply["ok"] | false)) {
      portENTER_CRITICAL(&mux);
      queue.discardPending();
      portEXIT_CRITICAL(&mux);
      companionRpcReset();
      publish(reply, packet.session);
      return;
    }
    latest = result;
    latestSession = packet.session;
  }
  if (latest.length()) {
    JsonDocument reply;
    if (!deserializeJson(reply, latest))
      publish(reply, latestSession);
  }
}
void companionBleEnd() {
  if (running)
    NimBLEDevice::deinit(true);
  running = false;
  responseCharacteristic = signalCharacteristic = nullptr;
  portENTER_CRITICAL(&mux);
  queue.end();
  identityLength = 0;
  redraw = false;
  authenticated = false;
  portEXIT_CRITICAL(&mux);
  pin = "";
  companionRpcReset();
}
bool companionBleConnected() {
  portENTER_CRITICAL(&mux);
  const bool result = queue.session().connected() && authenticated;
  portEXIT_CRITICAL(&mux);
  return result;
}
bool companionBleRunning() { return running; }
bool companionBleNeedsRedraw() {
  portENTER_CRITICAL(&mux);
  const bool value = redraw;
  redraw = false;
  portEXIT_CRITICAL(&mux);
  return value;
}
String companionPairingPin() { return pin; }
void companionResetPairing() {
  if (running)
    NimBLEDevice::deleteAllBonds();
}
#else
void companionBleBegin() {}
void companionBleEnd() {}
void companionBleTick() {}
void companionBleRefreshIdentity() {}
bool companionBleConnected() { return false; }
bool companionBleRunning() { return false; }
bool companionBleNeedsRedraw() { return false; }
String companionPairingPin() { return ""; }
void companionResetPairing() {}
#endif
