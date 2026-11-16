#pragma once
#include <Arduino.h>
#include <stdexcept>

namespace TestRadio {
inline bool initSuccess = true, advertiseSuccess = true;
inline unsigned deinitCalls = 0;
inline std::function<void()> readHook;
inline void outsideCritical() {
  if (TestCritical::depth)
    throw std::runtime_error("SDK call inside critical section");
}
} // namespace TestRadio
constexpr int BLE_HS_IO_DISPLAY_ONLY = 0;
namespace NIMBLE_PROPERTY {
constexpr uint32_t READ = 1, WRITE = 2, WRITE_NR = 4, NOTIFY = 8, READ_ENC = 16, READ_AUTHEN = 32,
                   WRITE_ENC = 64, WRITE_AUTHEN = 128;
}
class NimBLEConnInfo {
public:
  NimBLEConnInfo(uint16_t handle, bool encrypted = true, bool authenticated = true)
      : handle_(handle), encrypted_(encrypted), authenticated_(authenticated) {}
  uint16_t getConnHandle() const { return handle_; }
  bool isEncrypted() const { return encrypted_; }
  bool isAuthenticated() const { return authenticated_; }

private:
  uint16_t handle_;
  bool encrypted_, authenticated_;
};
class NimBLEServer;
class NimBLECharacteristic;
class NimBLEServerCallbacks {
public:
  virtual ~NimBLEServerCallbacks() = default;
  virtual void onConnect(NimBLEServer *, NimBLEConnInfo &) {}
  virtual void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) {}
  virtual void onAuthenticationComplete(NimBLEConnInfo &) {}
};
class NimBLECharacteristicCallbacks {
public:
  virtual ~NimBLECharacteristicCallbacks() = default;
  virtual void onRead(NimBLECharacteristic *, NimBLEConnInfo &) {}
  virtual void onWrite(NimBLECharacteristic *, NimBLEConnInfo &) {}
};
class NimBLECharacteristic {
public:
  uint32_t properties;
  size_t maximum;
  NimBLECharacteristicCallbacks *callbacks = nullptr;
  std::vector<uint8_t> value, signal;
  unsigned notifications = 0;
  NimBLECharacteristic(uint32_t flags, size_t size) : properties(flags), maximum(size) {}
  void setCallbacks(NimBLECharacteristicCallbacks *cb) { callbacks = cb; }
  void setValue(const char *text) {
    TestRadio::outsideCritical();
    value.assign(text, text + std::strlen(text));
  }
  std::vector<uint8_t> getValue() const {
    TestRadio::outsideCritical();
    if (TestRadio::readHook) {
      auto hook = std::move(TestRadio::readHook);
      TestRadio::readHook = nullptr;
      hook();
    }
    return value;
  }
  void notify(const uint8_t *bytes, size_t length) {
    TestRadio::outsideCritical();
    signal.assign(bytes, bytes + length);
    ++notifications;
  }
  void write(NimBLEConnInfo &peer, const std::vector<uint8_t> &bytes) {
    value = bytes;
    if (callbacks)
      callbacks->onWrite(this, peer);
  }
  std::string read(NimBLEConnInfo &peer) {
    if (callbacks)
      callbacks->onRead(this, peer);
    return {value.begin(), value.end()};
  }
};
class NimBLEService {
public:
  std::map<std::string, std::unique_ptr<NimBLECharacteristic>> characteristics;
  NimBLECharacteristic *createCharacteristic(const char *id, uint32_t flags, size_t size) {
    TestRadio::outsideCritical();
    auto object = std::make_unique<NimBLECharacteristic>(flags, size);
    auto *result = object.get();
    characteristics[id] = std::move(object);
    return result;
  }
};
class NimBLEServer {
public:
  NimBLEServerCallbacks *callbacks = nullptr;
  NimBLEService service;
  std::vector<uint16_t> rejected;
  bool advertiseAgain = false;
  void setCallbacks(NimBLEServerCallbacks *cb, bool) { callbacks = cb; }
  void advertiseOnDisconnect(bool value) { advertiseAgain = value; }
  void disconnect(uint16_t handle) {
    TestRadio::outsideCritical();
    rejected.push_back(handle);
  }
  void updateConnParams(uint16_t, int, int, int, int) { TestRadio::outsideCritical(); }
  NimBLEService *createService(const char *) { return &service; }
  void connect(NimBLEConnInfo peer) { callbacks->onConnect(this, peer); }
  void authenticate(NimBLEConnInfo peer) { callbacks->onAuthenticationComplete(peer); }
  void disconnected(NimBLEConnInfo peer) { callbacks->onDisconnect(this, peer, 0); }
};
class NimBLEAdvertising {
public:
  std::string service, name;
  bool scanResponse = false;
  void addServiceUUID(const char *id) { service = id; }
  void enableScanResponse(bool value) { scanResponse = value; }
  void setName(const char *value) { name = value; }
  bool start() { return TestRadio::advertiseSuccess; }
};
class NimBLEDevice {
public:
  inline static std::unique_ptr<NimBLEServer> server;
  inline static NimBLEAdvertising advertising;
  inline static bool bond = false, mitm = false, sc = false;
  inline static uint32_t passkey = 0;
  inline static int mtu = 0;
  static bool init(const char *) {
    TestRadio::outsideCritical();
    return TestRadio::initSuccess;
  }
  static void setMTU(int value) { mtu = value; }
  static void setSecurityAuth(bool b, bool m, bool s) {
    bond = b;
    mitm = m;
    sc = s;
  }
  static void setSecurityIOCap(int) {}
  static void setSecurityPasskey(uint32_t value) { passkey = value; }
  static NimBLEServer *createServer() {
    server = std::make_unique<NimBLEServer>();
    return server.get();
  }
  static NimBLEServer *getServer() { return server.get(); }
  static NimBLEAdvertising *getAdvertising() { return &advertising; }
  static void deinit(bool) {
    TestRadio::outsideCritical();
    server.reset();
    advertising = NimBLEAdvertising{};
    ++TestRadio::deinitCalls;
  }
  static void deleteAllBonds() { TestRadio::outsideCritical(); }
};
