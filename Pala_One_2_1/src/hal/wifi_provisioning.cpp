#include "src/hal/wifi_provisioning.h"

#include <ImprovWiFiLibrary.h>
#include <WiFi.h>

#include "src/config.h"
#include "src/storage/wifi_creds.h"

namespace WifiProvisioning {

namespace {
struct State {
  ImprovWiFi lib{&Serial};
  bool       uploadSession   = false;
  bool       credsReceived   = false;
  uint32_t   credsReceivedMs = 0;
  bool       ownsWifi        = false;  // true iff we brought Wi-Fi up to verify creds
  uint32_t   lastHostByteMs  = 0;      // 0 = no byte ever received this boot
};

State s_state;
}  // namespace

// After the credentials callback fires we give the library this much time to
// flush its "Provisioned" reply over USB-CDC before tearing Wi-Fi down. The
// reply itself takes microseconds at 115200 baud; this is just headroom.
static constexpr uint32_t kShutdownGraceMs = 2000;

// How long after the last byte received from the host we still treat the
// device as engaged for the sleep gate. Long enough to span a slow user
// finding their Wi-Fi password in a list and typing it; short enough that a
// genuinely abandoned session eventually lets the device idle.
static constexpr uint32_t kHostActivityMs = 5 * 60 * 1000;

// True when we believe a USB host is actively driving our bus. Combines two
// signals because each fails independently:
//
//   * HWCDC::isPlugged() polls for USB SOF packets. When it works, it's
//     authoritative — false means "no host on this cable". But on at least
//     one observed pioarduino + Heltec V3 build it returns 0 even while a
//     browser is plainly reading bytes (CDC link is up, logs are flowing).
//
//   * lastHostByteMs being recent means the host sent us a byte recently.
//     A polling tool (ESP Web Tools, Improv probe loops, a terminal with a
//     user typing) keeps this fresh; a wall charger or unplugged cable
//     never bumps it.
//
// Trusting either signal individually catches the failure mode of the other.
static bool hostPresent() {
  if (HWCDC::isPlugged()) return true;
  return s_state.lastHostByteMs != 0
      && (uint32_t)(millis() - s_state.lastHostByteMs) < kHostActivityMs;
}

static void releaseWifi() {
  if (!s_state.ownsWifi) return;
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  s_state.ownsWifi = false;
}

static void clearGraceTimer() {
  s_state.credsReceived   = false;
  s_state.credsReceivedMs = 0;
}

void begin() {
  s_state.lib.setDeviceInfo(
      ImprovTypes::ChipFamily::CF_ESP32_S3,
      "Pala One",
      FW_VERSION,
      "Pala One",
      "https://paullagier.github.io/pala-one-firmware/connected.html");

  // The library handles WiFi.begin() itself; we only persist creds once the
  // association succeeds. If the association fails the library reports the
  // error back to the browser and the saved creds stay untouched, which is
  // exactly what we want (don't poison NVS with bad creds).
  //
  // Known edge case (only when an upload session is active): the library
  // calls WiFi.mode(WIFI_STA)+begin() internally, which kills the upload
  // session's SoftAP and disconnects any phone mid-upload. Considered narrow
  // enough to defer; fix would be `setCustomConnectWiFi(...)` to bypass the
  // lib's connect while a session is up and just save creds.
  s_state.lib.onImprovConnected([](const char* ssid, const char* password) {
    WifiCreds::save(ssid, password);
    if (!s_state.uploadSession) {
      // We brought Wi-Fi up just to verify creds — schedule teardown after
      // the library finishes sending its "Provisioned" reply.
      s_state.ownsWifi        = true;
      s_state.credsReceived   = true;
      s_state.credsReceivedMs = millis();
    }
    // Upload session: leave Wi-Fi alone — upload screen owns it.
  });
}

void notifyUploadSession(bool active) {
  s_state.uploadSession = active;
  if (active) {
    // Upload takes over the radio. Cancel any pending teardown so the grace
    // timer doesn't yank Wi-Fi out from under it.
    s_state.ownsWifi = false;
    clearGraceTimer();
  }
}

bool isActive() {
  // Stay awake whenever a USB host is on the bus. Wall chargers and
  // unplugged cables register as not-plugged, so they don't pin us awake.
  return hostPresent();
}

void loop() {
  // Track bytes from host for the activity-window fallback in hostPresent().
  // Do this *before* handleSerial drains the buffer so we don't miss the
  // moment the browser starts talking.
  if (Serial.available() > 0) {
    s_state.lastHostByteMs = millis();
  }

  if (!hostPresent()) {
    // Host went away (unplug, browser closed the port, etc.). Drop anything
    // we'd been holding.
    releaseWifi();
    clearGraceTimer();
    s_state.lastHostByteMs = 0;
    return;
  }

  if (s_state.credsReceived
      && (uint32_t)(millis() - s_state.credsReceivedMs) > kShutdownGraceMs) {
    releaseWifi();
    clearGraceTimer();
    // Keep listening — host might want to re-provision a different network.
  }

  s_state.lib.handleSerial();
}

}  // namespace WifiProvisioning
