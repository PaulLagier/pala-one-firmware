#include "src/storage/navigation_profile.h"
// Implementation of the Improv Serial Wi-Fi provisioning wrapper.
//
// The on-device protocol work is done by jnthas/Improv-WiFi-Library
// (serial subset vendored under src/third_party/improv) — this file owns the
// ImprovWiFi instance, wires its callbacks, drives it from loop(), and
// coordinates Wi-Fi ownership with the upload session. See the header for the
// "what / why" of Improv and the listening model.
#include "src/hal/wifi_provisioning.h"

#include "src/third_party/improv/ImprovWiFiLibrary.h"
#include <WiFi.h>

#include "src/config.h"
#include "src/storage/wifi_creds.h"

namespace WifiProvisioning {

namespace {
struct State {
  ImprovWiFi lib{&Serial};
  bool uploadSession = false;
  bool credsReceived = false;
  uint32_t credsReceivedMs = 0;
  bool ownsWifi = false; // true iff we brought Wi-Fi up to verify creds
  uint32_t lastHostByteMs = 0;
  bool hostTrafficSeen = false;
};

State s_state;
} // namespace

// After the credentials callback fires we give the library this much time to
// flush its "Provisioned" reply over USB-CDC before tearing Wi-Fi down. The
// reply itself takes microseconds at 115200 baud; this is just headroom.
static constexpr uint32_t kShutdownGraceMs = 2000;

// How long after the last byte received from the host we still treat the
// device as engaged for the sleep gate. Long enough to span a slow user
// finding their Wi-Fi password in a list and typing it; short enough that a
// genuinely abandoned session eventually lets the device idle.
static constexpr uint32_t kHostActivityMs = 5 * 60 * 1000;

// USB power or SOF packets alone must not disable automatic sleep forever.
// Pending bytes protect a request arriving immediately before the sleep gate;
// the explicit flag also makes a first byte at millis()==0 and rollover valid.
static bool hostActive() {
  return Serial.available() > 0 ||
         (s_state.hostTrafficSeen &&
          static_cast<uint32_t>(millis() - s_state.lastHostByteMs) < kHostActivityMs);
}

static void releaseWifi() {
  if (!s_state.ownsWifi)
    return;
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  s_state.ownsWifi = false;
}

static void clearGraceTimer() {
  s_state.credsReceived = false;
  s_state.credsReceivedMs = 0;
}

void beginLocalSetup() {
  NavigationProfile::loadSettings();
  s_state.lib.setLocalSetupOnly(true);
  s_state.lib.onSetReadingMenu(
      [](bool enabled) { return NavigationProfile::setReadingFocused(enabled); });
  s_state.lib.onSetLanguage([](const char *code) { return Language::setLanguage(code); });
  s_state.lib.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32_S3, "Pala One", FW_VERSION,
                            "Pala One",
                            "https://paullagier.github.io/pala-one-firmware/connected.html");
}
void loopLocalSetup() {
  if (Serial.available() > 0) {
    s_state.lastHostByteMs = millis();
    s_state.hostTrafficSeen = true;
    s_state.lib.handleSerial();
  }
}
void begin() {
  beginLocalSetup();
  s_state.lib.setLocalSetupOnly(false);
  s_state.lib.setCooperativeConnections(true);
  s_state.lib.onImprovError([](ImprovTypes::Error error) {
    if (error == ImprovTypes::ERROR_UNABLE_TO_CONNECT && !s_state.uploadSession)
      releaseWifi();
  });

  // The library handles WiFi.begin() itself; we only persist creds once the
  // association succeeds. If the association fails the library reports the
  // error back to the browser and the saved creds stay untouched, which is
  // exactly what we want (don't poison NVS with bad creds).
  //
  // Association is advanced by loop(), so button input and screen updates
  // continue while DHCP/association completes. The SDK's STA begin preserves AP.
  s_state.lib.onImprovConnected([](const char *ssid, const char *password) {
    WifiCreds::save(ssid, password);
    if (!s_state.uploadSession) {
      // We brought Wi-Fi up just to verify creds — schedule teardown after
      // the library finishes sending its "Provisioned" reply.
      s_state.ownsWifi = true;
      s_state.credsReceived = true;
      s_state.credsReceivedMs = millis();
    }
    // Upload session: leave Wi-Fi alone — upload screen owns it.
  });
}

void notifyUploadSession(bool active) {
  // Cancel the USB attempt before another screen takes ownership of the radio.
  // Never switch off an upload-owned radio from a later USB callback.
  s_state.lib.cancelConnection();
  releaseWifi();
  clearGraceTimer();
  s_state.uploadSession = active;
}

bool isActive() { return hostActive(); }

void loop() {
  // Track actual host traffic for the finite provisioning activity window.
  // Do this *before* handleSerial drains the buffer so we don't miss the
  // moment the browser starts talking.
  if (Serial.available() > 0) {
    s_state.lastHostByteMs = millis();
    s_state.hostTrafficSeen = true;
  }

  if (!hostActive()) {
    s_state.lib.cancelConnection();
    // The host stopped sending commands. Release only our own radio state.
    releaseWifi();
    clearGraceTimer();
    s_state.hostTrafficSeen = false;
    return;
  }

  if (s_state.credsReceived && (uint32_t)(millis() - s_state.credsReceivedMs) > kShutdownGraceMs) {
    releaseWifi();
    clearGraceTimer();
    // Keep listening — host might want to re-provision a different network.
  }

  s_state.lib.handleSerial();
  if (s_state.lib.connectionPending() && !s_state.uploadSession) {
    s_state.ownsWifi = true;
    clearGraceTimer();
  }
}

} // namespace WifiProvisioning
