#ifndef PALA_PURE_STREAK_LOG_H
#define PALA_PURE_STREAK_LOG_H

#include "src/pure/arduino_compat.h"  // uint32_t

// ============================================================================
//  Reading-streak wire format + pure log-advance logic.
//
//  Lives in pure/ so the bitmap-shift + continuation rules are host-testable
//  independent of LittleFS / RTC / Toast. The firmware side (reading_hooks.cpp)
//  reads /apps/streak.dat into one of these structs, calls `applyStreakLog`
//  to compute the next state, and writes it back. The same struct layout is
//  shared with examples/reading_streak/app.c (schema-version gated).
// ============================================================================

static const uint32_t STREAK_SCHEMA          = 1;
static const uint32_t STREAK_DAY_SECS        = 86400u;
static const uint32_t STREAK_DAY_UNSET       = 0xFFFFFFFFu;
static const int      STREAK_PAGES_THRESHOLD = 5;     // pages today before we log

struct ReadingStreakFile {
  uint32_t version;        // == STREAK_SCHEMA
  uint32_t firstRtcSec;    // rtcSeconds() at first ever write
  uint32_t lastLoggedDay;  // day index, STREAK_DAY_UNSET = never
  uint32_t currentStreak;
  uint32_t longestStreak;
  uint32_t totalSessions;
  uint32_t bitmapHead;     // day index of bit 0
  uint32_t bitmap;         // bit i = "logged on day (head - i)"
};

struct StreakLogResult {
  bool                changed;  // false = same-day no-op
  ReadingStreakFile   next;     // valid iff `changed`
};

// Advance the streak state for a session logged on `today`. Pure — same
// `current` + `today` always yield the same result.
//
// Behaviour:
//   - If `today == current.lastLoggedDay` → no-op ({false, current}).
//   - Otherwise:
//     - bitmap shifts left by `(today - bitmapHead)` (zeros if shift >= 32).
//     - bit 0 set (today is logged).
//     - currentStreak = (lastLoggedDay+1 == today) ? prev+1 : 1.
//     - longestStreak = max(longestStreak, currentStreak).
//     - totalSessions++.
//     - lastLoggedDay = today.
//
// `STREAK_DAY_UNSET` as `lastLoggedDay` means "never logged before" and
// breaks continuation (the next log starts streak at 1, not 2).
StreakLogResult applyStreakLog(const ReadingStreakFile& current, uint32_t today);

#endif  // PALA_PURE_STREAK_LOG_H
