#include "src/hal/wifi_provisioning.h"
#include "src/ui/screens/storage_recovery_screen.h"
#include "src/hal/display.h"
#include "src/hal/input.h"
#include "src/storage/click_timings.h"
#include "src/ui/font.h"
#include "src/ui/lock.h"
#include "src/ui/widgets.h"

namespace {
void line(int y, const char *text) {
  Font::useToast();
  u8g2.setCursor(MARGIN_X + 4, y);
  u8g2.print(text);
}
void choice(int y, const char *text, bool selected) {
  if (selected)
    gfx.drawRect(MARGIN_X, y - 11, SCREEN_W - 2 * MARGIN_X, 15, 1);
  line(y, text);
}
} // namespace

StorageRecoveryScreen::StorageRecoveryScreen()
    : actions_{fsRecoveryRetry, fsRecoveryInitializeConfirmed} {}

void StorageRecoveryScreen::onEnter() {
  nextScreen = nullptr;
  ready_ = false;
  transition(Page::Menu);
}

void StorageRecoveryScreen::transition(Page page, int selected) {
  page_ = page;
  selected_ = selected;
  awaitRelease_ = true;
  releaseSeen_ = false;
  forceNextMenuFrameFull();
  draw();
  // Ignore input queued before the warning was fully visible. A press held
  // through a transition must be released before any next action is accepted.
  resetInputFrontend();
}

void StorageRecoveryScreen::onIdleTick() {
  if (!awaitRelease_)
    return;
  // The classifier is reset even if resetInputFrontend's release wait times
  // out. Require a physically released pin for a whole debounce interval.
  if (digitalRead(BTN) != HIGH) {
    releaseSeen_ = false;
    return;
  }
  const uint32_t now = millis();
  if (!releaseSeen_) {
    releaseSeen_ = true;
    releaseSinceMs_ = now;
  } else if (static_cast<uint32_t>(now - releaseSinceMs_) >= ClickTimings::debounceMs()) {
    awaitRelease_ = false;
  }
}

void StorageRecoveryScreen::working(const char *message) {
  forceNextMenuFrameFull();
  prepareMenuFrame();
  drawSectionHeader(D_BOOT_STORAGE_ERROR, false, false);
  line(65, message);
  display.update();
}

void StorageRecoveryScreen::draw() {
  prepareMenuFrame();
  // Recovery must remain readable even with a saved large body font. Use the
  // fixed accent-capable toast face for warnings/actions; no status tray needs boot init.
  drawSectionHeader(D_BOOT_STORAGE_ERROR, false, false);
  if (Lock::isLocked()) {
    line(54, D_RECOVERY_LOCKED);
    line(74, D_RECOVERY_UNLOCK);
  } else if (page_ == Page::Menu) {
    choice(52, D_RECOVERY_RETRY, selected_ == 0);
    choice(72, D_RECOVERY_HELP, selected_ == 1);
    choice(92, D_RECOVERY_ERASE, selected_ == 2);
    line(113, D_RECOVERY_KEYS);
  } else if (page_ == Page::Help) {
    line(48, D_RECOVERY_PRESERVED);
    line(64, D_RECOVERY_REFLASH);
    line(80, D_RECOVERY_BACKUP);
    line(108, D_RECOVERY_BACK);
  } else if (page_ == Page::Confirm || page_ == Page::Final) {
    line(45, D_RECOVERY_DELETE_FILES);
    line(61, page_ == Page::Confirm ? D_RECOVERY_KEEP_SETTINGS : D_RECOVERY_HOLD);
    choice(83, D_RECOVERY_CANCEL, selected_ == 0);
    choice(103, page_ == Page::Confirm ? D_RECOVERY_CONTINUE : D_RECOVERY_ERASE, selected_ == 1);
  } else {
    const char *reason = D_RECOVERY_RETRY_FAILED;
    if (page_ == Page::InitializeFailed) {
      if (failure_ == FsRecoveryResult::FormatFailed)
        reason = D_RECOVERY_FORMAT_FAILED;
      else if (failure_ == FsRecoveryResult::MountFailed)
        reason = D_RECOVERY_MOUNT_FAILED;
      else
        reason = D_RECOVERY_DIRECTORY_FAILED;
    }
    line(52, reason);
    line(76, D_RECOVERY_REFLASH);
    line(108, D_RECOVERY_BACK);
  }
  display.update();
}

void StorageRecoveryScreen::onButton(const ButtonEvent &event) {
  if (!event.any() || ready_ || awaitRelease_)
    return;
  if (Lock::isLocked()) {
    if (Lock::isUnlockGesture(event)) {
      Lock::disengage();
      transition(Page::Menu);
    }
    return;
  }
  if (event.kind == ButtonEvent::Triple) {
    transition(Page::Menu);
    return;
  }
  if (event.kind == ButtonEvent::Short) {
    if (page_ == Page::Menu)
      selected_ = (selected_ + 1) % 3;
    else if (page_ == Page::Confirm || page_ == Page::Final)
      selected_ = 1 - selected_;
    else
      return;
    transition(page_, selected_);
    return;
  }
  const bool extendedFinalHold =
      page_ == Page::Final && selected_ == 1 && event.kind == ButtonEvent::VeryLong;
  if (event.kind != ButtonEvent::Double && event.kind != ButtonEvent::Long && !extendedFinalHold)
    return;
  if (page_ == Page::Menu) {
    if (selected_ == 0) {
      working(D_RECOVERY_CHECKING);
      if (actions_.retry())
        ready_ = true;
      else
        transition(Page::RetryFailed);
    } else if (selected_ == 1)
      transition(Page::Help);
    else
      transition(Page::Confirm); // Cancel is always selected first.
  } else if (page_ == Page::Help || page_ == Page::RetryFailed || page_ == Page::InitializeFailed) {
    transition(Page::Menu);
  } else if (selected_ == 0) {
    transition(Page::Menu);
  } else if (page_ == Page::Confirm) {
    transition(Page::Final); // Another independent Cancel default.
  } else if (event.kind == ButtonEvent::Long || extendedFinalHold) {
    working(D_RECOVERY_WORKING);
    failure_ = actions_.initializeConfirmed();
    if (failure_ == FsRecoveryResult::Ready)
      ready_ = true;
    else
      transition(Page::InitializeFailed);
  }
}

void runStorageRecovery() {
  WifiProvisioning::beginLocalSetup();
  Font::loadSettings();
  ClickTimings::loadSettings();
  setCpuFrequencyMhz(80);
  StorageRecoveryScreen screen;
  screen.onEnter();
  while (!screen.ready()) {
    WifiProvisioning::loopLocalSetup();
    g_btns.poll();
    maybeRecoverFromIsrOverflow();
    screen.onButton(ButtonEvent::fromButtonState(g_btns));
    screen.onIdleTick();
    delay(10);
  }
  resetInputFrontend();
  setCpuFrequencyMhz(240);
}
