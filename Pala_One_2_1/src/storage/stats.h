#ifndef PALA_STORAGE_STATS_H
#define PALA_STORAGE_STATS_H

#include "src/pure/arduino_compat.h"  // uint32_t

// ============================================================================
//  Stats — lifetime page-turn and button-press counters.
//
//  Working counters live in RTC RAM (survive deep sleep, unlimited writes).
//  Flash flush happens at sleep-entry and every STATS_FLUSH_EVERY_EVENTS
//  bumps as a safety net. Cold-boot reload comes from /apps/stats.dat, which
//  the Stats example app reads to display the totals.
//
//  The wire-format struct lives in stats.cpp and MUST stay byte-identical
//  to the StatsFile in examples/stats/app.c.
// ============================================================================

// Cold-boot reload from /apps/stats.dat. No-op if RTC RAM was preserved
// across a deep-sleep cycle. Call once early in setup().
void statsEnsureLoaded();

// Bump the lifetime page-turn counter by one. Auto-flushes to flash every
// STATS_FLUSH_EVERY_EVENTS bumps.
void statsBumpPages();

// Bump the lifetime button-press counter by `delta`. No-op if delta == 0.
// Auto-flushes alongside page bumps.
void statsBumpButtons(uint32_t delta);

// Force a flush of RTC RAM counters to /apps/stats.dat. Called at deep-sleep
// entry so any in-flight deltas land before power-down.
void statsFlushToFile();

#endif  // PALA_STORAGE_STATS_H
