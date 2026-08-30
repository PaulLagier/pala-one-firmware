#ifndef PALA_STORAGE_STATISTICS_H
#define PALA_STORAGE_STATISTICS_H

#include "src/pure/arduino_compat.h"  // uint32_t, uint64_t

// ============================================================================
//  Statistics — firmware-internal reading dashboard.
//
//  Tracks two kinds of activity:
//
//    1. Lifetime counters (pagesRead, buttonPresses). Working values live
//       in RTC RAM so normal use takes zero flash writes; we flush to NVS
//       every STATS_FLUSH_EVERY_EVENTS bumps as a safety net and on
//       Sleep::enter so deep-sleep cycles preserve the delta. Cold-boot
//       reloads from NVS into the RTC counters.
//
//    2. Reading streak (currentStreak, longestStreak, totalSessions,
//       lastLogSec, lastReadSec, dayIndex, bitmap). Persisted straight to
//       NVS. A logged day happens at most once per rolling day; the
//       lastReadSec refresh that keeps the window open is rate-limited to
//       one write per STREAK_REFRESH_SECS, so flash wear stays a non-issue.
//       The shape is `ReadingStreakFile` from src/pure/streak_log.h;
//       the window + log-advance rules live in src/pure/streak_log.cpp and
//       are host-tested.
//
//  Both feed StatisticsScreen (src/ui/screens/statistics_screen) through
//  one read-only snapshot accessor — the screen never touches state
//  directly.
// ============================================================================

struct StatisticsSnapshot {
  // Lifetime
  uint64_t pagesRead;
  uint64_t buttonPresses;
  uint32_t firstStatsRtcSec;   // RTC seconds when stats were first initialised

  // Streak
  uint32_t firstStreakRtcSec;  // RTC seconds when streak was first initialised
  uint32_t currentStreak;
  uint32_t longestStreak;
  uint32_t totalSessions;
  uint32_t lastLogSec;         // STREAK_SEC_UNSET if never logged
  uint32_t dayIndex;           // rolling day ordinal; also the bitmap head
  uint32_t bitmap;             // bit i = "read on day (dayIndex - i)"
};

namespace Statistics {

// Cold-boot reload from NVS into RTC-RAM counters + bootstrap streak
// caches. Call once from setup() after `prefs.begin`. No-op on warm
// wake (RTC RAM is still authoritative).
void loadOnBoot();

// Page-turn hook — bumps the lifetime page counter and, after enough
// page turns, offers the session to applyStreakLog from
// src/pure/streak_log.cpp, which decides whether it opens a new streak
// day, merely keeps the current one alive, or lands after a lapse.
// Toasts "Reading streak: day N" on a
// successful log. Call from advancePage() / retreatPage() in
// src/ui/reader.cpp; bookmark-add doesn't go through those, so it
// correctly stays uncounted.
void onReaderPageTurn();

// Bump the lifetime button-press counter by `delta`. No-op when
// `delta == 0`. Auto-flushes alongside page bumps via the shared
// safety-net threshold.
void bumpButtons(uint32_t delta);

// Force a flush of RTC-RAM lifetime counters to NVS. Called from
// Sleep::enter so any in-flight deltas land before power-down. The
// streak portion of the state is already in NVS (logged days write
// straight through), so this only deals with pagesRead /
// buttonPresses / firstStatsRtcSec.
void flushToNvs();

// Read-only snapshot of all the dashboard values for the screen.
StatisticsSnapshot snapshot();

}  // namespace Statistics

#endif  // PALA_STORAGE_STATISTICS_H
