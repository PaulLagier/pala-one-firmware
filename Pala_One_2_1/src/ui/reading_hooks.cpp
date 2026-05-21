#include "src/ui/reading_hooks.h"

#include "src/state.h"      // FS (=LittleFS), prefs unused here but matches sibling modules
#include "src/ui/toast.h"   // Toast::show

// esp_rtc_get_time_us lives in a chip-specific header; forward-declaring it
// keeps this file building across chip variants. Same pattern as
// src/ui/pala_api_impl.cpp.
extern "C" uint64_t esp_rtc_get_time_us(void);

// ============================================================================
//  Wire-format structs — MUST match the SavedState / SleepTimerFile layouts
//  in examples/reading_streak/app.c and examples/sleep_timer/app.c. Field
//  order and types are the on-disk format; the schema-version field gates
//  upgrades.
// ============================================================================

struct ReadingStreakFile {
  uint32_t version;        // == STREAK_SCHEMA
  uint32_t firstRtcSec;    // rtcSeconds() at first ever write
  uint32_t lastLoggedDay;  // day index, 0xFFFFFFFFu = never
  uint32_t currentStreak;
  uint32_t longestStreak;
  uint32_t totalSessions;
  uint32_t bitmapHead;     // day index of bit 0
  uint32_t bitmap;         // bit i = "logged on day (head - i)"
};

struct SleepTimerFile {
  uint32_t version;        // == SLEEP_TIMER_SCHEMA
  uint32_t status;         // SLEEP_TIMER_IDLE / RUNNING / NEEDS_NOTIFY
  uint32_t endRtcSec;
  uint32_t durationMin;
  uint32_t startedRtcSec;
};

static const uint32_t STREAK_SCHEMA          = 1;
static const uint32_t STREAK_DAY_SECS        = 86400u;
static const int      STREAK_PAGES_THRESHOLD = 5;     // pages today before we log

static const uint32_t SLEEP_TIMER_SCHEMA       = 1;
static const uint32_t SLEEP_TIMER_IDLE         = 0;
static const uint32_t SLEEP_TIMER_RUNNING      = 1;
static const uint32_t SLEEP_TIMER_NEEDS_NOTIFY = 2;

static struct {
  int      pagesToday        = 0;
  uint32_t cachedFirstRtcSec = 0;
  uint32_t cachedLastDay     = 0xFFFFFFFFu;
  bool     bootstrapped      = false;
} g_streakAuto;

// Same source of truth that the apps see via PalaAPI::rtcSeconds — RTC
// microsecond counter scaled to seconds. Survives deep sleep.
static uint32_t rtcSecondsNow() {
  return (uint32_t)(esp_rtc_get_time_us() / 1000000ULL);
}

static bool readAppDat(const char* key, void* buf, size_t expected) {
  String path = String("/apps/") + key + ".dat";
  File f = FS.open(path, "r");
  if (!f) return false;
  size_t n = f.read((uint8_t*)buf, expected);
  f.close();
  return n == expected;
}

static void writeAppDat(const char* key, const void* buf, size_t len) {
  String path = String("/apps/") + key + ".dat";
  File f = FS.open(path, "w");
  if (!f) return;
  f.write((const uint8_t*)buf, len);
  f.close();
}

// Increment the streak after enough page turns on a new day. Mirrors the
// log logic in examples/reading_streak/app.c so manual taps and auto-log
// produce the same numbers.
static void streakAutoLogOnPageTurn() {
  if (!g_streakAuto.bootstrapped) {
    ReadingStreakFile s;
    if (!readAppDat("streak", &s, sizeof(s)) || s.version != STREAK_SCHEMA) {
      s.version       = STREAK_SCHEMA;
      s.firstRtcSec   = rtcSecondsNow();
      s.lastLoggedDay = 0xFFFFFFFFu;
      s.currentStreak = 0;
      s.longestStreak = 0;
      s.totalSessions = 0;
      s.bitmapHead    = 0;
      s.bitmap        = 0;
      writeAppDat("streak", &s, sizeof(s));
    }
    g_streakAuto.cachedFirstRtcSec = s.firstRtcSec;
    g_streakAuto.cachedLastDay     = s.lastLoggedDay;
    g_streakAuto.bootstrapped      = true;
  }

  uint32_t now = rtcSecondsNow();
  if (now < g_streakAuto.cachedFirstRtcSec) return;           // RTC ran backwards
  uint32_t today = (now - g_streakAuto.cachedFirstRtcSec) / STREAK_DAY_SECS;

  if (g_streakAuto.cachedLastDay == today) return;            // already logged today
  if (++g_streakAuto.pagesToday < STREAK_PAGES_THRESHOLD) return;

  ReadingStreakFile s;
  if (!readAppDat("streak", &s, sizeof(s)) || s.version != STREAK_SCHEMA) return;

  bool cont = (s.lastLoggedDay != 0xFFFFFFFFu) && (today == s.lastLoggedDay + 1);
  if (today > s.bitmapHead) {
    uint32_t d = today - s.bitmapHead;
    s.bitmap     = (d >= 32) ? 0 : (s.bitmap << d);
    s.bitmapHead = today;
  }
  s.bitmap        |= 1u;
  s.currentStreak  = cont ? (s.currentStreak + 1) : 1;
  if (s.currentStreak > s.longestStreak) s.longestStreak = s.currentStreak;
  s.lastLoggedDay  = today;
  s.totalSessions += 1;
  writeAppDat("streak", &s, sizeof(s));

  g_streakAuto.cachedLastDay = today;
  g_streakAuto.pagesToday    = 0;

  Toast::show(String("Reading streak: day ") + String(s.currentStreak));
}

// Flip RUNNING -> NEEDS_NOTIFY exactly once when the timer first expires,
// and toast at that moment. Subsequent page turns are quiet — the
// sleep_timer app's own UI is the persistent indicator (and it clears
// the file back to IDLE on acknowledge).
void sleepTimerCheckExpired() {
  SleepTimerFile s;
  if (!readAppDat("sleep_timer", &s, sizeof(s)) || s.version != SLEEP_TIMER_SCHEMA) return;
  if (s.status != SLEEP_TIMER_RUNNING) return;
  if (rtcSecondsNow() < s.endRtcSec)   return;

  s.status = SLEEP_TIMER_NEEDS_NOTIFY;
  writeAppDat("sleep_timer", &s, sizeof(s));
  Toast::show(String("Sleep timer up (") + String(s.durationMin) + " min)");
}

void onReaderPageTurn() {
  streakAutoLogOnPageTurn();
  sleepTimerCheckExpired();
}

// Microseconds until a running sleep timer expires, or 0 if no timer is
// running / it has already passed. Sleep::enter uses this to arm an RTC
// wake alarm so the device wakes itself at the timer's end.
uint64_t sleepTimerWakeUs() {
  SleepTimerFile s;
  if (!readAppDat("sleep_timer", &s, sizeof(s))) return 0;
  if (s.version != SLEEP_TIMER_SCHEMA)           return 0;
  if (s.status  != SLEEP_TIMER_RUNNING)          return 0;
  uint32_t now = rtcSecondsNow();
  if (s.endRtcSec <= now)                        return 0;
  return (uint64_t)(s.endRtcSec - now) * 1000000ULL;
}
