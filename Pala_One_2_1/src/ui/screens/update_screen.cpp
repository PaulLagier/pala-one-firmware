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
#include "src/ui/pala_chrome.h"
#include "src/lang/join.h"
#include "src/storage/navigation_profile.h"
#include "src/ui/screens/settings_screen.h"

static constexpr uint32_t kStaTimeoutMs = 15000;
static constexpr const char *kKeyOtaChannel = "cfg_ota_channel";

// Static progress callback — called from OTA::download() during the blocking
// binary stream. Fires g_updateScreen.setProgress() which redraws at 10%.
static void onDownloadProgress(int pct) { g_updateScreen.setProgress(pct); }

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
  if (pct % 10 == 0 || pct == 100)
    draw();
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
  nextScreen = NavigationProfile::readingFocused() ? static_cast<Screen *>(&g_settingsScreen)
                                                   : static_cast<Screen *>(&g_libraryScreen);
}

// ----------------------------------------------------------------------------
//  Lifecycle
// ----------------------------------------------------------------------------
void UpdateScreen::onEnter() {
  loadChannel();
  focusItem_ = 0;
  remoteVersion_ = "";
  progress_ = 0;
  wifiStarted_ = false;

  if (!WifiCreds::has()) {
    phase_ = Phase::NoCreds;
  } else if (wifiStaBegin()) {
    wifiStarted_ = true;
    WifiProvisioning::notifyUploadSession(true);
    phase_ = Phase::Connecting;
    staStartMs_ = millis();
  } else {
    phase_ = Phase::ConnFailed;
  }
  draw();
}

void UpdateScreen::onIdleTick() {
  if (phase_ != Phase::Connecting)
    return;

  WifiStaResult r = wifiStaPoll(net_);
  if (r == WifiStaResult::Connected) {
    phase_ = Phase::Idle;
    draw();
    return;
  }
  if (r == WifiStaResult::Failed || (uint32_t)(millis() - staStartMs_) > kStaTimeoutMs) {
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
  Font::useMenu();
  if (phase_ == Phase::NoCreds || phase_ == Phase::Connecting || phase_ == Phase::ConnFailed ||
      phase_ == Phase::Checking || phase_ == Phase::Downloading || phase_ == Phase::RebootPrompt) {
    drawSectionHeader(D_UPDATE_HEADER);
    if (phase_ == Phase::NoCreds)
      PalaChrome::message(gfx, u8g2, D_UPDATE_NO_CREDS_L1, D_UPDATE_NO_CREDS_L2);
    else if (phase_ == Phase::Connecting)
      PalaChrome::message(gfx, u8g2, D_UPDATE_CONNECTING);
    else if (phase_ == Phase::ConnFailed)
      PalaChrome::message(gfx, u8g2, D_UPDATE_CONN_FAILED);
    else if (phase_ == Phase::Checking)
      PalaChrome::message(gfx, u8g2, D_UPDATE_CHECKING);
    else if (phase_ == Phase::RebootPrompt)
      PalaChrome::message(gfx, u8g2, D_UPDATE_REBOOT_MSG, D_UPDATE_REBOOT_HINT);
    else {
      PalaChrome::panel(gfx);
      PalaChrome::infoLine(u8g2, D_UPDATE_INSTALLING, 49, true);
      char percent[8];
      snprintf(percent, sizeof(percent), "%d%%", progress_);
      PalaChrome::infoLine(u8g2, percent, 66);
      PalaChrome::progress(gfx, progress_);
    }
    if (phase_ == Phase::NoCreds || phase_ == Phase::ConnFailed)
      PalaChrome::card(gfx, u8g2, 89, D_READER_BACK_LIBRARY, true);
    Font::useBody();
    display.update();
    return;
  }

  // Status occupies the common heading; every menu uses three shared rows.
  const bool hasInstall = phase_ == Phase::UpdateAvailable || phase_ == Phase::DownloadFailed;
  const int backIndex = hasInstall ? 4 : 3;
  if (focusItem_ < 0 || focusItem_ > backIndex)
    focusItem_ = 0;
  String status = Language::join({D_UPDATE_VERSION_PREFIX, FW_VERSION});
  if (phase_ == Phase::ServerFail)
    status = D_UPDATE_SERVER_FAIL;
  else if (phase_ == Phase::UpToDate)
    status = D_UPDATE_UP_TO_DATE;
  else if (phase_ == Phase::UpdateAvailable)
    status = String(D_UPDATE_AVAILABLE_PREFIX) + remoteVersion_;
  else if (phase_ == Phase::DownloadFailed)
    status = D_UPDATE_DOWNLOAD_FAILED;
  drawSectionHeader(status.c_str());
  const int first = PalaChrome::firstVisible(focusItem_, backIndex + 1);
  for (int row = 0; row < PalaChrome::visibleCards && first + row <= backIndex; ++row) {
    const int index = first + row;
    String label;
    if (index == backIndex)
      label = D_READER_BACK_LIBRARY;
    else if (index == 0)
      label = D_UPDATE_CHAN_STABLE;
    else if (index == 1)
      label = D_UPDATE_CHAN_DEV;
    else if (index == 2)
      label = D_UPDATE_BTN_CHECK;
    else
      label = D_UPDATE_BTN_INSTALL;
    if (index < 2)
      PalaChrome::menuChoice(gfx, u8g2, PalaChrome::firstCardTop + row * PalaChrome::cardStep,
                             label.c_str(), index == focusItem_,
                             index == 0 ? stableChan_ : !stableChan_);
    else
      PalaChrome::menuCard(gfx, u8g2, PalaChrome::firstCardTop + row * PalaChrome::cardStep,
                           label.c_str(), index == focusItem_);
  }

  display.update();
}

// ----------------------------------------------------------------------------
//  Input
// ----------------------------------------------------------------------------
void UpdateScreen::onButton(const ButtonEvent &e) {
  if (!e.any())
    return;

  // Reboot prompt.
  if (phase_ == Phase::RebootPrompt) {
    if ((NavigationProfile::readingFocused() &&
         (e.kind == ButtonEvent::Long || e.kind == ButtonEvent::Double)) ||
        (!NavigationProfile::readingFocused() &&
         Gestures::resolveLegacyAction(e, ButtonEvent::Double, ACTION_MENU))) {
      esp_restart();
    }
    return;
  }

  if ((NavigationProfile::readingFocused() && e.kind == ButtonEvent::Triple) ||
      (!NavigationProfile::readingFocused() &&
       Gestures::resolveLegacyAction(e, ButtonEvent::Triple, ACTION_HOME))) {
    exitToLibrary();
    return;
  }

  // Full UI phases only
  const bool fullUi = phase_ == Phase::Idle || phase_ == Phase::ServerFail ||
                      phase_ == Phase::UpToDate || phase_ == Phase::UpdateAvailable ||
                      phase_ == Phase::DownloadFailed;
  if (!fullUi) {
    if ((phase_ == Phase::NoCreds || phase_ == Phase::ConnFailed) &&
        (e.kind == ButtonEvent::Long || e.kind == ButtonEvent::Double))
      exitToLibrary();
    return;
  }

  if ((NavigationProfile::readingFocused() && e.kind == ButtonEvent::Short) ||
      (!NavigationProfile::readingFocused() &&
       Gestures::resolveLegacyAction(e, ButtonEvent::Short, ACTION_NEXT))) {
    bool hasInstall = (phase_ == Phase::UpdateAvailable || phase_ == Phase::DownloadFailed);
    focusItem_ = (focusItem_ + 1) % (hasInstall ? 5 : 4);
    draw();
    return;
  }

  if (!NavigationProfile::readingFocused() && Gestures::isNonLegacyAction(e, ACTION_PREV)) {
    bool hasInstall = (phase_ == Phase::UpdateAvailable || phase_ == Phase::DownloadFailed);
    focusItem_--;
    if (focusItem_ < 0) {
      focusItem_ = hasInstall ? 4 : 3;
    }
    draw();
    return;
  }

  if ((NavigationProfile::readingFocused() &&
       (e.kind == ButtonEvent::Long || e.kind == ButtonEvent::Double)) ||
      (!NavigationProfile::readingFocused() &&
       Gestures::resolveLegacyAction(e, ButtonEvent::Double, ACTION_MENU))) {
    const bool hasInstall = phase_ == Phase::UpdateAvailable || phase_ == Phase::DownloadFailed;
    if (focusItem_ == (hasInstall ? 4 : 3)) {
      exitToLibrary();
      return;
    }
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
          phase_ = Phase::UpdateAvailable;
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
      phase_ = Phase::Downloading;
      draw();
      OtaDownloadResult r = OTA::download(stableChan_ ? "stable" : "dev", onDownloadProgress);
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
