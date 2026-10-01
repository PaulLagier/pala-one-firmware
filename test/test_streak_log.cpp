// Tests for src/pure/streak_log.cpp.
//
// applyStreakLog is the pure half of the reading-streak feature — given a
// `ReadingStreakFile` (the on-disk wire format) and the current monotonic RTC
// second, it computes the next state. Schema v2 replaced v1's fixed 24 h
// buckets with a rolling window, because the v1 buckets were anchored to an
// arbitrary time of day and broke streaks for anyone who didn't read at the
// same hour every day.
//
// The cases below pin both halves of that: the streak must survive ordinary
// time-of-day jitter, and it must still refuse to count two sittings on one
// evening as two days.

#include "test_framework.h"
#include "pure/streak_log.h"

static const uint32_t H    = 3600u;      // one hour in seconds
static const uint32_t BASE = 100000u;    // arbitrary non-zero RTC reading

static ReadingStreakFile freshState() {
  ReadingStreakFile s{};
  s.version       = STREAK_SCHEMA;
  s.firstRtcSec   = 0;
  s.lastLogSec    = STREAK_SEC_UNSET;
  s.lastReadSec   = 0;
  s.dayIndex      = 0;
  s.currentStreak = 0;
  s.longestStreak = 0;
  s.totalSessions = 0;
  s.bitmap        = 0;
  return s;
}

// ---------------------------------------------------------------------------
//  Wire format
// ---------------------------------------------------------------------------

TEST_CASE("streak schema versions have distinct blob sizes") {
  // statistics.cpp tells v1 and v2 blobs apart by their stored length before
  // it trusts the version word, so these must never collide.
  CHECK_EQ(sizeof(ReadingStreakFile),   (size_t)36);
  CHECK_EQ(sizeof(ReadingStreakFileV1), (size_t)32);
  CHECK_EQ(STREAK_SCHEMA, 2u);
}

// ---------------------------------------------------------------------------
//  Basic advance
// ---------------------------------------------------------------------------

TEST_CASE("applyStreakLog starts streak at 1 on first ever log") {
  ReadingStreakFile s = freshState();
  StreakLogResult r = applyStreakLog(s, BASE);
  REQUIRE(r.changed);
  CHECK(r.logged);
  CHECK_EQ(r.next.currentStreak, 1u);
  CHECK_EQ(r.next.longestStreak, 1u);
  CHECK_EQ(r.next.totalSessions, 1u);
  CHECK_EQ(r.next.lastLogSec,  BASE);
  CHECK_EQ(r.next.lastReadSec, BASE);
  CHECK_EQ(r.next.dayIndex, 0u);
  CHECK_EQ(r.next.bitmap & 1u, 1u);
}

TEST_CASE("applyStreakLog continues streak on consecutive days") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;
  s = applyStreakLog(s, BASE + 24 * H).next;
  s = applyStreakLog(s, BASE + 48 * H).next;
  CHECK_EQ(s.currentStreak, 3u);
  CHECK_EQ(s.longestStreak, 3u);
  CHECK_EQ(s.totalSessions, 3u);
  CHECK_EQ(s.dayIndex, 2u);
}

TEST_CASE("applyStreakLog preserves firstRtcSec and version") {
  ReadingStreakFile s = freshState();
  s.firstRtcSec = 0x12345678;
  StreakLogResult r = applyStreakLog(s, BASE);
  CHECK_EQ(r.next.firstRtcSec, 0x12345678u);
  CHECK_EQ(r.next.version,     STREAK_SCHEMA);
}

TEST_CASE("applyStreakLog does not mutate its input and is deterministic") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;

  const ReadingStreakFile before = s;
  StreakLogResult a = applyStreakLog(s, BASE + 25 * H);
  StreakLogResult b = applyStreakLog(s, BASE + 25 * H);

  CHECK_EQ(s.currentStreak, before.currentStreak);
  CHECK_EQ(s.lastLogSec,    before.lastLogSec);
  CHECK_EQ(a.next.currentStreak, b.next.currentStreak);
  CHECK_EQ(a.next.lastLogSec,    b.next.lastLogSec);
  CHECK_EQ(a.next.bitmap,        b.next.bitmap);
}

// ---------------------------------------------------------------------------
//  The reported problem: reading at a different hour each night
// ---------------------------------------------------------------------------

TEST_CASE("streak survives night-to-night jitter that v1 broke") {
  // Mon 22:00, a nightcap at Tue 01:00, then Tue 23:00, Wed 19:00, Thu 23:00.
  // Under v1's fixed buckets at least one of these fell on the wrong side of a
  // boundary. Under v2 it is one unbroken five-day streak.
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE +  0 * H).next;   // Mon 22:00  -> day 1
  s = applyStreakLog(s, BASE +  3 * H).next;   // Tue 01:00  -> same day
  s = applyStreakLog(s, BASE + 25 * H).next;   // Tue 23:00  -> day 2
  s = applyStreakLog(s, BASE + 45 * H).next;   // Wed 19:00  -> day 3
  s = applyStreakLog(s, BASE + 73 * H).next;   // Thu 23:00  -> day 4
  CHECK_EQ(s.currentStreak, 4u);
  CHECK_EQ(s.totalSessions, 4u);
}

TEST_CASE("a 28 hour gap continues the streak") {
  // The literal "24 h + 2 h grace" reading of the suggestion would break here.
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;
  StreakLogResult r = applyStreakLog(s, BASE + 28 * H);
  CHECK(r.logged);
  CHECK_EQ(r.next.currentStreak, 2u);
}

TEST_CASE("a sitting that logs nothing still holds the window open") {
  // Read Monday night, a short Tuesday sitting that is too soon to open a new
  // day, then Wednesday evening. Measured from the last *logged* day that is
  // 46 h and would lapse; measured from the last *read* it is 34 h and holds.
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;                    // day 1
  StreakLogResult mid = applyStreakLog(s, BASE + 12 * H);
  CHECK(mid.changed);            // lastReadSec moved
  CHECK(!mid.logged);            // but no new day
  s = mid.next;

  StreakLogResult r = applyStreakLog(s, BASE + 46 * H);
  CHECK(r.logged);
  CHECK_EQ(r.next.currentStreak, 2u);
}

// ---------------------------------------------------------------------------
//  Same reading day
// ---------------------------------------------------------------------------

TEST_CASE("reading twice in one evening counts once") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;
  s = applyStreakLog(s, BASE + 6 * H).next;
  s = applyStreakLog(s, BASE + 10 * H).next;
  CHECK_EQ(s.currentStreak, 1u);
  CHECK_EQ(s.totalSessions, 1u);
  CHECK_EQ(s.dayIndex, 0u);
  CHECK_EQ(s.lastReadSec, BASE + 10 * H);   // window still being pushed out
}

TEST_CASE("same-day re-read at the identical second is a no-op") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;
  StreakLogResult r = applyStreakLog(s, BASE);
  CHECK(!r.changed);
  CHECK(!r.logged);
  CHECK_EQ(r.next.currentStreak, 1u);
  CHECK_EQ(r.next.totalSessions, 1u);
}

TEST_CASE("the same-day floor is exactly STREAK_SAME_DAY_SECS") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;

  StreakLogResult just_under = applyStreakLog(s, BASE + STREAK_SAME_DAY_SECS - 1);
  CHECK(!just_under.logged);

  StreakLogResult exact = applyStreakLog(s, BASE + STREAK_SAME_DAY_SECS);
  CHECK(exact.logged);
  CHECK_EQ(exact.next.currentStreak, 2u);
}

// ---------------------------------------------------------------------------
//  Lapsing
// ---------------------------------------------------------------------------

TEST_CASE("applyStreakLog resets streak after a lapse but keeps longest") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;
  s = applyStreakLog(s, BASE + 24 * H).next;
  s = applyStreakLog(s, BASE + 48 * H).next;   // streak 3, longest 3
  s = applyStreakLog(s, BASE + 48 * H + 7 * 24 * H).next;
  CHECK_EQ(s.currentStreak, 1u);
  CHECK_EQ(s.longestStreak, 3u);
  CHECK_EQ(s.totalSessions, 4u);
}

TEST_CASE("the break ceiling is exactly STREAK_WINDOW_SECS") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;

  StreakLogResult exact = applyStreakLog(s, BASE + STREAK_WINDOW_SECS);
  CHECK(exact.logged);
  CHECK_EQ(exact.next.currentStreak, 2u);

  StreakLogResult over = applyStreakLog(s, BASE + STREAK_WINDOW_SECS + 1);
  CHECK(over.logged);
  CHECK_EQ(over.next.currentStreak, 1u);
}

// ---------------------------------------------------------------------------
//  Bitmap
// ---------------------------------------------------------------------------

TEST_CASE("bitmap records consecutive days as adjacent bits") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;             // dayIndex 0, bitmap ...0001
  s = applyStreakLog(s, BASE + 24 * H).next;    // dayIndex 1, bitmap ...0011
  s = applyStreakLog(s, BASE + 48 * H).next;    // dayIndex 2, bitmap ...0111
  CHECK_EQ(s.dayIndex, 2u);
  CHECK_EQ(s.bitmap, 0b111u);
}

TEST_CASE("a lapse leaves the missed days blank in the bitmap") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;             // dayIndex 0, bitmap ...0001
  s = applyStreakLog(s, BASE + 24 * H).next;    // dayIndex 1, bitmap ...0011

  // Three days later: the head jumps by 3, so two blank cells sit between the
  // old run and today.
  s = applyStreakLog(s, BASE + 24 * H + 72 * H).next;
  CHECK_EQ(s.dayIndex, 4u);
  CHECK_EQ(s.bitmap, 0b11001u);
}

TEST_CASE("lapse length rounds to the nearest whole day, floored at two") {
  // 37 h is one and a half days, which rounds to 2 — the shortest a lapse can
  // possibly be, since anything up to 36 h would have continued the streak.
  ReadingStreakFile a = freshState();
  a = applyStreakLog(a, BASE).next;
  a = applyStreakLog(a, BASE + 37 * H).next;
  CHECK_EQ(a.dayIndex, 2u);
  CHECK_EQ(a.bitmap, 0b101u);

  // 60 h rounds up to 3.
  ReadingStreakFile b = freshState();
  b = applyStreakLog(b, BASE).next;
  b = applyStreakLog(b, BASE + 60 * H).next;
  CHECK_EQ(b.dayIndex, 3u);
  CHECK_EQ(b.bitmap, 0b1001u);
}

TEST_CASE("bitmap clears when the lapse is 32 days or more") {
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;
  s = applyStreakLog(s, BASE + 24 * H).next;
  // A shift of >= 32 on a uint32_t is undefined behaviour with a raw `<<`;
  // applyStreakLog clamps to 0 so the result is deterministic.
  s = applyStreakLog(s, BASE + 24 * H + 40 * 24 * H).next;
  CHECK_EQ(s.dayIndex, 41u);
  CHECK_EQ(s.bitmap, 1u);          // only today's bit
  CHECK_EQ(s.currentStreak, 1u);
}

// ---------------------------------------------------------------------------
//  Clock hazards
// ---------------------------------------------------------------------------

TEST_CASE("a backwards RTC re-anchors without breaking or advancing") {
  // The RTC counter restarts at zero on a hard power cycle. Elapsed time is
  // then unknowable, so the streak is left alone rather than reset. Under v1
  // this case silently froze the streak forever.
  ReadingStreakFile s = freshState();
  s = applyStreakLog(s, BASE).next;
  s = applyStreakLog(s, BASE + 24 * H).next;   // streak 2

  StreakLogResult r = applyStreakLog(s, 5);    // counter went back to ~0
  CHECK(r.changed);
  CHECK(!r.logged);
  CHECK_EQ(r.next.currentStreak, 2u);
  CHECK_EQ(r.next.totalSessions, 2u);
  CHECK_EQ(r.next.dayIndex,      1u);
  CHECK_EQ(r.next.lastLogSec,    5u);
  CHECK_EQ(r.next.lastReadSec,   5u);

  // And the streak keeps going from the new anchor.
  StreakLogResult n = applyStreakLog(r.next, 5 + 24 * H);
  CHECK(n.logged);
  CHECK_EQ(n.next.currentStreak, 3u);
}

// ---------------------------------------------------------------------------
//  v1 -> v2 migration
// ---------------------------------------------------------------------------

TEST_CASE("migrateStreakV1 carries the streak across and re-anchors the clock") {
  ReadingStreakFileV1 v1{};
  v1.version       = 1;
  v1.firstRtcSec   = 500;
  v1.lastLoggedDay = 42;
  v1.currentStreak = 7;
  v1.longestStreak = 9;
  v1.totalSessions = 30;
  v1.bitmapHead    = 42;
  v1.bitmap        = 0b1011u;

  ReadingStreakFile s = migrateStreakV1(v1, BASE);
  CHECK_EQ(s.version,       STREAK_SCHEMA);
  CHECK_EQ(s.firstRtcSec,   500u);
  CHECK_EQ(s.currentStreak, 7u);
  CHECK_EQ(s.longestStreak, 9u);
  CHECK_EQ(s.totalSessions, 30u);
  CHECK_EQ(s.dayIndex,      42u);
  CHECK_EQ(s.bitmap,        0b1011u);
  CHECK_EQ(s.lastLogSec,    BASE);
  CHECK_EQ(s.lastReadSec,   BASE);
}

TEST_CASE("a migrated reader continues rather than restarting") {
  ReadingStreakFileV1 v1{};
  v1.version       = 1;
  v1.currentStreak = 7;
  v1.longestStreak = 9;
  v1.totalSessions = 30;
  v1.bitmapHead    = 42;
  v1.bitmap        = 0b1011u;

  ReadingStreakFile s = migrateStreakV1(v1, BASE);
  StreakLogResult r = applyStreakLog(s, BASE + 30 * H);
  CHECK(r.logged);
  CHECK_EQ(r.next.currentStreak, 8u);
  CHECK_EQ(r.next.longestStreak, 9u);
  CHECK_EQ(r.next.dayIndex,      43u);
}
