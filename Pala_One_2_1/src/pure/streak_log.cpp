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
    step            = lapsedDays(sinceLog);   // bit 0 is the last *logged* day
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

ReadingStreakFile streakAsOf(const ReadingStreakFile& current, uint32_t nowSec) {
  ReadingStreakFile s = current;
  if (s.lastLogSec == STREAK_SEC_UNSET) return s;              // never logged
  if (nowSec < s.lastReadSec) return s;                        // RTC reset: unknowable
  if (nowSec - s.lastReadSec <= STREAK_WINDOW_SECS) return s;  // still live

  const uint32_t step = lapsedDays(nowSec - s.lastLogSec);
  s.currentStreak = 0;
  s.dayIndex     += step;
  s.bitmap        = (step >= 32u) ? 0u : (s.bitmap << step);
  return s;
}

ReadingStreakFile migrateStreakV1(const ReadingStreakFileV1& v1, uint32_t nowSec) {
  ReadingStreakFile s{};
  s.version       = STREAK_SCHEMA;
  s.firstRtcSec   = v1.firstRtcSec;
  s.dayIndex      = v1.bitmapHead;
  s.currentStreak = v1.currentStreak;
  s.longestStreak = v1.longestStreak;
  s.totalSessions = v1.totalSessions;
  s.bitmap        = v1.bitmap;

  // Never logged under v1: start clean so the first session logs at once.
  if (v1.lastLoggedDay == 0xFFFFFFFFu) {   // v1 STREAK_DAY_UNSET
    s.lastLogSec  = STREAK_SEC_UNSET;
    s.lastReadSec = 0;
    return s;
  }

  s.lastLogSec  = nowSec;
  s.lastReadSec = nowSec;

  // If the RTC has not reset since v1 anchored, v1's own day index still
  // says whether the streak is live and whether today was already logged.
  // A counter below v1's anchor, or one that puts "today" before the last
  // logged day, proves a reset: keep the streak as it stands.
  if (nowSec < v1.firstRtcSec) return s;
  const uint32_t v1Today = (nowSec - v1.firstRtcSec) / STREAK_DAY_SECS;
  if (v1Today < v1.lastLoggedDay) return s;   // counter restarted since v1 anchored

  if (v1Today > v1.lastLoggedDay + 1u) {
    s.currentStreak = 0;   // already lapsed under v1: don't resurrect it
  }
  if (v1Today > v1.bitmapHead) {
    // Park the head on "yesterday" so the next log lands on today's cell and
    // any missed days show as blanks.
    const uint32_t shift = v1Today - 1u - v1.bitmapHead;
    s.bitmap   = (shift >= 32u) ? 0u : (v1.bitmap << shift);
    s.dayIndex = v1Today - 1u;
  }
  if (v1Today == v1.lastLoggedDay) {
    // Already logged today, at an unknown time since v1's day began. Assume
    // the start, but keep a log shut for STREAK_MIGRATE_HOLD after the update.
    // v1Today == lastLoggedDay puts dayStart at or before nowSec.
    const uint32_t dayStart = v1.firstRtcSec + v1.lastLoggedDay * STREAK_DAY_SECS;
    const uint32_t back     = STREAK_SAME_DAY_SECS - STREAK_MIGRATE_HOLD;
    const uint32_t latest   = (nowSec >= back) ? nowSec - back : 0u;
    s.lastLogSec = (dayStart > latest) ? dayStart : latest;
  } else if (nowSec >= STREAK_SAME_DAY_SECS) {
    s.lastLogSec = nowSec - STREAK_SAME_DAY_SECS;   // today not logged yet: allow it
  }
  return s;
}
