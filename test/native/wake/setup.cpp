#include "src/hal/input.h"
#include <string>
#include "src/storage/click_timings.h"
struct InputTestPrefs {
  bool wakePath = false;
  bool begin(const char *, bool) { return true; }
  std::string getString(const char *, const char *) { return wakePath ? "/books/test.txt" : ""; }
};
#include <cassert>
#include <cstdio>
#include <initializer_list>
uint64_t testClockMs = 0;
int testPin = HIGH;
InputTestPrefs prefs;
constexpr int INPUT = 0, INPUT_PULLUP = 1, CHANGE = 2, ESP_SLEEP_WAKEUP_EXT0 = 2;
#if defined(DISPLAY_V1_2)
constexpr bool fastPanel = true;
#else
constexpr bool fastPanel = false;
#endif
int wakeCause = 0, whiteClears = 0, frames = 0;
bool locked = false, restored = false, fsReady = true, injectClicks = false, noScreensaver = false;
bool readerForced = false, menuForced = false, modeFull = false, s_storageRecoveryActive = false;
uint32_t beforeDraw = 0;
int esp_sleep_get_wakeup_cause() { return wakeCause; }
bool setCpuFrequencyMhz(int) { return true; }
void pinMode(int, int) {}
int digitalPinToInterrupt(int pin) { return pin; }
void attachInterrupt(int, void (*)(), int) {}
namespace WifiProvisioning {
void beginLocalSetup() {}
} // namespace WifiProvisioning
struct SerialMock {
  void begin(int) {}
  template <class... T> void printf(const char *, T...) {};
} Serial;
struct FontMock {
  void begin(int) {}
} u8g2;
int gfx = 0;
void adcSetupOnce() {}
void updateBatteryCached(bool) { delay(34); }
namespace Language {
void loadSettings() {}
} // namespace Language
namespace Lock {
void loadSettings() {}
bool isLocked() { return locked; }
} // namespace Lock
namespace Gestures {
void loadSettings() {}
} // namespace Gestures
namespace Sleep {
void loadSettings() {}
bool noScreensaver() { return ::noScreensaver; }
} // namespace Sleep
void loadDisplaySettings() {}
void forceNextRenderFull() { readerForced = true; }
void forceNextMenuFrameFull() { menuForced = true; }
void setDisplayRefreshMode(bool full) { modeFull = full; }
struct DisplayMock {
  void fastmodeOff() { modeFull = true; }
  void clear() {
    assert(modeFull);
    ++whiteClears;
    delay(400);
  }
} display;
void frame(bool reader) {
  beforeDraw = millis();
  if (wakeCause == ESP_SLEEP_WAKEUP_EXT0)
    assert(reader ? (readerForced || noScreensaver || whiteClears) : menuForced);
  ++frames;
  const auto finish = testClockMs + 500;
  if (injectClicks) {
    testClockMs = 90;
    testPin = HIGH;
    injectButtonEdgeNow(false);
    testClockMs = 180;
    testPin = LOW;
    injectButtonEdgeNow(true);
    testClockMs = 230;
    testPin = HIGH;
    injectButtonEdgeNow(false);
  }
  testClockMs = finish;
}
struct ScreenMock {
  bool reader = false, recovery = false;
  void onEnter() {
    if (!recovery || !locked)
      frame(reader);
  }
};
ScreenMock g_libraryScreen, g_readerScreen{true}, g_storageRecoveryScreen{false, true};
ScreenMock *g_currentScreen = nullptr;
void renderCurrentPage() { frame(true); }
bool tryRestoreReadingSession() { return restored; }
bool fsBegin() { return fsReady; }
struct RecoveryPending {};
void runStorageRecovery() {
  s_storageRecoveryActive = true;
  g_currentScreen = &g_storageRecoveryScreen;
  g_currentScreen->onEnter();
  throw RecoveryPending{};
}
#include "wake-setup.inc"
int main() {
  int cases = 0;
  ClickTimings::resetToDefaults();
  for (bool wake : {false, true})
    for (bool read : {false, true})
      for (bool lock : {false, true})
        for (bool keep : {false, true}) {
          clearButtonQueue();
          g_btns.resetState();
          testClockMs = 0;
          testPin = wake ? LOW : HIGH;
          wakeCause = wake ? 2 : 0;
          locked = lock;
          restored = read;
          fsReady = true;
          noScreensaver = keep;
          prefs.wakePath = read;
          frames = whiteClears = 0;
          readerForced = menuForced = modeFull = false;
          injectClicks = wake && !lock;
          generatedSetup();
          assert(whiteClears == (wake ? (fastPanel || lock || (keep && read) ? 0 : 1) : 1));
          assert(frames == (lock ? 0 : 1));
          assert(g_currentScreen == (read ? &g_readerScreen : &g_libraryScreen));
          if (wake && read)
            assert(readerForced == (fastPanel && !keep));
          if (wake && !lock) {
            if (fastPanel)
              assert(beforeDraw < 200);
            g_btns.poll();
            assert(ButtonEvent::fromButtonState(g_btns).kind == ButtonEvent::Short &&
                   g_btns.peekPressCount() == 1);
          }
          if (wake && lock) {
            testClockMs = 900;
            testPin = HIGH;
            injectButtonEdgeNow(false);
            g_btns.poll();
            assert(ButtonEvent::fromButtonState(g_btns).kind == ButtonEvent::Long);
          }
          ++cases;
        }
  clearButtonQueue();
  g_btns.resetState();
  testClockMs = 0;
  testPin = LOW;
  wakeCause = 2;
  locked = false;
  fsReady = false;
  injectClicks = true;
  frames = whiteClears = 0;
  s_storageRecoveryActive = false;
  try {
    generatedSetup();
  } catch (const RecoveryPending &) {
  }
  assert(s_storageRecoveryActive && g_currentScreen == &g_storageRecoveryScreen &&
         whiteClears == (fastPanel || prefs.wakePath ? 0 : 1) && frames == 1);
  g_btns.poll();
  assert(ButtonEvent::fromButtonState(g_btns).kind == ButtonEvent::Short);
  ++cases;
  printf("PASS %d actual-sketch wake cases: cold/ext0, locked/unlocked, Library/reader, one clean "
         "image, retained first click, preserved unlock hold and storage-error path\n",
         cases);
}
