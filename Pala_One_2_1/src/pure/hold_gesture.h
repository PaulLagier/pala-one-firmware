#ifndef PALA_PURE_HOLD_GESTURE_H
#define PALA_PURE_HOLD_GESTURE_H

#include "src/pure/arduino_compat.h"  // uint32_t, uint8_t

#include "src/config.h"  // kDefaultLongMs, kDefaultVeryLongMs — compile-time defaults.
#include "src/storage/click_timings.h"  // runtime thresholds used by the classifier.

// ============================================================================
//  Pure hold-gesture classifier.
//
//  Three remappable hold-flavoured gestures fire from the input classifier:
//
//    Long       — solo hold, >= kDefaultLongMs, < kDefaultVeryLongMs, no prior click.
//    VeryLong   — solo hold, >= kDefaultVeryLongMs, no prior click.
//    ClickHold  — short click immediately followed by a long hold (a chord).
//
//  Two decision points consume those rules:
//    - On release (input.cpp's drain loop): the press just ended; we know
//      its exact duration. Classify and emit.
//    - In-progress (input.cpp's hold-detection block): the press is still
//      down; emit at the earliest threshold the gesture can be unambiguously
//      identified, then consume the press so the eventual release is
//      ignored.
//
//  Both rules are pure functions of (duration_ms, click_count). Living in
//  pure/ lets the rules be host-tested without the ISR ring buffer, the
//  edge debounce, or millis() running.
// ============================================================================

enum HoldGestureKind {
  HoldNone     = 0,
  HoldLong     = 1,
  HoldVeryLong = 2,
  HoldClickHold = 3,
};

// Release-time classification — called when a press ends with `dur_ms`
// elapsed since the down-edge. Caller has already established the release
// was the end of an armed press AND `dur_ms >= kDefaultLongMs` (durations below
// kDefaultLongMs are click-accumulation, not hold).
//
//   click_count == 1, any dur_ms        → ClickHold  (chord — pending single
//                                                     is consumed by the chord)
//   click_count == 0, dur_ms >= kDefaultVeryLongMs → VeryLong
//   click_count == 0, kDefaultLongMs <= dur < kDefaultVeryLongMs → Long
//   otherwise (click_count >= 2)        → None       (multi-click followed by
//                                                     a hold isn't a defined
//                                                     gesture; the pending
//                                                     clicks commit on their
//                                                     own and the hold is
//                                                     dropped)
HoldGestureKind classifyHoldRelease(uint32_t dur_ms, uint8_t click_count);

// In-progress classification — called every poll while the button is still
// down. Returns the gesture we can emit AT THIS INSTANT, given how long the
// press has been held. Plain Long is intentionally deferred to release (at
// kDefaultLongMs we can't yet tell whether the user is going on to a VeryLong).
//
//   click_count == 1, held_ms >= kDefaultLongMs → ClickHold  (chord crosses threshold)
//   click_count == 0, held_ms >= kDefaultVeryLongMs → VeryLong
//   otherwise                             → None        (keep waiting)
HoldGestureKind classifyHoldInProgress(uint32_t held_ms, uint8_t click_count);

#endif  // PALA_PURE_HOLD_GESTURE_H
