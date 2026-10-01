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
//    cfg_streak  blob — ReadingStreakFile (9 x uint32_t, schema v2).
//                       A 32-byte v1 blob left by an older firmware is
//                       migrated in place on first read.
//
//  RTC RAM: working lifetime counters (survive deep sleep, zero flash writes
//  during normal use). Re-seeded from cfg_stats on cold boot.
//
//  Plain RAM: streak gate cache (pagesSinceCheck + lastTouchSec) — reset on
//  every cold boot, which restarts the 5-page threshold for the next session.
//  Acceptable: page-turn count within one reading session is a small fraction
//  of any realistic streak rule.
//
//  Streak v2 tracks `lastReadSec` as well as `lastLogSec`, so unlike v1 it has
//  something to persist on sessions that don't advance the streak. Two gates
//  keep that off the flash: STREAK_PAGES_THRESHOLD page turns before we look
//  at NVS at all, then STREAK_REFRESH_SECS before a bare lastReadSec refresh
//  is worth a write. A logged day always writes through immediately.
// ============================================================================

static constexpr const char* kKeyStats  = "cfg_stats";
static constexpr const char* kKeyStreak = "cfg_streak";

static const uint32_t STATS_SCHEMA             = 1;
static const uint32_t STATS_FLUSH_EVERY_EVENTS = 100;

// Don't spend a flash write refreshing `lastReadSec` more often than this.
// The value only matters within a single sitting — it is well under the 18 h
// same-day floor, so it can never change which day a session lands on.
static const uint32_t STREAK_REFRESH_SECS      = 900;  // 15 min

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

// Plain RAM streak gate cache. Reset on cold boot.
static struct {
  int      pagesSinceCheck = 0;
  uint32_t lastTouchSec    = 0;      // RTC secs when we last consulted NVS
  bool     touched         = false;  // lastTouchSec is meaningful
  bool     bootstrapped    = false;
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

static void writeStreakBlob(const ReadingStreakFile& s) {
  prefs.putBytes(kKeyStreak, &s, sizeof(s));
}

// Read the streak blob, upgrading a schema-v1 one on the way through. The two
// schemas are different sizes (32 vs 36 bytes), so the stored length tells us
// which we're looking at without having to trust the version word first.
static bool readStreakBlob(ReadingStreakFile& out) {
  size_t len = prefs.getBytesLength(kKeyStreak);

  if (len == sizeof(ReadingStreakFile)) {
    size_t got = prefs.getBytes(kKeyStreak, &out, sizeof(out));
    return got == sizeof(out) && out.version == STREAK_SCHEMA;
  }

  if (len == sizeof(ReadingStreakFileV1)) {
    ReadingStreakFileV1 v1{};
    size_t got = prefs.getBytes(kKeyStreak, &v1, sizeof(v1));
    if (got != sizeof(v1) || v1.version != 1u) return false;
    out = migrateStreakV1(v1, rtcSecondsNow());
    writeStreakBlob(out);   // land the upgrade once, not on every read
    return true;
  }

  return false;
}

// One-time initialise the streak NVS key + RAM cache. Called on first
// page-turn after boot (lazy — keeps cold-start fast for non-readers).
static void bootstrapStreak() {
  ReadingStreakFile s{};
  if (!readStreakBlob(s)) {
    s.version       = STREAK_SCHEMA;
    s.firstRtcSec   = rtcSecondsNow();
    s.lastLogSec    = STREAK_SEC_UNSET;
    s.lastReadSec   = 0;
    s.dayIndex      = 0;
    s.currentStreak = 0;
    s.longestStreak = 0;
    s.totalSessions = 0;
    s.bitmap        = 0;
    writeStreakBlob(s);
  }
  g_streakAuto.bootstrapped = true;
}

// Try to advance the streak from the current RTC reading. Keeps the threshold
// gate from the original feature — STREAK_PAGES_THRESHOLD page turns before a
// session counts as reading — and adds a time gate so a long sitting doesn't
// re-read NVS every fifth page. The window rules themselves are in
// src/pure/streak_log.cpp (host-tested).
static void streakAutoLogOnPageTurn() {
  if (!g_streakAuto.bootstrapped) bootstrapStreak();

  if (++g_streakAuto.pagesSinceCheck < STREAK_PAGES_THRESHOLD) return;
  g_streakAuto.pagesSinceCheck = 0;

  uint32_t now = rtcSecondsNow();

  // Nothing the pure logic decides can change faster than the refresh
  // interval, so don't go back to NVS sooner than that. `now >= lastTouchSec`
  // guards a backwards RTC: if the counter reset, fall through and let
  // applyStreakLog re-anchor.
  if (g_streakAuto.touched && now >= g_streakAuto.lastTouchSec &&
      (now - g_streakAuto.lastTouchSec) < STREAK_REFRESH_SECS) {
    return;
  }

  ReadingStreakFile s;
  if (!readStreakBlob(s)) return;

  g_streakAuto.lastTouchSec = now;
  g_streakAuto.touched      = true;

  StreakLogResult r = applyStreakLog(s, now);
  if (!r.changed) return;

  // A bare lastReadSec refresh is worth a flash write only once per interval;
  // a logged day, or a backwards-RTC re-anchor, always writes through.
  const bool refreshDue = (r.next.lastReadSec < s.lastReadSec) ||
                          (r.next.lastReadSec - s.lastReadSec) >= STREAK_REFRESH_SECS;
  if (!r.logged && !refreshDue) return;

  writeStreakBlob(r.next);
  if (!r.logged) return;

  char msg[48];
  snprintf(msg, sizeof(msg), D_TOAST_STREAK_DAY_FMT,
           (unsigned)r.next.currentStreak);
  Toast::show(msg);
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
    s.lastLogSec        = str.lastLogSec;
    s.dayIndex          = str.dayIndex;
    s.bitmap            = str.bitmap;
  } else {
    s.firstStreakRtcSec = 0;
    s.currentStreak     = 0;
    s.longestStreak     = 0;
    s.totalSessions     = 0;
    s.lastLogSec        = STREAK_SEC_UNSET;
    s.dayIndex          = 0;
    s.bitmap            = 0;
  }
  return s;
}

}  // namespace Statistics
