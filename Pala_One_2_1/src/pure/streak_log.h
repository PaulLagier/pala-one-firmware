#ifndef PALA_PURE_STREAK_LOG_H
#define PALA_PURE_STREAK_LOG_H

#include "src/pure/arduino_compat.h"  // uint32_t

// ============================================================================
//  Reading-streak wire format + pure log-advance logic.
//
//  Lives in pure/ so the window + bitmap-shift rules are host-testable
//  independent of LittleFS / RTC / Toast. The firmware side (statistics.cpp)
//  reads the `cfg_streak` NVS blob into one of these structs, calls
//  `applyStreakLog` to compute the next state, and writes it back. The same
//  struct layout is shared with examples/reading_streak/app.c (schema-version
//  gated).
//
//  --------------------------------------------------------------------------
//  Why this is a rolling window and not a day index
//  --------------------------------------------------------------------------
//  The device has no wall clock. Schema v1 defined a "day" as
//  `(rtcSeconds() - firstRtcSec) / 86400`, i.e. fixed 24 h buckets anchored to
//  whenever the streak first initialised. That anchor is an arbitrary time of
//  day, so the bucket boundary usually fell inside somebody's normal reading
//  window: read at 23:00 on Monday and 01:00 on Wednesday and you had skipped
//  a bucket, while reading at 21:00 and 23:00 on the same evening crossed one
//  and counted twice. Users had to read at roughly the same hour every day to
//  keep a streak alive.
//
//  v2 drops the fixed anchor. Two timestamps carry the state:
//
//    lastLogSec   RTC seconds when the streak last advanced. Nothing can
//                 advance again until STREAK_SAME_DAY_SECS has passed, so a
//                 second sitting on the same evening is a no-op rather than
//                 a free extra day.
//
//    lastReadSec  RTC seconds of the most recent qualifying session, logged
//                 or not. The streak breaks only when *this* is more than
//                 STREAK_WINDOW_SECS old, so picking the book back up at any
//                 point inside the window keeps it alive.
//
//  Neither threshold can be exactly right without a calendar: a gap of 30 h
//  might be two late-night sessions in a row or one skipped day, and the
//  device cannot tell. The defaults below deliberately err towards keeping a
//  streak the reader earned rather than breaking one they did not.
//
//  A consequence worth knowing: because the floor is 18 h rather than 24 h, a
//  reader who drifts steadily earlier can bank at most four days in three.
//  That is the price of never punishing ordinary time-of-day jitter.
// ============================================================================

static const uint32_t STREAK_SCHEMA          = 2;
static const uint32_t STREAK_DAY_SECS        = 86400u;    // nominal day, for gap sizing
static const uint32_t STREAK_SAME_DAY_SECS   = 64800u;    // 18 h — below this, same reading day
static const uint32_t STREAK_WINDOW_SECS     = 129600u;   // 36 h — above this, the streak lapses
static const uint32_t STREAK_SEC_UNSET       = 0xFFFFFFFFu;
static const int      STREAK_PAGES_THRESHOLD = 5;         // page turns before a session counts

struct ReadingStreakFile {
  uint32_t version;        // == STREAK_SCHEMA
  uint32_t firstRtcSec;    // rtcSeconds() at first ever write
  uint32_t lastLogSec;     // RTC seconds at the last advance, STREAK_SEC_UNSET = never
  uint32_t lastReadSec;    // RTC seconds at the last qualifying session
  uint32_t dayIndex;       // rolling day ordinal; doubles as the bitmap head
  uint32_t currentStreak;
  uint32_t longestStreak;
  uint32_t totalSessions;
  uint32_t bitmap;         // bit i = "read on day (dayIndex - i)"
};

// Schema v1 on-disk shape. Retained only so `migrateStreakV1` can carry an
// existing streak forward; nothing else should reference it.
struct ReadingStreakFileV1 {
  uint32_t version;        // == 1
  uint32_t firstRtcSec;
  uint32_t lastLoggedDay;
  uint32_t currentStreak;
  uint32_t longestStreak;
  uint32_t totalSessions;
  uint32_t bitmapHead;
  uint32_t bitmap;
};

struct StreakLogResult {
  bool              changed;  // `next` differs from `current`; caller may persist
  bool              logged;   // a new streak day was recorded; caller toasts
  ReadingStreakFile next;
};

// Advance the streak state for reading activity at `nowSec` (monotonic RTC
// seconds). Pure — the same `current` + `nowSec` always yield the same result.
//
// Behaviour, in the order the cases are tested:
//
//   1. lastLogSec == STREAK_SEC_UNSET  → first ever log. dayIndex 0, bitmap 1,
//      currentStreak 1. {changed, logged}.
//   2. nowSec < lastReadSec  → the RTC counter ran backwards, which on this
//      hardware means a power cycle rather than time travel. Elapsed time is
//      unknowable, so both anchors move to `nowSec` and the streak is left
//      exactly as it was: not advanced, not broken. {changed, not logged}.
//   3. nowSec - lastLogSec < STREAK_SAME_DAY_SECS  → same reading day. Only
//      lastReadSec moves, which pushes the break deadline out. {changed iff
//      lastReadSec actually moved, not logged}.
//   4. Otherwise a new streak day is recorded:
//        - continuing (nowSec - lastReadSec <= STREAK_WINDOW_SECS):
//          currentStreak++, dayIndex += 1, bitmap shifts 1.
//        - lapsed: currentStreak = 1, and dayIndex / the bitmap skip the whole
//          gap (elapsed time rounded to whole days, at least 2) so the grid on
//          the statistics screen shows the missed days as blanks.
//      longestStreak = max(longestStreak, currentStreak), totalSessions++,
//      both anchors move to `nowSec`. {changed, logged}.
//
// A shift of 32 or more clears the bitmap outright rather than relying on
// `<<`, which is undefined at that width.
StreakLogResult applyStreakLog(const ReadingStreakFile& current, uint32_t nowSec);

// Convert a schema-v1 blob into a v2 one, preserving currentStreak,
// longestStreak, totalSessions and the 30-day bitmap.
//
// The v1 day index counted 24 h buckets from `firstRtcSec`, which is not
// comparable to a wall-clock-free rolling window, so both v2 anchors are set
// to `nowSec`: the reader keeps the streak they earned and gets a full fresh
// window from the moment they take the update. The session that triggers the
// migration does not itself log a day; the next one, up to 36 h later, does.
ReadingStreakFile migrateStreakV1(const ReadingStreakFileV1& v1, uint32_t nowSec);

#endif  // PALA_PURE_STREAK_LOG_H
