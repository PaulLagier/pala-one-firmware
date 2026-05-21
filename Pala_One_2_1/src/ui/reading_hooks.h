#ifndef PALA_UI_READING_HOOKS_H
#define PALA_UI_READING_HOOKS_H

// ============================================================================
//  Reading hooks — firmware-side integration with the reading_streak and
//  sleep_timer example apps.
//
//  Both apps own /apps/<key>.dat files; the firmware reads/writes the same
//  files so the features keep working when the user is in the book rather
//  than in the app:
//
//    - onReaderPageTurn(): called on every real page turn (next/prev) from
//      the reader and bookmark-preview screens. Auto-logs the reading streak
//      after STREAK_PAGES_THRESHOLD page turns on a new day, and flips a
//      RUNNING sleep timer to NEEDS_NOTIFY when its end-time passes.
//
//    - sleepTimerCheckExpired(): called once from setup() so a timer that
//      expired during deep sleep surfaces its toast on the first render
//      after wake.
//
//    - sleepTimerWakeUs(): returns microseconds until a running timer
//      expires, or 0 if no timer is running or it has already expired.
//      Used by Sleep::enter to arm an RTC wake alarm.
// ============================================================================

#include "src/pure/arduino_compat.h"  // uint32_t, uint64_t

void     onReaderPageTurn();
void     sleepTimerCheckExpired();
uint64_t sleepTimerWakeUs();

#endif  // PALA_UI_READING_HOOKS_H
