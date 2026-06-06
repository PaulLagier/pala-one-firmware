#include "src/ui/sleep.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

#include "src/config.h"
#include "src/state.h"
#include "src/hal/display.h"
#include "src/hal/input.h"          // injectButtonEdgeNow, markUserActivity
#include "src/storage/statistics.h" // Statistics::flushToNvs
#include "src/ui/font.h"            // Font::useToast / Font::useBody
#include "src/ui/lock.h"            // Lock::isLocked — gates the lock badge
#include "src/ui/pala_one_sleep_black_icon_v4.h"
#include "src/ui/screen.h"
#include "src/ui/screensavers.h"   // multi-slot rotation

namespace Sleep {

// Owned settings + NVS keys — file-private.
static int  s_idleSecs      = 120;
static bool s_noScreensaver = false;
static bool s_lockOnSleep   = false;
static constexpr const char* kKeyIdleSecs      = "cfg_sleep";
static constexpr const char* kKeyNoScreensaver = "cfg_noscr";
static constexpr const char* kKeyLockOnSleep   = "cfg_lck_slp";

void loadSettings() {
  int s = prefs.getInt(kKeyIdleSecs, 120);
  if (s < 10)   s = 10;
  if (s > 3600) s = 3600;
  s_idleSecs = s;
  s_noScreensaver = prefs.getBool(kKeyNoScreensaver, false);
  s_lockOnSleep   = prefs.getBool(kKeyLockOnSleep,   false);
}

void setIdleTimeout(int secs) {
  if (secs < 10)   secs = 10;
  if (secs > 3600) secs = 3600;
  s_idleSecs = secs;
  prefs.putInt(kKeyIdleSecs, s_idleSecs);
}

int      idleTimeoutSecs() { return s_idleSecs; }
uint32_t idleTimeoutMs()   { return (uint32_t)s_idleSecs * 1000UL; }
bool     noScreensaver()   { return s_noScreensaver; }
bool     lockOnSleep()     { return s_lockOnSleep; }

void setNoScreensaver(bool val) {
  s_noScreensaver = val;
  prefs.putBool(kKeyNoScreensaver, val);
}

void setLockOnSleep(bool val) {
  s_lockOnSleep = val;
  prefs.putBool(kKeyLockOnSleep, val);
}

// Draw a padlock icon in the top-right corner so a user who has locked the
// device and walked away can tell at a glance why it's not responding.
// White-on-black pill, icon only (no text label) — stays readable over both
// the built-in screensaver and any user-uploaded image.
static void drawLockBadge() {
  const int iconW = 7;   // padlock body width (cols 0–6)
  const int iconH = 9;   // total height: 4 rows shackle + 5 rows body
  const int padX  = 4;
  const int padY  = 2;
  const int boxW  = padX + iconW + padX;   // 15 px
  const int boxH  = padY + iconH + padY;   // 13 px
  const int boxX  = SCREEN_W - boxW - 2;
  const int boxY  = 2;

  // Pill: black fill, white 1px border bleed for hairline contrast against
  // dark uploaded images. Order matters — inner fill draws last.
  gfx.fillRect(boxX - 1, boxY - 1, boxW + 2, boxH + 2, 0);
  gfx.fillRect(boxX, boxY, boxW, boxH, 1);

  // Padlock glyph, white-on-black.
  const int iconX = boxX + padX;
  const int iconY = boxY + padY;
  const int bodyY = iconY + 4;
  // Body: solid white 7×5 rectangle.
  gfx.fillRect(iconX, bodyY, iconW, 5, 0);
  // Keyhole: one black pixel at the body centre.
  gfx.drawPixel(iconX + 3, bodyY + 2, 1);
  // Shackle: U-shape — left/right verticals (4 px tall) + 3-px top connector.
  gfx.drawFastVLine(iconX + 1, iconY, 4, 0);
  gfx.drawFastVLine(iconX + 5, iconY, 4, 0);
  gfx.drawPixel(iconX + 2, iconY, 0);
  gfx.drawPixel(iconX + 3, iconY, 0);
  gfx.drawPixel(iconX + 4, iconY, 0);
}

// Render the screensaver onto the e-ink before powering down. Falls back to
// the built-in icon if no user-uploaded screensavers are available.
static void drawSleepScreen() {
  display.fastmodeOff();
  beginPageCanvas();

  if (!Screensavers::drawNext()) {
    gfx.fillScreen(1);
    gfx.drawXBitmap(0, 0, pala_one_sleep_black_icon_v4_bits, SCREEN_W, SCREEN_H, 0);
  }
  if (Lock::isLocked()) drawLockBadge();
  display.update();
}

void enter() {
  if (!ENABLE_DEEP_SLEEP) return;

  // Each screen owns its own sleep cleanup: ReaderScreen + BookmarkPreview
  // close the open book; everything else (library, list, etc.) holds no
  // book under the strong invariant and just clears wake state.
  if (g_currentScreen) g_currentScreen->onSleep();

  delay(50);

  // No-screensaver mode only applies when coming from the reader. If the user
  // fell asleep in the library or a menu, show the screensaver as normal.
  // ReaderScreen::onSleep() has already called armResumeOnWake() above, which
  // writes wake_path to NVS — so checking it here tells us reliably whether
  // we were reading without coupling sleep.cpp to any screen type.
  bool wasReading = (prefs.getString("wake_path", "").length() > 0);

  // Engage the lock BEFORE drawing so the badge gates (Lock::isLocked()) in
  // both branches below see the new state. If we flipped it after the draw,
  // the framebuffer flushed to the panel would render unlocked and the badge
  // wouldn't appear until the next refresh (e.g. a button press on wake).
  if (s_lockOnSleep) Lock::engage();

  if (s_noScreensaver && wasReading) {
    // Full refresh of the last page so it sits cleanly on the panel.
    // The reader's framebuffer content is still intact at this point.
    // Draw the lock badge on top of that framebuffer when locked — the badge
    // is skipped by this branch (which bypasses drawSleepScreen entirely), so
    // we inject it here before the final display.update() flush.
    display.fastmodeOff();
    if (Lock::isLocked()) drawLockBadge();
    display.update();
    delay(600);
  } else {
    drawSleepScreen();
    delay(600);
  }

  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  esp_wifi_stop();
  btStop();

  Platform::prepareToSleep();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  // INPUT_PULLUP is in the digital IO domain, which powers down in deep sleep.
  // Route BTN to the RTC IO mux and hold the RTC-domain pull-up so the pin
  // doesn't float and spuriously trip ext0. rtc_gpio_init must come before
  // pullup_en or the pull bits land on the inactive (digital) mux.
  rtc_gpio_init((gpio_num_t)BTN);
  rtc_gpio_set_direction((gpio_num_t)BTN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_dis((gpio_num_t)BTN);
  rtc_gpio_pullup_en((gpio_num_t)BTN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN, 0);

  // Drain any RTC-RAM lifetime-counter deltas to NVS before power-down.
  Statistics::flushToNvs();

  delay(50);
  Serial.printf("[sleep] BTN=%d entering deep sleep\n", digitalRead(BTN));
  Serial.flush();
  esp_deep_sleep_start();
}

void idleLightSleep(bool tightTick) {
  // Don't sleep if the button is already pressed — would be a level-triggered
  // immediate wake, burning a sleep/wake cycle for no reason and skewing
  // the click classifier's edge timing.
  if (digitalRead(BTN) == LOW) return;

  // Tight: in a state that needs frequent polling (active toast). Loose: just
  // a heartbeat — button presses wake us instantly via ext0 regardless of
  // this interval. 150ms loose caps the worst-case fallback latency if a
  // release somehow lands in the tiny residual race window between the
  // main-loop sleep gate and `esp_light_sleep_start()`; battery cost vs. a
  // longer interval is negligible (a few extra wake-poll-sleep cycles per
  // second of true idle).
  const uint64_t timerUs = tightTick ? 50ULL * 1000 : 150ULL * 1000;

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN, 0);   // wake on button-low
  esp_sleep_enable_timer_wakeup(timerUs);
  esp_light_sleep_start();                            // BLOCKS until wake
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  // If GPIO woke us, the press edge wasn't seen by the normal CHANGE-edge
  // ISR (it was masked by the sleep gate). Inject a synthetic press so the
  // classifier counts it. The release edge will be picked up normally as
  // the user lifts off.
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    if (digitalRead(BTN) == LOW) injectButtonEdgeNow(true);
    markUserActivity();
  }
}

}  // namespace Sleep
