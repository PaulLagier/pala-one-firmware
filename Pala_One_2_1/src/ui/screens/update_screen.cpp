#include "src/ui/screens/update_screen.h"

#include <esp_system.h>
#include "src/config.h"
#include "src/hal/display.h"
#include "src/hal/input.h"
#include "src/hal/ota.h"
#include "src/hal/wifi.h"
#include "src/hal/wifi_provisioning.h"
#include "src/state.h"
#include "src/storage/wifi_creds.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/ui/reader_actions.h"

static constexpr uint32_t    kStaTimeoutMs  = 15000;
static constexpr const char* kKeyOtaChannel = "cfg_ota_channel";

// Static progress callback — called from OTA::download() during the blocking
// binary stream. Fires g_updateScreen.setProgress() which redraws at 10%.
static void onDownloadProgress(int pct) {
  g_updateScreen.setProgress(pct);
}

// ----------------------------------------------------------------------------
//  NVS helpers
// ----------------------------------------------------------------------------
void UpdateScreen::loadChannel() {
  stableChan_ = (prefs.getString(kKeyOtaChannel, "stable") == "stable");
}

void UpdateScreen::saveChannel() {
  prefs.putString(kKeyOtaChannel, stableChan_ ? "stable" : "dev");
}

// ----------------------------------------------------------------------------
//  Progress update (called during blocking download)
// ----------------------------------------------------------------------------
void UpdateScreen::setProgress(int pct) {
  progress_ = pct;
  if (pct % 10 == 0 || pct == 100) draw();
}

// ----------------------------------------------------------------------------
//  Exit
// ----------------------------------------------------------------------------
void UpdateScreen::exitToLibrary() {
  if (wifiStarted_) {
    wifiEnd();
    WifiProvisioning::notifyUploadSession(false);
    wifiStarted_ = false;
  }
  nextScreen = &g_libraryScreen;
}

// ----------------------------------------------------------------------------
//  Lifecycle
// ----------------------------------------------------------------------------
void UpdateScreen::onEnter() {
  loadChannel();
  focusItem_     = 0;
  remoteVersion_ = "";
  progress_      = 0;
  wifiStarted_   = false;

  if (!WifiCreds::has()) {
    phase_ = Phase::NoCreds;
  } else if (wifiStaBegin()) {
    wifiStarted_ = true;
    WifiProvisioning::notifyUploadSession(true);
    phase_      = Phase::Connecting;
    staStartMs_ = millis();
  } else {
    phase_ = Phase::ConnFailed;
  }
  draw();
}

void UpdateScreen::onIdleTick() {
  if (phase_ != Phase::Connecting) return;

  WifiStaResult r = wifiStaPoll(net_);
  if (r == WifiStaResult::Connected) {
    phase_ = Phase::Idle;
    draw();
    return;
  }
  if (r == WifiStaResult::Failed ||
      (uint32_t)(millis() - staStartMs_) > kStaTimeoutMs) {
    wifiEnd();
    WifiProvisioning::notifyUploadSession(false);
    wifiStarted_ = false;
    phase_ = Phase::ConnFailed;
    draw();
  }
}

// ----------------------------------------------------------------------------
//  Drawing
// ----------------------------------------------------------------------------
void UpdateScreen::draw() {
  prepareMenuFrame();
  Font::useBody();
  int y = drawSectionHeader(D_UPDATE_HEADER);

  // ---- Transient / single-message states -----------------------------------

  if (phase_ == Phase::NoCreds) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_NO_CREDS_L1);
    y += 14;
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_NO_CREDS_L2);
    display.update();
    return;
  }
  if (phase_ == Phase::Connecting) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_CONNECTING);
    display.update();
    return;
  }
  if (phase_ == Phase::ConnFailed) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_CONN_FAILED);
    display.update();
    return;
  }
  if (phase_ == Phase::Checking) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_CHECKING);
    display.update();
    return;
  }
  if (phase_ == Phase::Downloading) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_INSTALLING);
    y += 16;
    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", progress_);
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(pctBuf);
    display.update();
    return;
  }
  if (phase_ == Phase::RebootPrompt) {
    Font::useBold();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_REBOOT_MSG);
    y += 18;
    Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_REBOOT_HINT);
    display.update();
    return;
  }

  // ---- Full UI (Idle, ServerFail, UpToDate, UpdateAvailable, DownloadFailed)

  bool hasInstall = (phase_ == Phase::UpdateAvailable ||
                     phase_ == Phase::DownloadFailed);
  int maxItems = hasInstall ? 4 : 3;
  if (focusItem_ >= maxItems) focusItem_ = 0;

  // Version line
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPDATE_VERSION_PREFIX FW_VERSION);
  y += 16;

  // Channel label on its own line
  Font::useBody();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPDATE_CHANNEL_LABEL);
  y += 14;

  // Channel checkboxes
  int cx = MARGIN_X;

  Font::useBody();
  u8g2.setCursor(cx, y);
  u8g2.print(stableChan_ ? "[x] " : "[ ] ");
  cx += u8g2.getUTF8Width("[x] ");
  if (focusItem_ == 0) Font::useBold(); else Font::useBody();
  u8g2.setCursor(cx, y);
  u8g2.print(D_UPDATE_CHAN_STABLE);
  cx += u8g2.getUTF8Width(D_UPDATE_CHAN_STABLE);
  Font::useBody();
  cx += u8g2.getUTF8Width("   ");

  u8g2.setCursor(cx, y);
  u8g2.print(!stableChan_ ? "[x] " : "[ ] ");
  cx += u8g2.getUTF8Width("[x] ");
  if (focusItem_ == 1) Font::useBold(); else Font::useBody();
  u8g2.setCursor(cx, y);
  u8g2.print(D_UPDATE_CHAN_DEV);
  y += 14;


  // Check button
  if (focusItem_ == 2) Font::useBold(); else Font::useBody();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPDATE_BTN_CHECK);
  y += 20;

  // Status line — shown below the check button
  Font::useBody();
  if (phase_ == Phase::ServerFail) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_SERVER_FAIL);
    y += 14;
  } else if (phase_ == Phase::UpToDate) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_UP_TO_DATE);
    y += 14;
  } else if (phase_ == Phase::UpdateAvailable) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_AVAILABLE_PREFIX);
    u8g2.print(remoteVersion_.c_str());
    y += 14;
  } else if (phase_ == Phase::DownloadFailed) {
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_DOWNLOAD_FAILED);
    y += 14;
  }

  // Install button — shown when update is available or after failed download
  if (hasInstall) {
    if (focusItem_ == 3) Font::useBold(); else Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(D_UPDATE_BTN_INSTALL);
  }

  display.update();
}

// ----------------------------------------------------------------------------
//  Input
// ----------------------------------------------------------------------------
void UpdateScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;

  // Reboot prompt.
  if (phase_ == Phase::RebootPrompt) {
    if (Gestures::resolveLegacyAction(e, ButtonEvent::Double, ACTION_MENU)) {
      esp_restart();
    }
    return;
  }

  if (Gestures::resolveLegacyAction(e, ButtonEvent::Triple, ACTION_HOME)) {
    exitToLibrary();
    return;
  }

  // Full UI phases only
  const bool fullUi = phase_ == Phase::Idle        ||
                      phase_ == Phase::ServerFail   ||
                      phase_ == Phase::UpToDate     ||
                      phase_ == Phase::UpdateAvailable ||
                      phase_ == Phase::DownloadFailed;
  if (!fullUi) return;

  if (Gestures::resolveLegacyAction(e, ButtonEvent::Short, ACTION_NEXT))
  {
    bool hasInstall = (phase_ == Phase::UpdateAvailable ||
                       phase_ == Phase::DownloadFailed);
    focusItem_ = (focusItem_ + 1) % (hasInstall ? 4 : 3);
    draw();
    return;
  }

  if (Gestures::isNonLegacyAction(e, ACTION_PREV))
  {
    bool hasInstall = (phase_ == Phase::UpdateAvailable ||
                       phase_ == Phase::DownloadFailed);
    focusItem_--;
    if (focusItem_ < 0) {
      focusItem_ = hasInstall ? 3 : 2;
    }
    draw();
    return;
  }

  if (Gestures::resolveLegacyAction(e, ButtonEvent::Double, ACTION_MENU))
  {
    if (focusItem_ == 0) {
      stableChan_ = true;
      saveChannel();
      draw();

    } else if (focusItem_ == 1) {
      stableChan_ = false;
      saveChannel();
      draw();

    } else if (focusItem_ == 2) {
      // Check: probe + manifest fetch
      phase_ = Phase::Checking;
      draw();
      if (OTA::isUpdateServerReachable()) {
        OtaCheckResult r = OTA::checkAvailable(stableChan_ ? "stable" : "dev");
        if (r.updateAvailable) {
          remoteVersion_ = r.remoteVersion;
          phase_         = Phase::UpdateAvailable;
        } else if (r.remoteVersion.length() > 0) {
          phase_ = Phase::UpToDate;
        } else {
          phase_ = Phase::ServerFail;
        }
      } else {
        phase_ = Phase::ServerFail;
      }
      clearButtonQueue();
      draw();

    } else if (focusItem_ == 3) {
      // Install: download + flash
      progress_ = 0;
      phase_    = Phase::Downloading;
      draw();
      OtaDownloadResult r = OTA::download(stableChan_ ? "stable" : "dev",
                                          onDownloadProgress);
      clearButtonQueue();
      if (r.success) {
        // Flash complete — tear down WiFi and drop CPU to 80 MHz before
        // showing the reboot prompt. Sleep is now allowed again.
        if (wifiStarted_) {
          wifiEnd();
          WifiProvisioning::notifyUploadSession(false);
          wifiStarted_ = false;
        }
        phase_ = Phase::RebootPrompt;
      } else {
        phase_ = Phase::DownloadFailed;
      }
      draw();
    }
    return;
  }
}
