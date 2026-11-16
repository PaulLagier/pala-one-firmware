#include "src/storage/navigation_profile.h"
#include "src/ui/screens/upload_screen.h"

#include "src/config.h"
#include "src/hal/display.h"
#include "src/hal/input.h" // resetInputFrontend() — used on exit only
#include "src/hal/wifi.h"
#include "src/hal/companion_ble.h"
#include "src/hal/wifi_provisioning.h"
#include "src/state.h" // server
#include "src/storage/library.h"
#include "src/storage/wifi_creds.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/web/apps_upload.h" // resetAppUpload()
#include "src/web/upload.h"      // resetBookUpload() / resetSleepUpload()
#include "src/ui/reader_actions.h"

// How long to wait for an STA association before falling back to AP. Long
// enough for a typical 2.4 GHz home network (~1-3s); short enough that an
// unreachable network doesn't leave the user staring at "Connecting…".
static constexpr uint32_t kStaTimeoutMs = 5000;

// ---- Drawing --------------------------------------------------------------
static void drawConnecting(const String &ssid) {
  if (NavigationProfile::readingFocused()) {
    prepareMenuFrame();
    drawPalaList(D_UPLOAD_CONNECTING, 3, 2, [&](int i) -> String {
      return i == 0   ? ssid
             : i == 1 ? String(D_UPLOAD_HOTSPOT_HINT_L1)
                      : String(D_UPLOAD_HOTSPOT_HINT_L2);
    });
    display.update();
    return;
  }
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
  if (NavigationProfile::readingFocused()) {
    drawReadingView();
    return;
  }
  prepareMenuFrame();

  int y = drawSectionHeader(D_UPLOAD_HEADER);
#ifdef PALA_COMPANION
  if (phase_ == Phase::Choose) {
    const char *labels[] = {D_BLE_CHOICE_WIFI, D_BLE_CHOICE_APP, D_BLE_BACK};
    for (uint8_t i = 0; i < 3; ++i) {
      drawMenuRow(y, labels[i], choice_ == i);
      y += menuLineH();
    }
    display.update();
    return;
  }
  if (phase_ == Phase::StartingBluetooth || phase_ == Phase::BluetoothError) {
    Font::useToast();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(phase_ == Phase::StartingBluetooth ? D_BLE_STARTING : D_BLE_ERROR);
    if (phase_ == Phase::BluetoothError) {
      u8g2.setCursor(MARGIN_X, y + 14);
      u8g2.print(Gestures::legacyControlsOn() ? D_BLE_EXIT_LEGACY : D_BLE_EXIT_STANDARD);
    }
    display.update();
    return;
  }
  if (phase_ == Phase::Bluetooth) {
    Font::useToast();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_BLE_OPEN_APP);
    u8g2.setCursor(MARGIN_X, y + 14);
    u8g2.print(D_BLE_PIN);
    Font::useAppLarge();
    u8g2.setCursor(MARGIN_X, y + 38);
    u8g2.print(companionPairingPin());
    Font::useToast();
    u8g2.setCursor(MARGIN_X, y + 54);
    u8g2.print(companionBleConnected() ? D_BLE_CONNECTED : D_BLE_WAITING);
    u8g2.setCursor(MARGIN_X, y + 68);
    u8g2.print(Gestures::legacyControlsOn() ? D_BLE_EXIT_LEGACY : D_BLE_EXIT_STANDARD);
    display.update();
    return;
  }
#endif

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
#ifdef PALA_COMPANION
  phase_ = Phase::Choose;
  choice_ = 0; // Keep the existing Wi-Fi workflow first.
  draw();
#else
  beginSession();
#endif
}

void UploadScreen::beginBluetooth() {
  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();
  WifiProvisioning::notifyUploadSession(true);
  wifiEnd();
  setCpuFrequencyMhz(240);
  companionBleBegin();
  startedMs_ = millis();
  phase_ = companionBleRunning() ? Phase::Bluetooth : Phase::BluetoothError;
  if (phase_ == Phase::BluetoothError)
    setCpuFrequencyMhz(80);
  (void)companionBleNeedsRedraw();
  // The opening gesture was already consumed by onButton. Retain any new
  // button edges received while the radio was initializing.
  draw();
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
    phase_ = Phase::ConnectingSta;
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
  phase_ = Phase::Ready;
  view_ = 0;
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
  if (phase_ == Phase::Choose || phase_ == Phase::StartingBluetooth) {
    resetInputFrontend();
    nextScreen = &g_libraryScreen;
    return;
  }
  if (phase_ == Phase::Bluetooth || phase_ == Phase::BluetoothError)
    companionBleEnd();
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
void UploadScreen::onButton(const ButtonEvent &e) {
  if (NavigationProfile::readingFocused() && e.any()) {
    const bool open = e.kind == ButtonEvent::Long || e.kind == ButtonEvent::Double;
    if (e.kind == ButtonEvent::Triple) {
      stopSessionToLibrary();
      return;
    }
#ifdef PALA_COMPANION
    if (phase_ == Phase::Choose) {
      if (e.kind == ButtonEvent::Short) {
        choice_ = (choice_ + 1) % 3;
        draw();
      } else if (open) {
        if (choice_ == 0)
          beginSession();
        else if (choice_ == 1) {
          phase_ = Phase::StartingBluetooth;
          draw();
        } else
          stopSessionToLibrary();
      }
      return;
    }
    if (phase_ == Phase::StartingBluetooth)
      return;
    if (phase_ == Phase::Bluetooth || phase_ == Phase::BluetoothError) {
      if (open)
        stopSessionToLibrary();
      return;
    }
#endif
    if (phase_ == Phase::ConnectingSta) {
      fallbackToAp();
      return;
    }
    const int exitView = net_.fallbackUrl.length() ? 3 : 2;
    if (open && view_ == exitView) {
      stopSessionToLibrary();
      return;
    }
    if (open || e.kind == ButtonEvent::Short) {
      view_ = (view_ + 1) % (exitView + 1);
      draw();
    }
    return;
  }

  if (!e.any())
    return;

#ifdef PALA_COMPANION
  if (phase_ == Phase::Choose) {
    if (Gestures::resolveLegacyAction(e, ButtonEvent::Triple, ACTION_HOME)) {
      stopSessionToLibrary();
    } else if (Gestures::resolveLegacyAction(e, ButtonEvent::Short, ACTION_NEXT)) {
      choice_ = (choice_ + 1) % 3;
      draw();
    } else if (Gestures::isNonLegacyAction(e, ACTION_PREV)) {
      choice_ = (choice_ + 2) % 3;
      draw();
    } else if (Gestures::resolveLegacyAction(e, ButtonEvent::Double, ACTION_MENU)) {
      if (choice_ == 0)
        beginSession();
      else if (choice_ == 1) {
        phase_ = Phase::StartingBluetooth;
        draw();
      } else
        stopSessionToLibrary();
    }
    return;
  }
#endif

  // Any tap during the connecting splash means "use the hotspot instead".
  if (phase_ == Phase::ConnectingSta) {
    fallbackToAp();
    return;
  }

  if (Gestures::legacyControlsOn() &&
      (e.kind == ButtonEvent::Short || e.kind == ButtonEvent::Triple)) {
    stopSessionToLibrary();
  }

  if (!Gestures::legacyControlsOn()) {
    if (Gestures::actionFor(e.kind) == ACTION_HOME) {
      stopSessionToLibrary();
    }
  }
}

void UploadScreen::onIdleTick() {
  // The main loop ticks once after onButton and before applying nextScreen.
  // Do not start a radio after an exit/cancel requested in that same tick.
  if (nextScreen)
    return;
#ifdef PALA_COMPANION
  if (phase_ == Phase::Choose)
    return;
  if (phase_ == Phase::StartingBluetooth) {
    beginBluetooth();
    return;
  }
  if (phase_ == Phase::Bluetooth || phase_ == Phase::BluetoothError) {
    companionBleTick();
    if (companionBleNeedsRedraw())
      draw();
    if ((uint32_t)(millis() - startedMs_) > UPLOAD_AUTO_EXIT_MS)
      stopSessionToLibrary();
    return;
  }
#endif
  if (phase_ == Phase::ConnectingSta) {
    WifiStaResult r = wifiStaPoll(net_);
    if (r == WifiStaResult::Connected) {
      enterReady();
      return;
    }
    if (r == WifiStaResult::Failed || (uint32_t)(millis() - staStartedMs_) > kStaTimeoutMs) {
      fallbackToAp();
      return;
    }
    return; // still associating — server isn't up yet
  }

  server.handleClient();
  if ((uint32_t)(millis() - startedMs_) > UPLOAD_AUTO_EXIT_MS) {
    stopSessionToLibrary();
  }
}

void UploadScreen::drawReadingView() {
  prepareMenuFrame();
#ifdef PALA_COMPANION
  if (phase_ == Phase::Choose) {
    drawPalaList(D_MENU_CONNECT, 3, choice_, [](int i) -> String {
      return i == 0 ? D_BLE_CHOICE_WIFI : i == 1 ? D_BLE_CHOICE_APP : D_BLE_BACK;
    });
    display.update();
    return;
  }
  if (phase_ == Phase::Bluetooth || phase_ == Phase::BluetoothError ||
      phase_ == Phase::StartingBluetooth) {
    drawPalaList(D_BLE_CHOICE_APP, 3, 2, [&](int i) -> String {
      if (i == 2)
        return D_BLE_BACK;
      if (phase_ == Phase::BluetoothError)
        return i ? String("") : String(D_BLE_ERROR);
      if (phase_ == Phase::StartingBluetooth)
        return i ? String("") : String(D_BLE_STARTING);
      if (i == 0)
        return String(D_BLE_PIN) + " " + companionPairingPin();
      return companionBleConnected() ? D_BLE_CONNECTED : D_BLE_WAITING;
    });
    display.update();
    return;
  }
#endif
  const int exitView = net_.fallbackUrl.length() ? 3 : 2;
  drawPalaList(D_UPLOAD_HEADER, 3, 2, [&](int i) -> String {
    if (i == 2)
      return view_ == exitView ? D_READER_BACK_LIBRARY : D_UPLOAD_OPEN;
    if (view_ == exitView)
      return "";
    if (view_ == 0) {
      if (i == 0)
        return net_.mode == WifiMode::Station ? net_.staSsid : String(net_.apSsid);
      return net_.mode == WifiMode::Station ? String(D_UPLOAD_CONNECTED)
                                            : String(D_UPLOAD_PASSWORD) + " " + net_.apPass;
    }
    if (i == 0)
      return D_UPLOAD_OPEN;
    return view_ == 1 ? net_.primaryUrl : net_.fallbackUrl;
  });
  display.update();
}
