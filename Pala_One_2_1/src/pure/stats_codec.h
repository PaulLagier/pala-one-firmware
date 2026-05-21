#ifndef PALA_PURE_STATS_CODEC_H
#define PALA_PURE_STATS_CODEC_H

#include "src/pure/arduino_compat.h"  // uint32_t, uint64_t, size_t

// ============================================================================
//  Stats wire format + pure encode/decode.
//
//  Lives in pure/ so the schema-version check and round-trip are
//  host-testable independent of LittleFS / RTC RAM. The firmware side
//  (storage/stats.cpp) owns the RTC-RAM working counters and the flash I/O;
//  it converts between the in-memory counters and `StatsFile` via the codec.
//
//  The struct layout is shared with examples/stats/app.c (schema-version
//  gated). MUST stay byte-identical to the app-side definition.
// ============================================================================

static const uint32_t STATS_SCHEMA = 1;

struct StatsFile {
  uint32_t version;        // == STATS_SCHEMA
  uint32_t firstRtcSec;    // rtcSeconds() at first ever write
  uint64_t pagesRead;
  uint64_t buttonPresses;
};

static const size_t STATS_ENCODED_SIZE = sizeof(StatsFile);

// Serialise `s` into `buf`. Caller must ensure `buf` has at least
// STATS_ENCODED_SIZE bytes. Returns the number of bytes written.
size_t encodeStats(const StatsFile& s, uint8_t* buf);

// Parse `len` bytes from `buf` into `out`. Returns true on a valid record
// (schema matches, length exact), false otherwise — caller treats false as
// "reinitialise from zero state".
bool decodeStats(const uint8_t* buf, size_t len, StatsFile& out);

#endif  // PALA_PURE_STATS_CODEC_H
