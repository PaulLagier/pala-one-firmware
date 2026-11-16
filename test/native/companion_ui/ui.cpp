#include "test_framework.h"
#include "runtime.h"
#include "src/ui/screens/upload_screen.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/reader_actions.h"
#include "src/hal/wifi_provisioning.h"
LibraryScreen g_libraryScreen;
unsigned bleBegins = 0, bleEnds = 0, bleTicks = 0, wifiBegins = 0, wifiEnds = 0, bookResets = 0,
         sleepResets = 0, appResets = 0, catalogLoads = 0;
bool bleResult = true, bleActive = false, bleConnected = false, bleRedraw = false,
     staResult = false, legacy = true;
WifiStaResult pollResult = WifiStaResult::Connecting;
void companionBleBegin() {
  ++bleBegins;
  bleActive = bleResult;
  bleRedraw = true;
}
void companionBleEnd() {
  ++bleEnds;
  bleActive = false;
}
void companionBleTick() { ++bleTicks; }
bool companionBleRunning() { return bleActive; }
bool companionBleConnected() { return bleConnected; }
bool companionBleNeedsRedraw() {
  bool value = bleRedraw;
  bleRedraw = false;
  return value;
}
String companionPairingPin() { return "654321"; }
bool wifiStaBegin() {
  ++wifiBegins;
  return staResult;
}
WifiStaResult wifiStaPoll(WifiSession &out) {
  out.mode = WifiMode::Station;
  out.staSsid = "test-wifi";
  out.primaryUrl = "http://pala-one.local";
  return pollResult;
}
void wifiStaAbort() {}
WifiSession wifiBeginAccessPoint() {
  WifiSession net;
  net.primaryUrl = "http://192.168.4.1";
  return net;
}
void wifiEnd() {
  ++wifiEnds;
  setCpuFrequencyMhz(80);
}
void resetBookUpload() { ++bookResets; }
void resetSleepUpload() { ++sleepResets; }
void resetAppUpload() { ++appResets; }
void loadBooks() { ++catalogLoads; }
namespace Gestures {
bool legacyControlsOn() { return legacy; }
ButtonAction actionFor(ButtonEvent::Kind kind) {
  return kind == ButtonEvent::Short    ? ACTION_NEXT
         : kind == ButtonEvent::Double ? ACTION_PREV
         : kind == ButtonEvent::Triple ? ACTION_HOME
         : kind == ButtonEvent::Long   ? ACTION_MENU
                                       : ACTION_NONE;
}
bool resolveLegacyAction(const ButtonEvent &e, const ButtonEvent::Kind &kind, ButtonAction action) {
  return legacy ? e.kind == kind : actionFor(e.kind) == action;
}
bool isNonLegacyAction(const ButtonEvent &e, ButtonAction action) {
  return !legacy && actionFor(e.kind) == action;
}
} // namespace Gestures
namespace {
struct Fixture {
  UploadScreen screen;
  Fixture() {
    bleBegins = bleEnds = bleTicks = wifiBegins = wifiEnds = bookResets = sleepResets = appResets =
        catalogLoads = 0;
    bleResult = true;
    bleActive = bleConnected = bleRedraw = staResult = false;
    legacy = true;
    pollResult = WifiStaResult::Connecting;
    mockMillis = 0;
    resets = updates = lastCpuMHz = 0;
    server = MockServer{};
    WifiProvisioning::session = false;
    screen.onEnter();
  }
  void send(ButtonEvent::Kind kind) {
    ButtonEvent event;
    event.kind = kind;
    screen.onButton(event);
  }
  bool shows(const char *text) {
    for (const auto &line : rendered)
      if (line.text == text)
        return true;
    return false;
  }
  void app() {
    send(ButtonEvent::Short);
    send(ButtonEvent::Double);
    screen.onIdleTick();
  }
};
} // namespace
TEST_CASE("Connection chooser retains WiFi first and does not start a radio on entry") {
  Fixture f;
  CHECK(f.screen.allowSleep());
  CHECK(f.shows(D_BLE_CHOICE_WIFI));
  CHECK(f.shows(D_BLE_CHOICE_APP));
  CHECK_EQ(wifiBegins, 0u);
  CHECK_EQ(bleBegins, 0u);
  f.send(ButtonEvent::Triple);
  CHECK(f.screen.nextScreen == &g_libraryScreen);
  CHECK_EQ(wifiEnds, 0u);
  CHECK_EQ(bleEnds, 0u);
}
TEST_CASE("Existing WiFi upload flow and cleanup remain available") {
  Fixture f;
  f.send(ButtonEvent::Double);
  CHECK_EQ(wifiBegins, 1u);
  CHECK_EQ(bleBegins, 0u);
  CHECK(!f.screen.allowSleep());
  CHECK_EQ(server.starts, 1u);
  CHECK(WifiProvisioning::session);
  f.screen.onIdleTick();
  CHECK_EQ(server.polls, 1u);
  f.send(ButtonEvent::Short);
  CHECK(f.screen.nextScreen == &g_libraryScreen);
  CHECK_EQ(wifiEnds, 1u);
  CHECK_EQ(bleEnds, 0u);
  CHECK_EQ(server.stops, 1u);
  CHECK(!WifiProvisioning::session);
}
TEST_CASE("App connection starts on idle tick at full speed and avoids a redundant redraw") {
  Fixture f;
  f.send(ButtonEvent::Short);
  f.send(ButtonEvent::Double);
  CHECK_EQ(bleBegins, 0u);
  CHECK(f.shows(D_BLE_STARTING));
  f.screen.onIdleTick();
  CHECK_EQ(bleBegins, 1u);
  CHECK_EQ(wifiBegins, 0u);
  CHECK_EQ(lastCpuMHz, 240u);
  CHECK(!f.screen.allowSleep());
  CHECK(f.shows("654321"));
  CHECK(f.shows(D_BLE_EXIT_LEGACY));
  const auto draws = updates;
  f.screen.onIdleTick();
  CHECK_EQ(updates, draws);
  CHECK_EQ(bleTicks, 1u);
  bleConnected = bleRedraw = true;
  f.screen.onIdleTick();
  CHECK(f.shows(D_BLE_CONNECTED));
  for (const auto &row : rendered)
    CHECK(row.y < SCREEN_H);
  f.send(ButtonEvent::Short);
  CHECK_EQ(bleEnds, 1u);
  CHECK_EQ(lastCpuMHz, 80u);
  CHECK(f.screen.nextScreen == &g_libraryScreen);
}
TEST_CASE("App startup failure permits idle sleep and standard Home exit") {
  Fixture f;
  bleResult = false;
  legacy = false;
  f.send(ButtonEvent::Short);
  f.send(ButtonEvent::Long);
  f.screen.onIdleTick();
  CHECK(f.shows(D_BLE_ERROR));
  CHECK(f.shows(D_BLE_EXIT_STANDARD));
  CHECK(f.screen.allowSleep());
  CHECK_EQ(lastCpuMHz, 80u);
  f.send(ButtonEvent::Short);
  CHECK(f.screen.nextScreen == nullptr);
  f.send(ButtonEvent::Triple);
  CHECK(f.screen.nextScreen == &g_libraryScreen);
  CHECK_EQ(bleEnds, 1u);
}
TEST_CASE("Connection sessions exit at the original deadline including millis wrap") {
  Fixture f;
  mockMillis = 0xfffffff0u;
  f.app();
  mockMillis += UPLOAD_AUTO_EXIT_MS + 1;
  f.screen.onIdleTick();
  CHECK(f.screen.nextScreen == &g_libraryScreen);
  CHECK_EQ(bleEnds, 1u);
  CHECK(!WifiProvisioning::session);
}

TEST_CASE("Cancelling before the Bluetooth idle start never starts a background radio") {
  Fixture f;
  f.send(ButtonEvent::Short);
  f.send(ButtonEvent::Double);
  CHECK(f.shows(D_BLE_STARTING));
  f.send(ButtonEvent::Short);
  REQUIRE(f.screen.nextScreen == &g_libraryScreen);
  f.screen.onIdleTick();
  CHECK_EQ(bleBegins, 0u);
  CHECK_EQ(wifiEnds, 0u);
  CHECK(!WifiProvisioning::session);
}
