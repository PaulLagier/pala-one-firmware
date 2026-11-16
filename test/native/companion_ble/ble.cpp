#include <Arduino.h>
#include "test_framework.h"
#include <ArduinoJson.h>
#include <NimBLEDevice.h>
#include "src/state.h"
#include "src/storage/library.h"
#include "src/hal/companion_ble.h"
#include "src/hal/companion_rpc.h"

MemoryFS FS;
Catalog g_library;
unsigned resetCalls = 0, controlCalls = 0, dataCalls = 0, loadCalls = 0;
bool failNext = false, oversizeReply = false;
std::function<void()> duringRpc;
void loadBooks() { ++loadCalls; }
void companionRpcReset() { ++resetCalls; }
String reply(uint32_t id) {
  if (duringRpc) {
    auto hook = std::move(duringRpc);
    duringRpc = nullptr;
    hook();
  }
  JsonDocument doc;
  doc["id"] = id;
  doc["ok"] = !failNext;
  if (failNext)
    doc["error"] = "test-error";
  if (oversizeReply)
    doc["text"] = std::string(1024, 'x');
  failNext = false;
  String text;
  serializeJson(doc, text);
  return text;
}
String companionRpcControl(const uint8_t *bytes, size_t length) {
  ++controlCalls;
  JsonDocument input;
  deserializeJson(input, bytes, length);
  return reply(input["id"] | 0U);
}
String companionRpcChunk(const uint8_t *bytes, size_t) {
  ++dataCalls;
  return reply(uint32_t(bytes[0]) | uint32_t(bytes[1]) << 8 | uint32_t(bytes[2]) << 16 |
               uint32_t(bytes[3]) << 24);
}
namespace {
constexpr const char *identityId = "a28b1002-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *controlId = "a28b1003-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *responseId = "a28b1004-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *dataId = "a28b1005-4d48-4e42-9b82-6b6c50414c41";
constexpr const char *signalId = "a28b1006-4d48-4e42-9b82-6b6c50414c41";
NimBLECharacteristic &characteristic(const char *id) {
  return *NimBLEDevice::getServer()->service.characteristics.at(id);
}
void reset() {
  companionBleEnd();
  resetCalls = controlCalls = dataCalls = loadCalls = 0;
  TestRadio::initSuccess = TestRadio::advertiseSuccess = true;
  TestRadio::readHook = nullptr;
  duringRpc = nullptr;
  failNext = oversizeReply = false;
  FS = MemoryFS{};
  companionBleBegin();
}
void writeControl(NimBLEConnInfo &peer, uint32_t id = 1) {
  auto text = "{\"id\":" + std::to_string(id) + ",\"op\":\"ping\"}";
  characteristic(controlId).write(peer, {text.begin(), text.end()});
}
JsonDocument response(NimBLEConnInfo &peer) {
  auto text = characteristic(responseId).read(peer);
  CHECK(text.size() <= 512);
  JsonDocument doc;
  CHECK(!deserializeJson(doc, text));
  return doc;
}
JsonDocument identity(NimBLEConnInfo &peer) {
  auto text = characteristic(identityId).read(peer);
  CHECK(text.size() <= 512);
  JsonDocument doc;
  CHECK(!deserializeJson(doc, text));
  return doc;
}
} // namespace
TEST_CASE("BLE backend retains its UUID protocol and authenticated characteristic properties") {
  reset();
  REQUIRE(companionBleRunning());
  CHECK_EQ(loadCalls, 1u);
  CHECK_EQ(resetCalls, 1u);
  CHECK(NimBLEDevice::bond && NimBLEDevice::mitm && NimBLEDevice::sc);
  CHECK_EQ(NimBLEDevice::mtu, 247);
  CHECK_EQ(companionPairingPin().length(), 6u);
  CHECK(NimBLEDevice::passkey >= 100000 && NimBLEDevice::passkey <= 999999);
  for (const char *id : {identityId, responseId}) {
    auto &c = characteristic(id);
    CHECK(c.properties & NIMBLE_PROPERTY::READ_ENC);
    CHECK(c.properties & NIMBLE_PROPERTY::READ_AUTHEN);
    CHECK_EQ(c.maximum, 512u);
  }
  for (const char *id : {controlId, dataId}) {
    auto &c = characteristic(id);
    CHECK(c.properties & NIMBLE_PROPERTY::WRITE_ENC);
    CHECK(c.properties & NIMBLE_PROPERTY::WRITE_AUTHEN);
  }
  CHECK(characteristic(signalId).properties & NIMBLE_PROPERTY::READ_ENC);
  NimBLEConnInfo owner(1);
  NimBLEDevice::getServer()->connect(owner);
  CHECK(!companionBleConnected());
  NimBLEDevice::getServer()->authenticate(owner);
  CHECK(companionBleConnected());
  const auto doc = identity(owner);
  CHECK(doc["protocol"] == "pala-companion");
  CHECK_EQ(doc["version"].as<int>(), 3);
  CHECK(doc["session"].as<uint32_t>() != 0);
  CHECK_EQ(resetCalls, 1u);
  companionBleTick();
  CHECK_EQ(resetCalls, 2u);
}
TEST_CASE("BLE callbacks reject unauthorized writes and execute RPC only on main-loop tick") {
  reset();
  NimBLEConnInfo owner(1), unencrypted(1, false, false), unauthenticated(1, true, false),
      stranger(2);
  NimBLEDevice::getServer()->connect(owner);
  writeControl(unencrypted);
  writeControl(unauthenticated);
  writeControl(stranger);
  companionBleTick();
  CHECK_EQ(controlCalls, 0u);
  CHECK(characteristic(identityId).read(unencrypted).empty());
  writeControl(owner, 37);
  CHECK_EQ(controlCalls, 0u);
  companionBleTick();
  CHECK_EQ(controlCalls, 1u);
  auto doc = response(owner);
  CHECK_EQ(doc["id"].as<int>(), 37);
  CHECK(doc["ok"].as<bool>());
  auto &signal = characteristic(signalId);
  CHECK_EQ(signal.signal.size(), 4u);
  CHECK_EQ(signal.signal[0], 37u);
  NimBLEDevice::getServer()->connect(stranger);
  CHECK_EQ(NimBLEDevice::getServer()->rejected.back(), 2u);
  NimBLEDevice::getServer()->disconnected(stranger);
  writeControl(owner, 38);
  companionBleTick();
  CHECK_EQ(controlCalls, 2u);
}
TEST_CASE(
    "BLE backend discards disconnect queues and stale callbacks with reused connection handles") {
  reset();
  NimBLEConnInfo owner(7);
  auto *server = NimBLEDevice::getServer();
  server->connect(owner);
  const auto oldToken = identity(owner)["session"].as<uint32_t>();
  writeControl(owner);
  server->disconnected(owner);
  server->connect(owner);
  companionBleTick();
  CHECK_EQ(controlCalls, 0u);
  CHECK(identity(owner)["session"].as<uint32_t>() != oldToken);
  TestRadio::readHook = [&] {
    server->disconnected(owner);
    server->connect(owner);
  };
  writeControl(owner);
  companionBleTick();
  CHECK_EQ(controlCalls, 0u);
  writeControl(owner, 9);
  companionBleTick();
  CHECK_EQ(controlCalls, 1u);
  CHECK_EQ(response(owner)["id"].as<int>(), 9);
}
TEST_CASE("BLE backend bounds work per tick and explicitly aborts overflowing windows") {
  reset();
  NimBLEConnInfo owner(1);
  NimBLEDevice::getServer()->connect(owner);
  for (unsigned id = 1; id <= 8; ++id)
    writeControl(owner, id);
  companionBleTick();
  CHECK_EQ(controlCalls, 4u);
  CHECK_EQ(response(owner)["id"].as<int>(), 4);
  companionBleTick();
  CHECK_EQ(controlCalls, 8u);
  CHECK_EQ(response(owner)["id"].as<int>(), 8);
  for (unsigned id = 1; id <= 9; ++id)
    writeControl(owner, id);
  companionBleTick();
  CHECK_EQ(controlCalls, 8u);
  CHECK(response(owner)["error"] == "queue-full");
  writeControl(owner, 99);
  companionBleTick();
  CHECK_EQ(controlCalls, 9u);
  CHECK_EQ(response(owner)["id"].as<int>(), 99);
}
TEST_CASE("BLE backend suppresses old replies and keeps failed windows from becoming success") {
  reset();
  NimBLEConnInfo owner(1);
  auto *server = NimBLEDevice::getServer();
  server->connect(owner);
  duringRpc = [&] {
    server->disconnected(owner);
    server->connect(owner);
  };
  writeControl(owner, 11);
  companionBleTick();
  CHECK_EQ(response(owner)["id"].as<int>(), 0);
  failNext = true;
  writeControl(owner, 12);
  writeControl(owner, 13);
  companionBleTick();
  CHECK_EQ(response(owner)["id"].as<int>(), 12);
  CHECK(response(owner)["error"] == "test-error");
  const auto count = controlCalls;
  companionBleTick();
  CHECK_EQ(controlCalls, count);
  oversizeReply = true;
  writeControl(owner, 14);
  companionBleTick();
  CHECK(response(owner)["error"] == "response-too-large");
}
TEST_CASE("BLE backend handles invalid packet lengths startup failures and repeated teardown") {
  reset();
  NimBLEConnInfo owner(1);
  NimBLEDevice::getServer()->connect(owner);
  characteristic(controlId).write(owner, {});
  characteristic(controlId).write(owner, std::vector<uint8_t>(513, 'x'));
  companionBleTick();
  CHECK_EQ(controlCalls, 0u);
  companionBleEnd();
  CHECK(!companionBleRunning());
  CHECK(!companionBleConnected());
  CHECK(companionPairingPin() == "");
  companionBleEnd();
  CHECK_EQ(TestCritical::depth, 0);
  TestRadio::initSuccess = false;
  companionBleBegin();
  CHECK(!companionBleRunning());
  TestRadio::initSuccess = true;
  TestRadio::advertiseSuccess = false;
  const auto deinit = TestRadio::deinitCalls;
  companionBleBegin();
  CHECK(!companionBleRunning());
  CHECK_EQ(TestRadio::deinitCalls, deinit + 1);
}

TEST_CASE("BLE backend routes bounded binary data separately from control JSON") {
  reset();
  NimBLEConnInfo owner(1), unauthorized(1, false, false);
  NimBLEDevice::getServer()->connect(owner);
  std::vector<uint8_t> bytes(512, 0);
  bytes[0] = 123;
  characteristic(dataId).write(unauthorized, bytes);
  characteristic(dataId).write(owner, bytes);
  companionBleTick();
  CHECK_EQ(dataCalls, 1u);
  CHECK_EQ(controlCalls, 0u);
  CHECK_EQ(response(owner)["id"].as<int>(), 123);
  bytes.push_back(0);
  characteristic(dataId).write(owner, bytes);
  companionBleTick();
  CHECK_EQ(dataCalls, 1u);
}

TEST_CASE("BLE authentication failure revokes queued work before asynchronous disconnect") {
  reset();
  NimBLEConnInfo owner(1), revoked(1, true, false), replacement(2);
  auto *server = NimBLEDevice::getServer();
  server->connect(owner);
  server->authenticate(owner);
  writeControl(owner, 21);
  server->authenticate(revoked);
  CHECK(!companionBleConnected());
  companionBleTick();
  CHECK_EQ(controlCalls, 0u);
  server->connect(replacement);
  server->authenticate(replacement);
  server->disconnected(owner);
  writeControl(replacement, 22);
  companionBleTick();
  CHECK(companionBleConnected());
  CHECK_EQ(controlCalls, 1u);
  CHECK_EQ(response(replacement)["id"].as<int>(), 22);
}
