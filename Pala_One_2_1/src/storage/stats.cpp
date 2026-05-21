#include "src/storage/stats.h"

#include "src/state.h"   // FS (=LittleFS)

// esp_rtc_get_time_us lives in a chip-specific header; forward-declaring it
// keeps this file building across chip variants. Same pattern as
// src/ui/pala_api_impl.cpp.
extern "C" uint64_t esp_rtc_get_time_us(void);

// ============================================================================
//  Wire format — MUST stay byte-identical to StatsFile in examples/stats/app.c.
// ============================================================================
struct StatsFile {
  uint32_t version;        // == STATS_SCHEMA
  uint32_t firstRtcSec;    // rtcSeconds() at first ever write
  uint64_t pagesRead;
  uint64_t buttonPresses;
};

static const uint32_t STATS_SCHEMA             = 1;
static const uint32_t STATS_FLUSH_EVERY_EVENTS = 100;

// Working counters live in RTC RAM. Preserved across deep sleep; lost on
// cold boot, which `statsEnsureLoaded` repairs from /apps/stats.dat.
RTC_DATA_ATTR static uint64_t s_pagesRead          = 0;
RTC_DATA_ATTR static uint64_t s_buttonPresses      = 0;
RTC_DATA_ATTR static uint32_t s_firstRtcSec        = 0;
RTC_DATA_ATTR static uint32_t s_eventsSinceFlush   = 0;
RTC_DATA_ATTR static bool     s_rtcInitialised     = false;

static uint32_t rtcSecondsNow() {
  return (uint32_t)(esp_rtc_get_time_us() / 1000000ULL);
}

void statsFlushToFile() {
  StatsFile s;
  s.version       = STATS_SCHEMA;
  s.firstRtcSec   = s_firstRtcSec;
  s.pagesRead     = s_pagesRead;
  s.buttonPresses = s_buttonPresses;

  File f = FS.open("/apps/stats.dat", "w");
  if (!f) return;
  f.write((const uint8_t*)&s, sizeof(s));
  f.close();

  s_eventsSinceFlush = 0;
}

void statsEnsureLoaded() {
  if (s_rtcInitialised) return;

  StatsFile s;
  File f = FS.open("/apps/stats.dat", "r");
  if (f && f.read((uint8_t*)&s, sizeof(s)) == sizeof(s) && s.version == STATS_SCHEMA) {
    s_pagesRead     = s.pagesRead;
    s_buttonPresses = s.buttonPresses;
    s_firstRtcSec   = s.firstRtcSec;
  } else {
    s_pagesRead     = 0;
    s_buttonPresses = 0;
    s_firstRtcSec   = rtcSecondsNow();
  }
  if (f) f.close();

  s_eventsSinceFlush = 0;
  s_rtcInitialised   = true;
}

static inline void bumpEventsAndMaybeFlush() {
  if (++s_eventsSinceFlush >= STATS_FLUSH_EVERY_EVENTS) statsFlushToFile();
}

void statsBumpPages() {
  s_pagesRead++;
  bumpEventsAndMaybeFlush();
}

void statsBumpButtons(uint32_t delta) {
  if (delta == 0) return;
  s_buttonPresses += delta;
  bumpEventsAndMaybeFlush();
}
