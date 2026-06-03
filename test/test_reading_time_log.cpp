// Tests for src/pure/reading_time_log.cpp.
//
// applyReadingTime / viewReadingTime are the pure half of the reading-time
// feature — given a `ReadingTimeFile` (the on-disk wire format), a day index,
// and a per-page reading delta, they bucket the time into day/week/month/year
// and lifetime totals. The firmware (storage/statistics.cpp) keeps one of
// these structs in RTC RAM; these tests pin the roll-over + averaging rules so
// the displayed numbers stay correct as buckets advance.

#include "test_framework.h"
#include "pure/reading_time_log.h"

static ReadingTimeFile freshState() {
  ReadingTimeFile f{};
  f.version     = RTIME_SCHEMA;
  f.firstRtcSec = 0;
  return f;
}

TEST_CASE("applyReadingTime accumulates into every bucket on first log") {
  ReadingTimeFile f = freshState();
  f = applyReadingTime(f, /*day=*/0, /*seconds=*/60);
  CHECK_EQ(f.totalSeconds, 60u);
  CHECK_EQ(f.daySeconds,   60u);
  CHECK_EQ(f.weekSeconds,  60u);
  CHECK_EQ(f.monthSeconds, 60u);
  CHECK_EQ(f.yearSeconds,  60u);
}

TEST_CASE("applyReadingTime is a no-op for zero seconds") {
  ReadingTimeFile f = freshState();
  f = applyReadingTime(f, 3, 120);
  ReadingTimeFile g = applyReadingTime(f, 3, 0);
  CHECK_EQ(g.totalSeconds, f.totalSeconds);
  CHECK_EQ(g.daySeconds,   f.daySeconds);
}

TEST_CASE("applyReadingTime sums multiple logs on the same day") {
  ReadingTimeFile f = freshState();
  f = applyReadingTime(f, 5, 100);
  f = applyReadingTime(f, 5, 50);
  CHECK_EQ(f.daySeconds,   150u);
  CHECK_EQ(f.totalSeconds, 150u);
}

TEST_CASE("applyReadingTime resets the day bucket but keeps the week within a week") {
  ReadingTimeFile f = freshState();
  f = applyReadingTime(f, 0, 100);   // week 0
  f = applyReadingTime(f, 1, 40);    // next day, still week 0
  CHECK_EQ(f.daySeconds,   40u);     // day bucket rolled over
  CHECK_EQ(f.weekSeconds,  140u);    // week bucket kept accumulating
  CHECK_EQ(f.monthSeconds, 140u);
  CHECK_EQ(f.totalSeconds, 140u);
}

TEST_CASE("applyReadingTime rolls the week bucket on a new week") {
  ReadingTimeFile f = freshState();
  f = applyReadingTime(f, 0, 100);   // day 0, week 0
  f = applyReadingTime(f, 7, 30);    // day 7, week 1
  CHECK_EQ(f.weekSeconds,  30u);     // week bucket reset
  CHECK_EQ(f.monthSeconds, 130u);    // still month 0
  CHECK_EQ(f.totalSeconds, 130u);
}

TEST_CASE("applyReadingTime rolls month and year buckets at their boundaries") {
  ReadingTimeFile f = freshState();
  f = applyReadingTime(f, 0, 100);
  f = applyReadingTime(f, 30, 40);   // month 1, year 0
  CHECK_EQ(f.monthSeconds, 40u);
  CHECK_EQ(f.yearSeconds,  140u);
  f = applyReadingTime(f, 365, 10);  // year 1
  CHECK_EQ(f.yearSeconds,  10u);
  CHECK_EQ(f.totalSeconds, 150u);
}

TEST_CASE("applyReadingTime preserves version and firstRtcSec") {
  ReadingTimeFile f = freshState();
  f.firstRtcSec = 0x0BADF00D;
  f = applyReadingTime(f, 2, 10);
  CHECK_EQ(f.version,     RTIME_SCHEMA);
  CHECK_EQ(f.firstRtcSec, 0x0BADF00Du);
}

TEST_CASE("viewReadingTime shows current buckets and zeros stale ones") {
  ReadingTimeFile f = freshState();
  f = applyReadingTime(f, 10, 600);  // day 10, week 1, month 0, year 0

  // Viewed on the same day: all buckets live.
  ReadingTimeView v = viewReadingTime(f, 10);
  CHECK_EQ(v.today, 600u);
  CHECK_EQ(v.week,  600u);
  CHECK_EQ(v.month, 600u);
  CHECK_EQ(v.year,  600u);
  CHECK_EQ((unsigned)v.total, 600u);

  // Viewed the next day (day 11, same week): today resets, week persists.
  ReadingTimeView v2 = viewReadingTime(f, 11);
  CHECK_EQ(v2.today, 0u);
  CHECK_EQ(v2.week,  600u);

  // Viewed a week later (day 17, week 2): week resets too, month persists.
  ReadingTimeView v3 = viewReadingTime(f, 17);
  CHECK_EQ(v3.today, 0u);
  CHECK_EQ(v3.week,  0u);
  CHECK_EQ(v3.month, 600u);
}

TEST_CASE("viewReadingTime averages lifetime total over elapsed buckets") {
  ReadingTimeFile f = freshState();
  // 700s total spread so the view sees it as lifetime total.
  f = applyReadingTime(f, 0, 700);
  // On day 6 (7 elapsed days, 1 elapsed week): avg/day = 100, avg/week = 700.
  ReadingTimeView v = viewReadingTime(f, 6);
  CHECK_EQ((unsigned)v.total, 700u);
  CHECK_EQ(v.avgPerDay,  100u);
  CHECK_EQ(v.avgPerWeek, 700u);

  // On day 13 (14 elapsed days, 2 elapsed weeks): avg/day = 50, avg/week = 350.
  ReadingTimeView v2 = viewReadingTime(f, 13);
  CHECK_EQ(v2.avgPerDay,  50u);
  CHECK_EQ(v2.avgPerWeek, 350u);
}
