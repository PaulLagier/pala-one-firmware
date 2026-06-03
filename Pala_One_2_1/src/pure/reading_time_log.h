#ifndef PALA_PURE_READING_TIME_LOG_H
#define PALA_PURE_READING_TIME_LOG_H

#include "src/pure/arduino_compat.h"  // uint32_t, uint64_t

// ============================================================================
//  Reading-time wire format + pure bucket-advance / view logic.
//
//  Mirrors the streak_log split: the day/week/month/year roll-over rules live
//  here so they're host-testable without RTC, NVS, or the display. The
//  firmware side (storage/statistics.cpp) keeps one of these structs in RTC
//  RAM, feeds it capped per-page reading deltas via `applyReadingTime`, and
//  flushes it to NVS on the same safety-net schedule as the lifetime counters.
//
//  "day" is a day index = (rtcSeconds - firstRtcSec) / 86400, identical to the
//  reading-streak day index. Week/month/year are coarse fixed-length buckets
//  derived from it (day/7, day/30, day/365) — not real calendar boundaries,
//  matching the firmware's RTC-relative time model (no wall clock).
// ============================================================================

static const uint32_t RTIME_SCHEMA         = 1;
static const uint32_t RTIME_DAYS_PER_WEEK   = 7;
static const uint32_t RTIME_DAYS_PER_MONTH  = 30;
static const uint32_t RTIME_DAYS_PER_YEAR   = 365;

// Max seconds credited to a single page-turn gap. Longer gaps are treated as
// "put the book down" (idle / overnight / first turn after deep-sleep wake)
// and dropped entirely rather than clamped, so reading time never counts idle.
static const uint32_t RTIME_GAP_CAP_SECS = 300;  // 5 min/page ceiling

struct ReadingTimeFile {
  uint32_t version;       // == RTIME_SCHEMA
  uint32_t firstRtcSec;   // rtcSeconds() at first init — day-index epoch
  uint64_t totalSeconds;  // lifetime reading seconds

  uint32_t dayIndex;      uint32_t daySeconds;
  uint32_t weekIndex;     uint32_t weekSeconds;
  uint32_t monthIndex;    uint32_t monthSeconds;
  uint32_t yearIndex;     uint32_t yearSeconds;
};

// Add `seconds` of reading logged on day index `day`. Pure — same inputs
// always yield the same result. Each bucket whose index changed is reset to 0
// before the add, so a stored bucket only ever holds the *current*
// day/week/month/year total. `totalSeconds` always accumulates. `seconds == 0`
// is a no-op (returns `cur` unchanged). version / firstRtcSec are preserved.
ReadingTimeFile applyReadingTime(const ReadingTimeFile& cur, uint32_t day, uint32_t seconds);

// Snapshot for display, given the *current* day index `curDay` (derived from
// "now", not from the last logged day). A bucket whose stored index != the
// current index reads as 0 — e.g. nothing read yet today, or the stored
// daySeconds belongs to a day that has since rolled over. Averages divide the
// lifetime total by elapsed buckets since the epoch (curDay+1 days, etc.), so
// they answer "how much do you read per day/week on average".
struct ReadingTimeView {
  uint64_t total;
  uint32_t today;
  uint32_t week;
  uint32_t month;
  uint32_t year;
  uint32_t avgPerDay;
  uint32_t avgPerWeek;
};
ReadingTimeView viewReadingTime(const ReadingTimeFile& f, uint32_t curDay);

#endif  // PALA_PURE_READING_TIME_LOG_H
