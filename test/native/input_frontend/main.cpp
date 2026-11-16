#include "src/hal/input.h"
#include "src/storage/click_timings.h"
#include <cstdio>
#include <initializer_list>
#include <cstdlib>
uint64_t testClockMs = 1000;
int testPin = HIGH;

static int checks = 0;
static void require(bool ok, const char *message) {
  ++checks;
  if (!ok) {
    fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}
static void reset(uint32_t gap = 175) {
  clearButtonQueue();
  g_btns.resetState();
  testClockMs = 1000;
  testPin = HIGH;
  ClickTimings::resetToDefaults();
  ClickTimings::setMaxClickGapMs(gap);
}
static void edge(uint32_t ms, bool down) {
  testClockMs = ms;
  testPin = down ? LOW : HIGH;
  injectButtonEdgeNow(down);
}
static ButtonEvent::Kind poll(uint32_t ms) {
  testClockMs = ms;
  g_btns.poll();
  return ButtonEvent::fromButtonState(g_btns).kind;
}
int main() {
  for (uint32_t gap : {120u, 175u, 250u}) {
    reset(gap);
    edge(1000, true);
    edge(1050, false);
    require(poll(1050 + gap - 1) == ButtonEvent::None, "single waits for configured silence");
    require(poll(1050 + gap + 1) == ButtonEvent::Short, "single commits after configured silence");
    reset(gap);
    edge(1000, true);
    edge(1050, false);
    edge(1100, true);
    edge(1150, false);
    require(poll(1150 + gap + 1) == ButtonEvent::Double, "queued double remains a double");
    reset(gap);
    edge(1000, true);
    edge(1050, false);
    edge(1050 + gap + 30, true);
    edge(1100 + gap + 30, false);
    require(poll(2000) == ButtonEvent::Short,
            "separated queued clicks do not merge during a redraw");
    require(poll(2001) == ButtonEvent::Short, "second queued click is not lost");
    require(!buttonQueueNonEmpty(), "queue drains in order");
  }
  reset();
  edge(1000, true);
  edge(1050, false);
  edge(1100, true);
  edge(1150, false);
  edge(1200, true);
  edge(1250, false);
  require(poll(1500) == ButtonEvent::Triple, "triple survives delayed polling");
  reset();
  edge(1000, true);
  edge(1900, false);
  edge(2200, true);
  edge(2250, false);
  require(poll(2500) == ButtonEvent::Long, "long queued before a short emits first");
  require(poll(2501) == ButtonEvent::Short, "short after long is not lost");
  reset();
  for (int i = 0; i < 5; ++i) {
    edge(1000 + i * 60, true);
    edge(1030 + i * 60, false);
  }
  require(poll(1600) == ButtonEvent::Quad, "quad commits without consuming fifth click");
  require(poll(1601) == ButtonEvent::Short, "fifth click starts a new sequence");
  reset();
  edge(1000, true);
  edge(1005, false);
  edge(1010, true);
  edge(1040, false);
  require(poll(1300) == ButtonEvent::Short, "contact bounce produces one click");
  require(g_btns.peekPressCount() == 1, "bounce does not inflate press statistics");
  reset();
  g_btns.seedPressOnWake(0);
  edge(200, false);
  require(poll(400) == ButtonEvent::Short, "wake-seeded press has a matching release");
  reset();
  edge(1000, true);
  require(poll(3000) == ButtonEvent::VeryLong, "held very-long gesture emits at threshold");
  edge(3100, false);
  require(poll(3300) == ButtonEvent::None, "very-long release is not repeated");
  reset();
  edge(1000, true);
  edge(1050, false);
  edge(1100, true);
  require(poll(2000) == ButtonEvent::ClickHold, "click-then-hold survives queued edges");
  edge(2100, false);
  require(poll(2300) == ButtonEvent::None, "click-hold release is consumed");
  reset();
  edge(0xfffffff0u, true);
  edge(34, false);
  require(poll(210) == ButtonEvent::Short, "click timing remains valid across millis rollover");
  for (uint32_t gap : {120u, 175u, 250u}) {
    reset(gap);
    g_btns.seedPressOnWake(0);
    edge(90, false);
    edge(180, true);
    edge(230, false);
    const auto before = testClockMs;
    discardWakePress(true, 0);
    require(testClockMs == before, "wake preparation never waits for release");
    require(poll(1200) == ButtonEvent::Short, "first intentional click during wake draw survives");
    require(g_btns.peekPressCount() == 1, "wake release does not inflate button statistics");
    require(poll(1201) == ButtonEvent::None, "wake and first click dispatch exactly once");
  }
  reset();
  edge(90, false);
  edge(180, true);
  edge(230, false);
  edge(300, true);
  edge(350, false);
  discardWakePress(true, 0);
  require(poll(1000) == ButtonEvent::Double, "double during wake redraw remains a double");
  require(g_btns.peekPressCount() == 2, "only intentional double presses are counted");
  reset();
  discardWakePress(true, 0);
  require(poll(3000) == ButtonEvent::None,
          "held wake press never becomes an unintended hold action");
  require(g_btns.isPressed(), "held wake press remains visible to idle sleep gating");
  edge(3100, false);
  edge(3200, true);
  edge(3250, false);
  require(poll(3600) == ButtonEvent::Short, "release after long wake hold unblocks next click");
  reset();
  edge(90, false);
  edge(180, true);
  edge(1080, false);
  discardWakePress(true, 0);
  require(poll(1400) == ButtonEvent::Long, "intentional long hold after wake is retained");
  reset();
  edge(180, true);
  edge(230, false);
  discardWakePress(false, 0);
  require(poll(1000) == ButtonEvent::Short,
          "wake released before ISR attachment does not discard first operation");
  reset();
  edge(90, false);
  edge(180, true);
  edge(230, false);
  edge(500, true);
  edge(550, false);
  discardWakePress(true, 0);
  require(poll(1000) == ButtonEvent::Short,
          "first of separated early wake clicks dispatches first");
  require(poll(1001) == ButtonEvent::Short, "second separated early wake click stays queued");
  reset();
  edge(90, false);
  edge(95, true);
  edge(100, false);
  edge(180, true);
  edge(230, false);
  discardWakePress(true, 0);
  require(poll(1000) == ButtonEvent::Short && g_btns.peekPressCount() == 1,
          "wake-release bounce does not leak or merge a gesture");
  reset();
  discardWakePress(true, 0xffffffe0u);
  edge(0xfffffff0u, false);
  edge(50, true);
  edge(100, false);
  require(poll(400) == ButtonEvent::Short, "wake preservation handles millis rollover");
  printf("%d input frontend checks passed\n", checks);
}
