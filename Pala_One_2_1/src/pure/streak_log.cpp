#include "src/pure/streak_log.h"

// Whole days in `secs`, rounded to nearest, floored at 2. Used only to size
// the bitmap gap after a lapse, so the statistics grid shows roughly the right
// number of blank cells. Written as divide-then-compare rather than
// `(secs + halfDay) / day` so it cannot overflow on a nonsense timestamp.
static uint32_t lapsedDays(uint32_t secs) {
  uint32_t days = secs / STREAK_DAY_SECS;
  if ((secs % STREAK_DAY_SECS) >= (STREAK_DAY_SECS / 2)) days += 1;
  return (days < 2u) ? 2u : days;
}

StreakLogResult applyStreakLog(const ReadingStreakFile& current, uint32_t nowSec) {
  ReadingStreakFile s = current;

  // ---- 1. First ever log -------------------------------------------------
  if (s.lastLogSec == STREAK_SEC_UNSET) {
    s.dayIndex      = 0;
    s.bitmap        = 1u;
    s.currentStreak = 1;
    if (s.longestStreak < 1u) s.longestStreak = 1u;
    s.totalSessions += 1;
    s.lastLogSec     = nowSec;
    s.lastReadSec    = nowSec;
    return StreakLogResult{true, true, s};
  }

  // ---- 2. RTC ran backwards ----------------------------------------------
  // `lastReadSec >= lastLogSec` is an invariant of everything below, so one
  // test covers both anchors. A power cycle restarts the counter, and elapsed
  // time is then unknowable: re-anchor and leave the streak exactly as it was.
  if (nowSec < s.lastReadSec) {
    s.lastLogSec  = nowSec;
    s.lastReadSec = nowSec;
    return StreakLogResult{true, false, s};
  }

  const uint32_t sinceLog  = nowSec - s.lastLogSec;
  const uint32_t sinceRead = nowSec - s.lastReadSec;

  // ---- 3. Same reading day -----------------------------------------------
  // Still counts as activity: moving lastReadSec pushes the break deadline
  // out, so an evening spent picking the book up and putting it down keeps
  // the streak alive without inflating it.
  if (sinceLog < STREAK_SAME_DAY_SECS) {
    s.lastReadSec = nowSec;
    return StreakLogResult{s.lastReadSec != current.lastReadSec, false, s};
  }

  // ---- 4. A new streak day -----------------------------------------------
  uint32_t step;
  if (sinceRead <= STREAK_WINDOW_SECS) {
    step             = 1u;
    s.currentStreak += 1;
  } else {
    step            = lapsedDays(sinceRead);
    s.currentStreak = 1;
  }

  s.dayIndex += step;
  s.bitmap    = (step >= 32u) ? 0u : (s.bitmap << step);
  s.bitmap   |= 1u;

  if (s.currentStreak > s.longestStreak) s.longestStreak = s.currentStreak;
  s.totalSessions += 1;
  s.lastLogSec     = nowSec;
  s.lastReadSec    = nowSec;

  return StreakLogResult{true, true, s};
}

ReadingStreakFile migrateStreakV1(const ReadingStreakFileV1& v1, uint32_t nowSec) {
  ReadingStreakFile s{};
  s.version       = STREAK_SCHEMA;
  s.firstRtcSec   = v1.firstRtcSec;
  s.lastLogSec    = nowSec;
  s.lastReadSec   = nowSec;
  s.dayIndex      = v1.bitmapHead;
  s.currentStreak = v1.currentStreak;
  s.longestStreak = v1.longestStreak;
  s.totalSessions = v1.totalSessions;
  s.bitmap        = v1.bitmap;
  return s;
}
