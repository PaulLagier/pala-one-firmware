#include "test_framework.h"
#include "serial.h"
#include <WiFi.h>
#include "src/storage/wifi_creds.h"
#include "src/hal/wifi_provisioning.h"
TestSerial Serial;
TestWiFi WiFi;
namespace {
void reset() {
  Serial.input.clear();
  testMillis += 300001;
  WifiProvisioning::loop();
  WifiProvisioning::notifyUploadSession(false);
  WiFi = {};
  Serial.output.clear();
  WifiCreds::savedSsid = "old";
  WifiCreds::savedPassword = "keep";
  WifiCreds::saves = 0;
  WifiProvisioning::begin();
  HWCDC::plugged = true;
}
void command(uint8_t cmd, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> bytes = {
      'I', 'M', 'P', 'R', 'O', 'V', 1, 3, uint8_t(data.size() + 2), cmd, uint8_t(data.size())};
  bytes.insert(bytes.end(), data.begin(), data.end());
  uint8_t sum = 0;
  for (auto b : bytes)
    sum += b;
  bytes.push_back(sum);
  Serial.input.insert(Serial.input.end(), bytes.begin(), bytes.end());
}
void drain() {
  while (Serial.available())
    WifiProvisioning::loop();
}
void credentials(const char *ssid = "new", const char *password = "secret") {
  std::string s = ssid, p = password;
  std::vector<uint8_t> data = {uint8_t(s.size())};
  data.insert(data.end(), s.begin(), s.end());
  data.push_back(uint8_t(p.size()));
  data.insert(data.end(), p.begin(), p.end());
  command(1, data);
  drain();
}
} // namespace
TEST_CASE("An idle attached USB host cannot inhibit sleep indefinitely") {
  reset();
  CHECK(!WifiProvisioning::isActive());
  CHECK_EQ(WiFi.radioCalls, 0u);
  testMillis = 0;
  command(3, {});
  CHECK(WifiProvisioning::isActive());
  drain();
  CHECK(WifiProvisioning::isActive());
  testMillis = 299999;
  CHECK(WifiProvisioning::isActive());
  testMillis = 300000;
  CHECK(!WifiProvisioning::isActive());
  WifiProvisioning::loop();
  CHECK(!Serial.output.empty());
  CHECK_EQ(WiFi.radioCalls, 0u);
}
TEST_CASE("USB activity expiration is wrap-safe and new queued commands revive it") {
  reset();
  testMillis = 0xfffffff0u;
  command(3, {});
  drain();
  testMillis += 299999;
  CHECK(WifiProvisioning::isActive());
  testMillis += 1;
  CHECK(!WifiProvisioning::isActive());
  command(3, {});
  CHECK(WifiProvisioning::isActive());
  drain();
  CHECK(WifiProvisioning::isActive());
}
TEST_CASE("Failed association releases the USB-owned radio without changing saved credentials") {
  reset();
  WiFi.failConnection = true;
  const auto start = testMillis;
  credentials();
  CHECK_EQ(testMillis, start);
  CHECK_EQ(WifiCreds::saves, 0u);
  testMillis += 30000;
  WifiProvisioning::loop();
  CHECK_EQ(WiFi.radioMode, WIFI_OFF);
  CHECK_EQ(WiFi.modeCalls, 1u);
  CHECK_EQ(WifiCreds::saves, 0u);
  CHECK(WifiCreds::savedSsid == "old");
  WiFi.failConnection = false;
  credentials("retry", "correct");
  CHECK_EQ(WifiCreds::saves, 1u);
  CHECK(WifiCreds::savedSsid == "retry");
  testMillis += 2001;
  WifiProvisioning::loop();
  CHECK_EQ(WiFi.radioMode, WIFI_OFF);
}
TEST_CASE("A newer association survives the previous success grace timer") {
  reset();
  credentials();
  REQUIRE(WifiCreds::saves == 1);
  testMillis += 1000;
  WiFi.failConnection = true;
  credentials("second", "two");
  testMillis += 2001;
  WifiProvisioning::loop();
  CHECK(WiFi.radioMode != WIFI_OFF);
  WiFi.connected = true;
  WifiProvisioning::loop();
  CHECK(WifiCreds::savedSsid == "second");
  testMillis += 2001;
  WifiProvisioning::loop();
  CHECK_EQ(WiFi.radioMode, WIFI_OFF);
}
TEST_CASE("Upload-owned radios are not switched off by USB failure success or expiry") {
  for (bool success : {false, true}) {
    reset();
    WifiProvisioning::notifyUploadSession(true);
    WiFi.radioMode = 2;
    WiFi.failConnection = !success;
    credentials();
    if (!success) {
      testMillis += 30000;
      WifiProvisioning::loop();
    }
    testMillis += 300001;
    WifiProvisioning::loop();
    CHECK_EQ(WiFi.modeCalls, 0u);
    CHECK((WiFi.radioMode & 2) != 0);
  }
}
TEST_CASE("Radio ownership transfer cancels stale pending USB credential callbacks") {
  reset();
  WiFi.failConnection = true;
  credentials();
  WifiProvisioning::notifyUploadSession(true);
  CHECK_EQ(WiFi.radioMode, WIFI_OFF);
  WiFi.radioMode = 2;
  WiFi.connected = true;
  testMillis += 31000;
  WifiProvisioning::loop();
  CHECK_EQ(WifiCreds::saves, 0u);
  CHECK_EQ(WiFi.radioMode, 2);
}
