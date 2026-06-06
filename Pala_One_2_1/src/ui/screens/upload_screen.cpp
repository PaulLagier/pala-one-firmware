#include "src/ui/screens/upload_screen.h"

#include "src/config.h"
#include "src/hal/display.h"
#include "src/hal/input.h"        // resetInputFrontend() — used on exit only
#include "src/hal/wifi.h"
#include "src/hal/wifi_provisioning.h"
#include "src/state.h"            // server
#include "src/storage/library.h"
#include "src/storage/wifi_creds.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/web/apps_upload.h"  // resetAppUpload()
#include "src/web/upload.h"       // resetBookUpload() / resetSleepUpload()

// How long to wait for an STA association before falling back to AP. Long
// enough for a typical 2.4 GHz home network (~1-3s); short enough that an
// unreachable network doesn't leave the user staring at "Connecting…".
static constexpr uint32_t kStaTimeoutMs = 5000;

// ---- Drawing --------------------------------------------------------------
static void drawConnecting(const String& ssid) {
  prepareMenuFrame();
  int y = drawSectionHeader(D_UPLOAD_HEADER);

  Font::useBold();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPLOAD_CONNECTING);
  y += 14;

  Font::useBody();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(ssid.c_str());
  y += 18;

  Font::useBody();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPLOAD_HOTSPOT_HINT_L1);
  y += 14;
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPLOAD_HOTSPOT_HINT_L2);

  display.update();
}

void UploadScreen::draw() {
  prepareMenuFrame();

  int y = drawSectionHeader(D_UPLOAD_HEADER);

  if (net_.mode == WifiMode::Station) {
    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_CONNECTED);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(net_.staSsid.c_str());
    y += 18;

    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_OPEN);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(net_.primaryUrl.c_str());
    y += 14;

    if (net_.fallbackUrl.length() > 0) {
      u8g2.setCursor(MARGIN_X, y);
      u8g2.print(net_.fallbackUrl.c_str());
    }
  } else {
    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_WIFI);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(net_.apSsid);
    y += 16;

    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_PASSWORD);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(net_.apPass);
    y += 16;

    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPLOAD_OPEN);
    y += 14;

    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(net_.primaryUrl.c_str());
  }

  display.update();
}

// ---- Lifecycle ------------------------------------------------------------
void UploadScreen::onEnter() {
  beginSession();
}

void UploadScreen::beginSession() {
  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  // Tell WifiProvisioning to keep its hands off the radio for the duration
  // of the session — set BEFORE wifiStaBegin so a same-tick
  // WifiProvisioning::loop() can't race the WiFi.begin() we're about to fire.
  WifiProvisioning::notifyUploadSession(true);

  if (wifiStaBegin()) {
    phase_        = Phase::ConnectingSta;
    staStartedMs_ = millis();
    drawConnecting(WifiCreds::ssid());
  } else {
    // No stored creds — straight to AP, no point showing a splash for a
    // path we know will time out.
    net_ = wifiBeginAccessPoint();
    enterReady();
  }
}

void UploadScreen::enterReady() {
  phase_     = Phase::Ready;
  startedMs_ = millis();
  server.begin();
  draw();
}

void UploadScreen::fallbackToAp() {
  wifiStaAbort();
  net_ = wifiBeginAccessPoint();
  enterReady();
}

void UploadScreen::stopSessionToLibrary() {
  server.stop();
  wifiEnd();
  WifiProvisioning::notifyUploadSession(false);

  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  loadBooks();
  // Discard the exit-click so it doesn't leak into the library screen as a
  // menu selection.
  resetInputFrontend();
  nextScreen = &g_libraryScreen;
}

// ---- Per-iteration input + tick -------------------------------------------
void UploadScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;

  // Any tap during the connecting splash means "use the hotspot instead".
  if (phase_ == Phase::ConnectingSta) {
    fallbackToAp();
    return;
  }

  if (e.kind == ButtonEvent::Short || e.kind == ButtonEvent::Triple) {
    stopSessionToLibrary();
  }
}

void UploadScreen::onIdleTick() {
  if (phase_ == Phase::ConnectingSta) {
    WifiStaResult r = wifiStaPoll(net_);
    if (r == WifiStaResult::Connected) {
      enterReady();
      return;
    }
    if (r == WifiStaResult::Failed ||
        (uint32_t)(millis() - staStartedMs_) > kStaTimeoutMs) {
      fallbackToAp();
      return;
    }
    return;   // still associating — server isn't up yet
  }

  server.handleClient();
  if ((uint32_t)(millis() - startedMs_) > UPLOAD_AUTO_EXIT_MS) {
    stopSessionToLibrary();
  }
}
