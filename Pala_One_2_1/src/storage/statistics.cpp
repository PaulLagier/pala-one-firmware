#include "src/storage/statistics.h"

#include "src/pure/streak_log.h"  // ReadingStreakFile + applyStreakLog
#include "src/state.h"            // prefs
#include "src/ui/toast.h"         // Toast::show

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
//
//  RTC RAM: working lifetime counters (survive deep sleep, zero flash writes
//  during normal use). Re-seeded from cfg_stats on cold boot.
//
//  Plain RAM: streak threshold-gate cache (pagesToday + cachedLastDay) —
//  reset on every cold boot, which restarts the 5-page threshold for the
//  next session. Acceptable: page-turn count within one reading session is
//  a small fraction of any realistic streak rule.
// ============================================================================

static constexpr const char* kKeyStats  = "cfg_stats";
static constexpr const char* kKeyStreak = "cfg_streak";

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

static void writeStatsBlob() {
  StatsBlob b;
  b.version       = STATS_SCHEMA;
  b.firstRtcSec   = s_firstStatsRtcSec;
  b.pagesRead     = s_pagesRead;
  b.buttonPresses = s_buttonPresses;
  prefs.putBytes(kKeyStats, &b, sizeof(b));
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
  s_eventsSinceFlush = 0;
  s_rtcInitialised   = true;
}

static inline void bumpEventsAndMaybeFlush() {
  if (++s_eventsSinceFlush >= STATS_FLUSH_EVERY_EVENTS) writeStatsBlob();
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
  writeStatsBlob();
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
  return s;
}

}  // namespace Statistics
