#include "src/hal/input.h"
#include "src/storage/click_timings.h"
#include <cassert>
#include <cstdio>
uint64_t testClockMs = 1000;
int testPin = HIGH;
unsigned sleepCalls = 0, lightCalls = 0, drawCalls = 0;
bool locked = false, usbActive = false, injectAfterPoll = false, rawPressAfterPoll = false;
ButtonEvent::Kind unlockKind = ButtonEvent::Double, lastEvent = ButtonEvent::None;
struct TestScreen {
  TestScreen *nextScreen = nullptr;
  bool sleepAllowed = true;
  bool allowSleep() { return sleepAllowed; }
  void draw() { ++drawCalls; }
  void onButton(const ButtonEvent &e) { lastEvent = e.kind; }
  void onIdleTick() {}
  void onEnter() {}
} screen;
TestScreen *g_currentScreen = &screen;
namespace Lock {
bool isLocked() { return locked; }
bool isUnlockGesture(const ButtonEvent &ev) { return ev.kind == unlockKind; }
void disengage() { locked = false; }
} // namespace Lock
namespace WifiProvisioning {
void loop() {}
} // namespace WifiProvisioning
namespace Sleep {
bool inhibited() { return usbActive || !g_currentScreen->allowSleep(); }
uint32_t idleTimeoutMs() { return 10000; }
void enter() { ++sleepCalls; }
void idleLightSleep(bool) { ++lightCalls; }
} // namespace Sleep
namespace Icons {
bool trayStateChanged() { return false; }
} // namespace Icons
namespace ReaderMenu {
bool isActive() { return false; }
void draw() {}
} // namespace ReaderMenu
namespace Toast {
void show(const char *) {}
bool clearIfExpired() { return false; }
bool isActive() { return false; }
} // namespace Toast
namespace Statistics {
void bumpButtons(uint32_t) {}
} // namespace Statistics
struct Display {
  void fastmodeOff() {}
} display;
void forceNextRenderFull() {}
void forceNextMenuFrameFull() {}
bool headerIndicatorVisible() { return false; }
void updateBatteryCached() {
  if (rawPressAfterPoll) {
    rawPressAfterPoll = false;
    testPin = LOW;
  }
  if (injectAfterPoll) {
    injectAfterPoll = false;
    const auto now = testClockMs;
    testClockMs = now - 90;
    testPin = LOW;
    injectButtonEdgeNow(true);
    testClockMs = now - 40;
    testPin = HIGH;
    injectButtonEdgeNow(false);
    testClockMs = now;
  }
}
bool batteryChargingChanged() { return false; }
#include "idle-loop.inc"
namespace {
void reset() {
  clearButtonQueue();
  g_btns.resetState();
  ClickTimings::resetToDefaults();
  testPin = HIGH;
  testClockMs = 1000;
  markUserActivity();
  locked = usbActive = injectAfterPoll = rawPressAfterPoll = false;
  screen.sleepAllowed = true;
  screen.nextScreen = nullptr;
  g_currentScreen = &screen;
  generatedLoop();
  sleepCalls = lightCalls = drawCalls = 0;
  lastEvent = ButtonEvent::None;
}
void edge(uint32_t when, bool down) {
  testClockMs = when;
  testPin = down ? LOW : HIGH;
  injectButtonEdgeNow(down);
}
void tick(uint32_t when) {
  testClockMs = when;
  generatedLoop();
}
} // namespace
int main() {
  reset();
  tick(11001);
  assert(sleepCalls == 1);
  reset();
  edge(10990, true);
  tick(11031);
  assert(sleepCalls == 0 && lightCalls == 0);
  edge(11890, false);
  tick(11891);
  assert(sleepCalls == 0 && lastEvent == ButtonEvent::Long);
  reset();
  edge(10990, true);
  edge(11030, false);
  tick(11031);
  assert(sleepCalls == 0 && lightCalls == 0);
  tick(11210);
  assert(lastEvent == ButtonEvent::Short && sleepCalls == 0 && userIdleMs() == 0);
  reset();
  rawPressAfterPoll = true;
  tick(11031);
  assert(sleepCalls == 0 && lightCalls == 0);
  reset();
  injectAfterPoll = true;
  tick(11031);
  assert(sleepCalls == 0 && buttonQueueNonEmpty());
  tick(11210);
  assert(lastEvent == ButtonEvent::Short && sleepCalls == 0);
  reset();
  locked = true;
  edge(2490, true);
  edge(2530, false);
  tick(2531);
  assert(sleepCalls == 0 && locked);
  edge(2580, true);
  edge(2620, false);
  tick(2800);
  assert(!locked && sleepCalls == 0 && drawCalls == 1);
  tick(12801);
  assert(sleepCalls == 1);
  reset();
  locked = true;
  usbActive = true;
  tick(2501);
  assert(sleepCalls == 1);
  reset();
  usbActive = true;
  tick(11001);
  assert(sleepCalls == 0);
  usbActive = false;
  tick(11002);
  assert(sleepCalls == 1);
  reset();
  screen.sleepAllowed = false;
  tick(11001);
  assert(sleepCalls == 0);
  reset();
  testClockMs = 0xfffffff0u;
  markUserActivity();
  tick(9985);
  assert(sleepCalls == 1);
  puts("Actual sketch loop preserves held/pending/queued input at idle deadlines, custom unlock "
       "sequences, USB release and screen-owned sleep gates.");
}
