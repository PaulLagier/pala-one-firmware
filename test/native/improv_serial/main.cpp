#include "test_framework.h"
#include <ImprovWiFiLibrary.h>
#include <deque>
#include <algorithm>

TestWiFi WiFi;
struct Wire : Stream {
  std::deque<uint8_t> input;
  std::vector<uint8_t> output;
  int available() override { return int(input.size()); }
  int read() override {
    if (input.empty())
      return -1;
    auto b = input.front();
    input.pop_front();
    return b;
  }
  size_t write(const uint8_t *bytes, size_t n) override {
    output.insert(output.end(), bytes, bytes + n);
    return n;
  }
};
using Bytes = std::vector<uint8_t>;
static Bytes frame(const Bytes &payload) {
  Bytes bytes = {'I', 'M', 'P', 'R', 'O', 'V', 1, 3, uint8_t(payload.size())};
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  uint8_t checksum = 0;
  for (auto b : bytes)
    checksum += b;
  bytes.push_back(checksum);
  return bytes;
}
static void sendRaw(ImprovWiFi &parser, Wire &wire, const Bytes &bytes) {
  for (auto b : bytes)
    wire.input.push_back(b);
  while (wire.available())
    parser.handleSerial();
}
static void send(ImprovWiFi &parser, Wire &wire, const Bytes &payload) {
  sendRaw(parser, wire, frame(payload));
}
static void setup(ImprovWiFi &parser) {
  parser.setDeviceInfo(ImprovTypes::CF_ESP32_S3, "Pala One", "serial-test", "Pala One");
}
static bool validFrame(const Bytes &bytes) {
  if (bytes.size() < 10 || bytes.size() != size_t(bytes[8]) + 10)
    return false;
  const Bytes signature = {'I', 'M', 'P', 'R', 'O', 'V', 1};
  if (!std::equal(signature.begin(), signature.end(), bytes.begin()))
    return false;
  uint8_t sum = 0;
  for (size_t i = 0; i + 1 < bytes.size(); ++i)
    sum += bytes[i];
  return sum == bytes.back();
}
static bool validReply(const Bytes &bytes) { return validFrame(bytes) && bytes[7] == 4; }
static bool validFrames(const Bytes &bytes) {
  size_t offset = 0;
  while (offset < bytes.size()) {
    if (bytes.size() - offset < 10)
      return false;
    size_t length = size_t(bytes[offset + 8]) + 10;
    if (length > bytes.size() - offset)
      return false;
    if (!validFrame(Bytes(bytes.begin() + offset, bytes.begin() + offset + length)))
      return false;
    offset += length;
  }
  return true;
}
static std::string savedSsid, savedPassword;
static void connected(const char *ssid, const char *password) {
  savedSsid = ssid;
  savedPassword = password;
}
static Bytes credentials(const std::string &ssid, const std::string &password) {
  Bytes data = {1, uint8_t(ssid.size() + password.size() + 2), uint8_t(ssid.size())};
  data.insert(data.end(), ssid.begin(), ssid.end());
  data.push_back(uint8_t(password.size()));
  data.insert(data.end(), password.begin(), password.end());
  return data;
}

TEST_CASE("every consecutive device-info query gets a checksummed reply") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  for (int i = 0; i < 30; ++i) {
    wire.output.clear();
    send(parser, wire, {3, 0});
    REQUIRE(validReply(wire.output));
    CHECK_EQ(wire.output[9], 3);
    const std::string output(wire.output.begin(), wire.output.end());
    CHECK(output.find("serial-test") != std::string::npos);
  }
}
TEST_CASE("short malformed credential records never call WiFi begin and recover") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  for (const Bytes &data : std::vector<Bytes>{{},
                                              {1},
                                              {1, 0},
                                              {1, 1, 255},
                                              {1, 3, 1, 'x', 255},
                                              {1, 5, 2, 'x', 'y', 10, 'z'},
                                              {1, 2, 0, 0}}) {
    send(parser, wire, data);
    CHECK_EQ(WiFi.beginCalls, 0u);
    CHECK(validFrames(wire.output));
    wire.output.clear();
    send(parser, wire, {3, 0});
    CHECK(validReply(wire.output));
  }
}
TEST_CASE("all receive lengths fit or are rejected without memory corruption") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  for (unsigned length = 0; length < 256; ++length) {
    Bytes data(length, 0);
    if (length)
      data[0] = 0xfe;
    if (length > 1)
      data[1] = uint8_t(length - 2);
    send(parser, wire, data);
    CHECK_EQ(WiFi.beginCalls, 0u);
    CHECK(validFrames(wire.output));
    wire.output.clear();
    send(parser, wire, {3, 0});
    CHECK(validReply(wire.output));
  }
}
TEST_CASE("valid credentials preserve special characters and open-network passwords") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  parser.onImprovConnected(connected);
  for (const auto &password : {std::string("correct!password"), std::string()}) {
    const std::string ssid = "Pala! café";
    wire.output.clear();
    send(parser, wire, credentials(ssid, password));
    CHECK(validFrames(wire.output));
    CHECK(savedSsid == ssid);
    CHECK(savedPassword == password);
  }
  CHECK_EQ(WiFi.beginCalls, 2u);
}
TEST_CASE("bad checksums and signatures do not connect or poison the next request") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  Bytes bad = frame(credentials("test-network", "test-password"));
  bad.back() ^= 0x40;
  sendRaw(parser, wire, bad);
  CHECK_EQ(WiFi.beginCalls, 0u);
  wire.output.clear();
  send(parser, wire, {3, 0});
  CHECK(validReply(wire.output));
  sendRaw(parser, wire, {'I', 'M', 'I', 'M'}); // Overlap with the next full signature.
  wire.output.clear();
  send(parser, wire, {3, 0});
  CHECK(validReply(wire.output));
  CHECK_EQ(WiFi.beginCalls, 0u);
}
TEST_CASE("partial frames expire between bytes including millis wraparound") {
  for (uint32_t start : {0u, UINT32_MAX - 100u}) {
    testMillis = start;
    Wire wire;
    ImprovWiFi parser(&wire);
    setup(parser);
    WiFi = {};
    Bytes partial = frame({3, 0});
    partial.resize(10);
    sendRaw(parser, wire, partial);
    testMillis += 1001;
    wire.output.clear();
    send(parser, wire, {3, 0});
    CHECK(validReply(wire.output));
    CHECK_EQ(WiFi.beginCalls, 0u);
  }
}
TEST_CASE("timeout is inter-byte rather than a deadline for the entire frame") {
  testMillis = 0;
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  for (auto byte : frame({3, 0})) {
    testMillis += 500;
    sendRaw(parser, wire, {byte});
  }
  CHECK(validReply(wire.output));
}
TEST_CASE("explicit parser reset discards an incomplete frame") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  sendRaw(parser, wire, {'I', 'M', 'P', 'R', 'O'});
  parser.resetSerialParser();
  send(parser, wire, {3, 0});
  CHECK(validReply(wire.output));
}

static unsigned languageCalls = 0;
static std::string selectedLanguage;
static bool acceptLanguage(const char *code) {
  ++languageCalls;
  if (std::string(code) != "de" && std::string(code) != "fr")
    return false;
  selectedLanguage = code;
  return true;
}
TEST_CASE("vendor language RPC accepts exact codes and keeps ordinary device queries usable") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  languageCalls = 0;
  selectedLanguage.clear();
  parser.onSetLanguage(acceptLanguage);
  for (const std::string code : {"de", "fr"}) {
    wire.output.clear();
    send(parser, wire, {0x81, 2, uint8_t(code[0]), uint8_t(code[1])});
    CHECK_EQ(selectedLanguage, code);
    CHECK(validFrames(wire.output));
    CHECK(std::find(wire.output.begin(), wire.output.end(), uint8_t(0x81)) != wire.output.end());
  }
  CHECK_EQ(languageCalls, 2u);
  CHECK_EQ(WiFi.beginCalls, 0u);
  wire.output.clear();
  send(parser, wire, {3, 0});
  CHECK(validReply(wire.output));
}
TEST_CASE("malformed or unsupported vendor languages cannot change the selection") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  languageCalls = 0;
  selectedLanguage = "fr";
  parser.onSetLanguage(acceptLanguage);
  for (const Bytes payload : {Bytes{0x81, 0}, Bytes{0x81, 1, 'd'}, Bytes{0x81, 3, 'd', 'e', 0},
                              Bytes{0x81, 2, 'D', 'E'}, Bytes{0x81, 2, 0, 'e'}}) {
    send(parser, wire, payload);
    CHECK_EQ(languageCalls, 0u);
    CHECK_EQ(selectedLanguage, "fr");
    wire.output.clear();
    send(parser, wire, {3, 0});
    CHECK(validReply(wire.output));
  }
  send(parser, wire, {0x81, 2, 'z', 'z'});
  CHECK_EQ(languageCalls, 1u);
  CHECK_EQ(selectedLanguage, "fr");
  parser.onSetLanguage(nullptr);
  send(parser, wire, {0x81, 2, 'd', 'e'});
  CHECK_EQ(languageCalls, 1u);
  CHECK_EQ(selectedLanguage, "fr");
  CHECK_EQ(WiFi.beginCalls, 0u);
}

static unsigned menuCalls = 0;
static bool menuValue = false;
static bool menuWrites = true;
static bool acceptMenu(bool value) {
  ++menuCalls;
  if (!menuWrites)
    return false;
  menuValue = value;
  return true;
}
TEST_CASE("vendor menu RPC accepts exact booleans and rejects malformed payloads") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  menuCalls = 0;
  menuValue = false;
  menuWrites = true;
  parser.onSetReadingMenu(acceptMenu);
  for (const uint8_t value : {'1', '0'}) {
    wire.output.clear();
    send(parser, wire, {0x82, 1, value});
    CHECK_EQ(menuValue, value == '1');
    CHECK(validFrames(wire.output));
    CHECK(std::find(wire.output.begin(), wire.output.end(), uint8_t(0x82)) != wire.output.end());
  }
  CHECK_EQ(menuCalls, 2u);
  for (const Bytes payload :
       {Bytes{0x82, 0}, Bytes{0x82, 1, 1}, Bytes{0x82, 1, '2'}, Bytes{0x82, 2, '1', '0'}}) {
    send(parser, wire, payload);
    CHECK_EQ(menuCalls, 2u);
    CHECK(!menuValue);
    wire.output.clear();
    send(parser, wire, {3, 0});
    CHECK(validReply(wire.output));
  }
  menuWrites = false;
  send(parser, wire, {0x82, 1, '1'});
  CHECK(!menuValue);
  parser.onSetReadingMenu(nullptr);
  send(parser, wire, {0x82, 1, '1'});
  CHECK_EQ(menuCalls, 3u);
  CHECK_EQ(WiFi.beginCalls, 0u);
}

TEST_CASE("uninitialized storage setup accepts local settings without touching the radio") {
  Wire wire;
  ImprovWiFi parser(&wire);
  setup(parser);
  WiFi = {};
  languageCalls = menuCalls = 0;
  menuWrites = true;
  selectedLanguage.clear();
  menuValue = false;
  parser.onSetLanguage(acceptLanguage);
  parser.onSetReadingMenu(acceptMenu);
  parser.setLocalSetupOnly(true);
  send(parser, wire, {3, 0});
  CHECK(validReply(wire.output));
  wire.output.clear();
  send(parser, wire, {0x81, 2, 'd', 'e'});
  CHECK_EQ(selectedLanguage, "de");
  CHECK_EQ(languageCalls, 1u);
  CHECK(validFrames(wire.output));
  wire.output.clear();
  send(parser, wire, {0x82, 1, '1'});
  CHECK(menuValue);
  CHECK_EQ(menuCalls, 1u);
  CHECK(validFrames(wire.output));
  for (const Bytes request : {Bytes{2, 0}, Bytes{4, 0}, Bytes{1, 4, 1, 'a', 1, 'b'}}) {
    wire.output.clear();
    send(parser, wire, request);
    CHECK(validFrames(wire.output));
    CHECK_EQ(WiFi.radioCalls, 0u);
    CHECK_EQ(WiFi.beginCalls, 0u);
  }
  wire.output.clear();
  send(parser, wire, {3, 0});
  CHECK(validReply(wire.output));
  parser.setLocalSetupOnly(false);
  send(parser, wire, {2, 0});
  CHECK(WiFi.radioCalls > 0);
}
