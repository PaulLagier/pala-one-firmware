#include "src/pure/streak_log.h"

StreakLogResult applyStreakLog(const ReadingStreakFile& current, uint32_t today) {
  StreakLogResult r{};
  if (current.lastLoggedDay == today) {
    r.changed = false;
    r.next    = current;
    return r;
  }

  ReadingStreakFile s = current;

  if (today > s.bitmapHead) {
    uint32_t d = today - s.bitmapHead;
    s.bitmap     = (d >= 32) ? 0u : (s.bitmap << d);
    s.bitmapHead = today;
  }
  s.bitmap |= 1u;

  bool cont = (s.lastLoggedDay != STREAK_DAY_UNSET)
           && (today == s.lastLoggedDay + 1);
  s.currentStreak  = cont ? (s.currentStreak + 1) : 1;
  if (s.currentStreak > s.longestStreak) s.longestStreak = s.currentStreak;
  s.lastLoggedDay  = today;
  s.totalSessions += 1;

  r.changed = true;
  r.next    = s;
  return r;
}
