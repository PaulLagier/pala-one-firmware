// Tests for src/pure/streak_log.cpp.
//
// applyStreakLog is the pure half of the reading-streak feature — given a
// `ReadingStreakFile` (the on-disk wire format) and `today` (a day index),
// it computes the next state. The bitmap-shift + continuation rules need
// to match the in-app logic byte for byte so manual taps and the
// firmware's auto-log produce identical numbers; these tests pin that.

#include "test_framework.h"
#include "pure/streak_log.h"

static ReadingStreakFile freshState() {
  ReadingStreakFile s{};
  s.version       = STREAK_SCHEMA;
  s.firstRtcSec   = 0;
  s.lastLoggedDay = STREAK_DAY_UNSET;
  s.currentStreak = 0;
  s.longestStreak = 0;
  s.totalSessions = 0;
  s.bitmapHead    = 0;
  s.bitmap        = 0;
  return s;
}

TEST_CASE("applyStreakLog starts streak at 1 on first ever log") {
  ReadingStreakFile s = freshState();
  StreakLogResult r = applyStreakLog(s, 10);
  REQUIRE(r.changed);
  CHECK_EQ(r.next.currentStreak, 1u);
  CHECK_EQ(r.next.longestStreak, 1u);
  CHECK_EQ(r.next.totalSessions, 1u);
  CHECK_EQ(r.next.lastLoggedDay, 10u);
  CHECK_EQ(r.next.bitmapHead, 10u);
  CHECK_EQ(r.next.bitmap & 1u, 1u);
}

TEST_CASE("applyStreakLog continues streak on consecutive days") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, 10).next;
  s = applyStreakLog(s, 11).next;
  s = applyStreakLog(s, 12).next;
  CHECK_EQ(s.currentStreak, 3u);
  CHECK_EQ(s.longestStreak, 3u);
  CHECK_EQ(s.totalSessions, 3u);
  CHECK_EQ(s.lastLoggedDay, 12u);
}

TEST_CASE("applyStreakLog resets streak after a gap but keeps longest") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, 10).next;
  s = applyStreakLog(s, 11).next;
  s = applyStreakLog(s, 12).next;   // streak 3, longest 3
  s = applyStreakLog(s, 20).next;   // gap of 7 days -> reset to 1
  CHECK_EQ(s.currentStreak, 1u);
  CHECK_EQ(s.longestStreak, 3u);
  CHECK_EQ(s.totalSessions, 4u);
}

TEST_CASE("applyStreakLog is a no-op when logging the same day twice") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, 10).next;
  StreakLogResult r = applyStreakLog(s, 10);
  CHECK(!r.changed);
  CHECK_EQ(r.next.currentStreak, 1u);
  CHECK_EQ(r.next.totalSessions, 1u);
}

TEST_CASE("applyStreakLog bitmap tracks the last 32 days") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, 100).next;
  s = applyStreakLog(s, 101).next;  // bit 1
  s = applyStreakLog(s, 103).next;  // bit 3 (gap of 1 day stays in window)
  // Bitmap: today (bit 0) + bit 2 (day 101) + bit 3 (day 100... wait)
  // After head=103: shifted by (103-100)=3 from prior head 100.
  // bitmap before shift: 0b0110 (bits 0,1,2 from 100/101/102? actually
  // we set bit0 each log). Let's just check the shape:
  //   day 100 logged -> head=100, bitmap = ...0001
  //   day 101 logged -> shift by 1 -> ...0010 then |1 -> ...0011, head=101
  //   day 103 logged -> shift by 2 -> ...1100 then |1 -> ...1101, head=103
  CHECK_EQ(s.bitmapHead, 103u);
  CHECK_EQ(s.bitmap, 0b1101u);
}

TEST_CASE("applyStreakLog clears bitmap when shift >= 32") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, 100).next;
  s = applyStreakLog(s, 101).next;
  // Jump > 32 days. The bitmap shift of >=32 in a uint32_t is undefined in
  // C++ if done with a raw `<<`; applyStreakLog clamps to 0 explicitly so
  // the result is deterministic.
  s = applyStreakLog(s, 200).next;
  CHECK_EQ(s.bitmapHead, 200u);
  CHECK_EQ(s.bitmap, 1u);  // only today's bit
}

TEST_CASE("applyStreakLog preserves firstRtcSec and version") {
  ReadingStreakFile s = freshState();
  s.firstRtcSec = 0x12345678;
  StreakLogResult r = applyStreakLog(s, 5);
  CHECK_EQ(r.next.firstRtcSec, 0x12345678u);
  CHECK_EQ(r.next.version,     STREAK_SCHEMA);
}
