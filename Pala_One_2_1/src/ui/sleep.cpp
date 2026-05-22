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
#include "src/ui/pala_one_sleep_black_icon_v4.h"
#include "src/ui/screen.h"

namespace Sleep {

// Owned setting + NVS key — file-private.
static int s_idleSecs = 120;
static constexpr const char* kKeyIdleSecs = "cfg_sleep";

void loadSettings() {
  int s = prefs.getInt(kKeyIdleSecs, 120);
  if (s < 10)   s = 10;
  if (s > 3600) s = 3600;
  s_idleSecs = s;
}

void setIdleTimeout(int secs) {
  if (secs < 10)   secs = 10;
  if (secs > 3600) secs = 3600;
  s_idleSecs = secs;
  prefs.putInt(kKeyIdleSecs, s_idleSecs);
}

int      idleTimeoutSecs() { return s_idleSecs; }
uint32_t idleTimeoutMs()   { return (uint32_t)s_idleSecs * 1000UL; }

// Render the sleep image onto the e-ink before powering down. Tries the
// user-uploaded /sleep.bin first; falls back to the built-in icon.
static void drawSleepScreen() {
  display.fastmodeOff();
  beginPageCanvas();

  File sf = FS.open("/sleep.bin", "r");
  if (sf && sf.size() >= 3904) {
    static uint8_t sleepBuf[3904];
    sf.read(sleepBuf, 3904);
    sf.close();
    gfx.fillScreen(1);
    gfx.drawXBitmap(0, 0, sleepBuf, SCREEN_W, SCREEN_H, 0);
  } else {
    if (sf) sf.close();
    gfx.fillScreen(1);
    gfx.drawXBitmap(0, 0, pala_one_sleep_black_icon_v4_bits, SCREEN_W, SCREEN_H, 0);
  }
  display.update();
}

void enter() {
  if (!ENABLE_DEEP_SLEEP) return;

  // Each screen owns its own sleep cleanup: ReaderScreen + BookmarkPreview
  // close the open book; everything else (library, list, etc.) holds no
  // book under the strong invariant and just clears wake state.
  if (g_currentScreen) g_currentScreen->onSleep();

  delay(50);

  drawSleepScreen();
  delay(600);

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
