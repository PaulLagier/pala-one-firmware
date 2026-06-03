#include "src/storage/statistics.h"

#include "src/pure/streak_log.h"        // ReadingStreakFile + applyStreakLog
#include "src/pure/reading_time_log.h"  // ReadingTimeFile + applyReadingTime
#include "src/state.h"                  // prefs
#include "src/ui/toast.h"               // Toast::show

// esp_rtc_get_time_us lives in a chip-specific header; forward-declaring it
// keeps this file building across chip variants. Same pattern as
// src/ui/pala_api_impl.cpp.
extern "C" uint64_t esp_rtc_get_time_us(void);

// ============================================================================
//  Storage layout
//
//  NVS:
//    cfg_stats   blob — { uint32_t version; uint32_t firstRtcSec;
//                         uint64_t pagesRead; uint64_t buttonPresses; }
//    cfg_streak  blob — ReadingStreakFile (8 x uint32_t)
//    cfg_rtime   blob — ReadingTimeFile (reading-time buckets + total)
//
//  RTC RAM: working lifetime counters + reading-time buckets (survive deep
//  sleep, zero flash writes during normal use). Re-seeded from cfg_stats /
//  cfg_rtime on cold boot. Reading time is bucketed per page turn into RTC RAM
//  and flushed on the same safety-net schedule as the lifetime counters, so
//  frequent page turns never hit flash.
//
//  Plain RAM: streak threshold-gate cache (pagesToday + cachedLastDay) —
//  reset on every cold boot, which restarts the 5-page threshold for the
//  next session. Acceptable: page-turn count within one reading session is
//  a small fraction of any realistic streak rule.
// ============================================================================

static constexpr const char* kKeyStats  = "cfg_stats";
static constexpr const char* kKeyStreak = "cfg_streak";
static constexpr const char* kKeyRtime  = "cfg_rtime";

static const uint32_t STATS_SCHEMA            = 1;
static const uint32_t STATS_FLUSH_EVERY_EVENTS = 100;

struct StatsBlob {
  uint32_t version;
  uint32_t firstRtcSec;
  uint64_t pagesRead;
  uint64_t buttonPresses;
};

// RTC-RAM working counters. Preserved across deep sleep; lost on power loss.
RTC_DATA_ATTR static uint64_t s_pagesRead          = 0;
RTC_DATA_ATTR static uint64_t s_buttonPresses      = 0;
RTC_DATA_ATTR static uint32_t s_firstStatsRtcSec   = 0;
RTC_DATA_ATTR static uint32_t s_eventsSinceFlush   = 0;
RTC_DATA_ATTR static bool     s_rtcInitialised     = false;

// RTC-RAM reading-time accumulator. Working copy of the cfg_rtime blob; the
// pure bucket rules live in pure/reading_time_log.cpp. s_lastReadMarkRtcSec is
// the RTC second of the previous reading mark — the gap to "now" on the next
// page turn is the time spent on the page just read. Both survive deep sleep
// (a stale overnight mark is dropped by the RTIME_GAP_CAP_SECS guard).
RTC_DATA_ATTR static ReadingTimeFile s_rtime            = {};
RTC_DATA_ATTR static uint32_t        s_lastReadMarkRtcSec = 0;

// Plain RAM streak threshold-gate cache. Reset on cold boot.
static struct {
  int      pagesToday        = 0;
  uint32_t cachedFirstRtcSec = 0;
  uint32_t cachedLastDay     = STREAK_DAY_UNSET;
  bool     bootstrapped      = false;
} g_streakAuto;

static uint32_t rtcSecondsNow() {
  return (uint32_t)(esp_rtc_get_time_us() / 1000000ULL);
}

// ---------------------------------------------------------------------------
//  Stats half
// ---------------------------------------------------------------------------

static void writeRtimeBlob() {
  prefs.putBytes(kKeyRtime, &s_rtime, sizeof(s_rtime));
}

// Flush both RTC-RAM blobs (lifetime counters + reading-time buckets) to NVS
// and reset the safety-net counter. They share one flush cadence because both
// are bumped by the same page-turn / button events.
static void flushAll() {
  StatsBlob b;
  b.version       = STATS_SCHEMA;
  b.firstRtcSec   = s_firstStatsRtcSec;
  b.pagesRead     = s_pagesRead;
  b.buttonPresses = s_buttonPresses;
  prefs.putBytes(kKeyStats, &b, sizeof(b));
  writeRtimeBlob();
  s_eventsSinceFlush = 0;
}

static void loadStatsFromNvs() {
  if (s_rtcInitialised) return;

  StatsBlob b{};
  size_t got = prefs.getBytes(kKeyStats, &b, sizeof(b));
  if (got == sizeof(b) && b.version == STATS_SCHEMA) {
    s_pagesRead       = b.pagesRead;
    s_buttonPresses   = b.buttonPresses;
    s_firstStatsRtcSec = b.firstRtcSec;
  } else {
    s_pagesRead       = 0;
    s_buttonPresses   = 0;
    s_firstStatsRtcSec = rtcSecondsNow();
  }

  ReadingTimeFile rf{};
  size_t gotR = prefs.getBytes(kKeyRtime, &rf, sizeof(rf));
  if (gotR == sizeof(rf) && rf.version == RTIME_SCHEMA) {
    s_rtime = rf;
  } else {
    s_rtime             = ReadingTimeFile{};
    s_rtime.version     = RTIME_SCHEMA;
    s_rtime.firstRtcSec = rtcSecondsNow();
  }
  // Cold boot starts a fresh reading session — no mark carries over.
  s_lastReadMarkRtcSec = 0;

  s_eventsSinceFlush = 0;
  s_rtcInitialised   = true;
}

// Day index for "now", matching the streak day index. Clamps to 0 if the RTC
// ran backwards (e.g. firstRtcSec persisted across a power loss that reset the
// RTC timer) so we never underflow the unsigned subtraction.
static uint32_t rtimeDayNow() {
  uint32_t now = rtcSecondsNow();
  if (now < s_rtime.firstRtcSec) return 0;
  return (now - s_rtime.firstRtcSec) / STREAK_DAY_SECS;
}

// Credit the time spent on the page just turned away from. The gap from the
// previous mark to now is real reading time; gaps over the cap are idle / a
// post-wake first turn and are dropped. Buckets live in RTC RAM — persistence
// rides the shared flush cadence, so this stays flash-free.
static void accrueReadingTimeOnPageTurn() {
  uint32_t now = rtcSecondsNow();
  if (s_lastReadMarkRtcSec != 0 && now >= s_lastReadMarkRtcSec) {
    uint32_t delta = now - s_lastReadMarkRtcSec;
    if (delta > 0 && delta <= RTIME_GAP_CAP_SECS) {
      s_rtime = applyReadingTime(s_rtime, rtimeDayNow(), delta);
    }
  }
  s_lastReadMarkRtcSec = now;
}

static inline void bumpEventsAndMaybeFlush() {
  if (++s_eventsSinceFlush >= STATS_FLUSH_EVERY_EVENTS) flushAll();
}

// ---------------------------------------------------------------------------
//  Streak half
// ---------------------------------------------------------------------------

static bool readStreakBlob(ReadingStreakFile& out) {
  size_t got = prefs.getBytes(kKeyStreak, &out, sizeof(out));
  return got == sizeof(out) && out.version == STREAK_SCHEMA;
}

static void writeStreakBlob(const ReadingStreakFile& s) {
  prefs.putBytes(kKeyStreak, &s, sizeof(s));
}

// One-time initialise the streak NVS key + RAM cache. Called on first
// page-turn after boot (lazy — keeps cold-start fast for non-readers).
static void bootstrapStreak() {
  ReadingStreakFile s{};
  if (!readStreakBlob(s)) {
    s.version       = STREAK_SCHEMA;
    s.firstRtcSec   = rtcSecondsNow();
    s.lastLoggedDay = STREAK_DAY_UNSET;
    s.currentStreak = 0;
    s.longestStreak = 0;
    s.totalSessions = 0;
    s.bitmapHead    = 0;
    s.bitmap        = 0;
    writeStreakBlob(s);
  }
  g_streakAuto.cachedFirstRtcSec = s.firstRtcSec;
  g_streakAuto.cachedLastDay     = s.lastLoggedDay;
  g_streakAuto.bootstrapped      = true;
}

// Try to advance the streak based on today's RTC. Mirrors the threshold
// gate from the original reading-streak feature: STREAK_PAGES_THRESHOLD
// page turns on a new day before we commit. The actual continuation +
// bitmap-shift rules are in src/pure/streak_log.cpp (host-tested).
static void streakAutoLogOnPageTurn() {
  if (!g_streakAuto.bootstrapped) bootstrapStreak();

  uint32_t now = rtcSecondsNow();
  if (now < g_streakAuto.cachedFirstRtcSec) return;           // RTC ran backwards
  uint32_t today = (now - g_streakAuto.cachedFirstRtcSec) / STREAK_DAY_SECS;

  if (g_streakAuto.cachedLastDay == today) return;            // already logged today
  if (++g_streakAuto.pagesToday < STREAK_PAGES_THRESHOLD) return;

  ReadingStreakFile s;
  if (!readStreakBlob(s)) return;

  StreakLogResult r = applyStreakLog(s, today);
  if (!r.changed) return;

  writeStreakBlob(r.next);
  g_streakAuto.cachedLastDay = today;
  g_streakAuto.pagesToday    = 0;

  Toast::show(String("Reading streak: day ") + String(r.next.currentStreak));
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------

namespace Statistics {

void loadOnBoot() {
  loadStatsFromNvs();
  // Streak bootstrap is lazy — first page turn handles it. Keeps setup()
  // fast for users who never read.
}

void onReaderPageTurn() {
  loadStatsFromNvs();  // safety in case setup() didn't call us
  accrueReadingTimeOnPageTurn();
  s_pagesRead++;
  bumpEventsAndMaybeFlush();
  streakAutoLogOnPageTurn();
}

void bumpButtons(uint32_t delta) {
  if (delta == 0) return;
  loadStatsFromNvs();
  s_buttonPresses += delta;
  bumpEventsAndMaybeFlush();
}

void flushToNvs() {
  if (!s_rtcInitialised) return;
  flushAll();
}

StatisticsSnapshot snapshot() {
  loadStatsFromNvs();

  StatisticsSnapshot s{};
  s.pagesRead         = s_pagesRead;
  s.buttonPresses     = s_buttonPresses;
  s.firstStatsRtcSec  = s_firstStatsRtcSec;

  ReadingStreakFile str{};
  if (readStreakBlob(str)) {
    s.firstStreakRtcSec = str.firstRtcSec;
    s.currentStreak     = str.currentStreak;
    s.longestStreak     = str.longestStreak;
    s.totalSessions     = str.totalSessions;
    s.lastLoggedDay     = str.lastLoggedDay;
    s.bitmapHead        = str.bitmapHead;
    s.bitmap            = str.bitmap;
  } else {
    s.firstStreakRtcSec = 0;
    s.currentStreak     = 0;
    s.longestStreak     = 0;
    s.totalSessions     = 0;
    s.lastLoggedDay     = STREAK_DAY_UNSET;
    s.bitmapHead        = 0;
    s.bitmap            = 0;
  }

  ReadingTimeView v = viewReadingTime(s_rtime, rtimeDayNow());
  s.totalReadSecs      = v.total;
  s.todayReadSecs      = v.today;
  s.weekReadSecs       = v.week;
  s.monthReadSecs      = v.month;
  s.yearReadSecs       = v.year;
  s.avgPerDayReadSecs  = v.avgPerDay;
  s.avgPerWeekReadSecs = v.avgPerWeek;
  return s;
}

}  // namespace Statistics
