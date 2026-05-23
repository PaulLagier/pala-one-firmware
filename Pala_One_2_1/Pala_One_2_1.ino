// ============================================================================
//  Pala One — firmware entry point.
//
//  The real firmware lives under src/ (hal/, pure/, storage/, ui/, web/).
//  This file exists for two reasons:
//
//    1. Arduino IDE requires a .ino with the same name as the sketch folder.
//    2. It provides a single place for Arduino IDE users to pick the board
//       revision. PlatformIO users pick the env in platformio.ini instead
//       and can leave the BOARD_V1_x defines below alone.
//
//  Build options:
//
//    - PlatformIO (recommended):
//        pio run -e wireless-paper-v1_2 -t upload   # V1.2 panel
//        pio run -e wireless-paper-v1_1 -t upload   # V1.1 panel
//
//    - Arduino IDE 2:
//        1. Install the Heltec ESP32 board package (heltec_wifi_lora_32_V3).
//        2. Install libraries: heltec-eink-modules (todd-herbert fork),
//           Adafruit GFX, U8g2_for_Adafruit_GFX.
//        3. Uncomment exactly one of BOARD_V1_1 / BOARD_V1_2 below.
//        4. Compile and upload.
// ============================================================================

// ── Board selection: uncomment the line that matches your hardware ──────────
// #define BOARD_V1_1
// #define BOARD_V1_2
// ────────────────────────────────────────────────────────────────────────────

// ── Language selection: uncomment exactly one (Arduino IDE) ─────────────────
//   PlatformIO users pick the env in platformio.ini (-en / -es leaf envs)
//   and can leave these defines commented out. Default if nothing is set:
//   English (with a #pragma message warning from src/config.h).
#define LANG_EN
// #define LANG_ES_LA
// ────────────────────────────────────────────────────────────────────────────

// ── Web UI default theme: uncomment exactly one ─────────────────────────────
//   The web UI has a light and a dark palette and a per-page toggle button.
//   Once a visitor picks one the choice is remembered in their browser's
//   localStorage and overrides whatever's set here — this define only picks
//   the *first-visit* default. Default if nothing is set: light.
//   PlatformIO users can also pass -D WEB_THEME_DARK in build_flags.
#define WEB_THEME_LIGHT
// #define WEB_THEME_DARK
// ────────────────────────────────────────────────────────────────────────────

// When built with PlatformIO, WIRELESS_PAPER + DISPLAY_V1_x come from
// build_flags and the BOARD_V1_x macros above stay commented out. When
// built with Arduino IDE, the macros above drive the same defines so the
// rest of the firmware sees one consistent set of feature flags.
#if defined(BOARD_V1_1)
  #ifndef WIRELESS_PAPER
    #define WIRELESS_PAPER
  #endif
  #ifndef DISPLAY_V1_1
    #define DISPLAY_V1_1
  #endif
#elif defined(BOARD_V1_2)
  #ifndef WIRELESS_PAPER
    #define WIRELESS_PAPER
  #endif
  #ifndef DISPLAY_V1_2
    #define DISPLAY_V1_2
  #endif
#endif

#if !defined(DISPLAY_V1_1) && !defined(DISPLAY_V1_2)
  #error "Board not selected. Arduino IDE: uncomment BOARD_V1_1 or BOARD_V1_2 in Pala_One_2_1.ino. PlatformIO: build with -e wireless-paper-v1_1 or -e wireless-paper-v1_2."
#endif

#include <Arduino.h>
#include <esp_sleep.h>

#include "src/config.h"
#include "src/state.h"
#include "src/hal/battery.h"
#include "src/hal/display.h"
#include "src/hal/input.h"
#include "src/pure/hashing.h"
#include "src/storage/app_catalog.h"
#include "src/storage/fs_util.h"
#include "src/storage/library.h"
#include "src/storage/list_items.h"
#include "src/storage/page_cache.h"
#include "src/ui/font.h"
#include "src/ui/pala_api_impl.h"
#include "src/ui/reader.h"
#include "src/ui/screen.h"
#include "src/ui/widgets.h"  // drawCenter
#include "src/ui/screens/about_screen.h"
#include "src/ui/screens/apps_screen.h"
#include "src/ui/screens/bookmarks/book_select_screen.h"
#include "src/ui/screens/bookmarks/bookmark_list_screen.h"
#include "src/ui/screens/bookmarks/preview_screen.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screens/list_screen.h"
#include "src/ui/screens/reader_screen.h"
#include "src/ui/screens/upload_screen.h"
#include "src/ui/screensavers.h"
#include "src/ui/sleep.h"
#include "src/ui/text.h"
#include "src/ui/toast.h"
#include "src/web/web.h"

// ============================================================================
//  Screen instances + current-screen pointer
// ============================================================================
LibraryScreen              g_libraryScreen;
ReaderScreen               g_readerScreen;
UploadScreen               g_uploadScreen;
AboutScreen                g_aboutScreen;
AppsScreen                 g_appsScreen;
ListScreen                 g_listScreen;
BookmarkBookSelectScreen   g_bmBookSelectScreen;
BookmarkListScreen         g_bmListScreen;
BookmarkPreviewScreen      g_bmPreviewScreen;

Screen* g_currentScreen = &g_libraryScreen;

// ============================================================================
//  Setup
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("[boot] wake cause: %d\n", esp_sleep_get_wakeup_cause());
  setCpuFrequencyMhz(240); // full speed for init; lowered to 80 MHz at end of setup

  pinMode(BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN), btnISR, CHANGE);

  u8g2.begin(gfx);

#if HAS_BATTERY
  adcSetupOnce();
  pinMode(BAT_ADC_CTRL, INPUT);
  updateBatteryCached(true);
#endif

  // Load Sleep settings early — before display.clear() — so the no-screensaver
  // flag is available to gate the full-refresh boot clear below.
  prefs.begin("ereader", false);
  Sleep::loadSettings();

  // Skip the full-refresh boot clear only when all three are true:
  //   (a) waking from deep sleep (not a fresh boot),
  //   (b) no-screensaver mode is on, and
  //   (c) we were reading a book (wake_path was set by armResumeOnWake during
  //       the sleep entry — tryRestoreReadingSession() will consume it later).
  // If the user fell asleep in a menu the screensaver was shown, so we always
  // clear normally in that case.
  bool wokeFromSleep = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0);
  bool wereReading   = (prefs.getString("wake_path", "").length() > 0);
  display.fastmodeOff();
  if (!wokeFromSleep || !Sleep::noScreensaver() || !wereReading) {
    display.clear();
  }

  if (!fsBegin()) {
    drawCenter(D_BOOT_STORAGE_ERROR, D_BOOT_TRY_FACTORY_RESET);
    return;
  }
  ensureBooksDir();

  {
    uint64_t chipId = ESP.getEfuseMac();
    snprintf(AP_SSID, sizeof(AP_SSID), "PALA-%06llX", chipId & 0xFFFFFFULL);
  }

  Font::loadSettings();
  Screensavers::loadSettings();
  loadBooks();
  loadListItems();
  loadApps();
  initPalaAPI();
  registerWebRoutes();
  markUserActivity();

  if (tryRestoreReadingSession()) {
    renderCurrentPage();      // ~300ms draw — wake-press releases during this
    resetInputFrontend();     // then discard the wake-press only
    g_currentScreen = &g_readerScreen;
  } else {
    g_currentScreen = &g_libraryScreen;
    g_libraryScreen.onEnter();
    resetInputFrontend();
  }

  // Drop to 80 MHz for normal operation — saves significant power.
  // Upload mode will raise it back to 240 MHz temporarily.
  setCpuFrequencyMhz(80);
}

// ============================================================================
//  Main loop
// ============================================================================
void loop() {
  g_btns.poll();
  maybeRecoverFromIsrOverflow();

  ButtonEvent ev = ButtonEvent::fromButtonState(g_btns);
  if (ev.any()) markUserActivity();

  if (ENABLE_DEEP_SLEEP && g_currentScreen->allowSleep()) {
    if (userIdleMs() > Sleep::idleTimeoutMs()) {
      Sleep::enter();
      return;
    }
  }

  g_currentScreen->onButton(ev);
  g_currentScreen->onIdleTick();

  // Toast just expired? Repaint so its pixels actually disappear.
  if (Toast::clearIfExpired()) g_currentScreen->draw();

  if (g_currentScreen->nextScreen) {
    g_currentScreen = g_currentScreen->nextScreen;
    g_currentScreen->nextScreen = nullptr;
    g_currentScreen->onEnter();
  }

  // Light-sleep idle gating. The single biggest battery saver while reading:
  // between page turns the loop has nothing to do, so we drop the CPU until
  // either the button is pressed or a short timer fires for housekeeping.
  // Skipped on screens that need the CPU active (UploadScreen → SoftAP), and
  // mid-click-sequence — the classifier's trailing-silence wait runs against
  // millis(), and sleeping through it would add up to one tick interval of
  // latency per emit. Cost of staying awake during a click sequence is at
  // most ~550ms (MAX_CLICK_SEQUENCE_MS); the long quiet gaps between page
  // turns are where the battery savings actually come from.
  //
  // The `buttonQueueNonEmpty()` check closes a race: the ISR can queue a
  // release edge after this iter's `poll()` ran but before we reach this
  // gate — `clickCount_` is still 0 at that instant, but the edge is
  // sitting in the ring buffer waiting to be drained. Without the check we
  // sleep through it; ext0 (level-low) doesn't fire on a release, so we'd
  // only re-process the edge on the next timer wake (~150ms later in the
  // worst case under the bound below).
  if (g_currentScreen->allowSleep()
      && !g_btns.hasPendingClicks()
      && !buttonQueueNonEmpty()) {
    Sleep::idleLightSleep(Toast::isActive());
  }
}
