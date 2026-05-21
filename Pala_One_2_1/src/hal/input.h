#ifndef PALA_HAL_INPUT_H
#define PALA_HAL_INPUT_H

#include <Arduino.h>

#include "src/config.h"

// ============================================================================
//  Button-input subsystem
//
//  Owns the GPIO-button ISR, the edge-event ring buffer, the debounced
//  click-classification state machine, and the user-activity timer used by
//  the deep-sleep deadline. The hardware-adjacent half of input lives here;
//  the rest of the firmware reads results through `g_btns` and the few
//  free functions declared below.
//
//  See input.cpp for the implementation overview + walked-through examples.
// ============================================================================

// Internal state machine + per-tick output flags for the click classifier.
// `poll()` (called from the main loop) consumes the ISR queue and updates
// the output flags. The flags are valid for exactly one loop iteration —
// `resetClicks()` clears them at the top of each `poll()`.
struct ButtonState {
  // Internal state machine — only `poll()` should be touching these.
  bool stablePressed_ = false;
  uint32_t lastStableChange_ = 0;
  uint32_t pressStart_ = 0;
  bool pressArmed_ = false;
  uint32_t lastRelease_ = 0;
  uint32_t firstClickRelease_ = 0;
  uint8_t clickCount_ = 0;

  // Output flags — set by `poll()`, read by callers (`ButtonEvent::fromButtonState`).
  // Cleared at the top of every `poll()`, so they're valid for exactly one
  // loop iteration after they're set.
  bool shortClick_ = false;
  bool doubleClick_ = false;
  bool tripleClick_ = false;
  bool quadClick_ = false;
  bool longClick_ = false;

  // Count of accepted short press-release pairs since the last
  // `consumePressCount()` call. Bypasses multi-click grouping — every
  // release that the classifier accepted (long-press releases excluded)
  // bumps this counter. Surfaced for the Pala apps API; firmware code
  // uses the click flags above instead.
  uint32_t rawPressCount_ = 0;

  void resetClicks() {
    shortClick_ = false;
    doubleClick_ = false;
    tripleClick_ = false;
    quadClick_ = false;
    longClick_ = false;
  }

  void resetState() {
    stablePressed_ = false;
    lastStableChange_ = 0;
    pressStart_ = 0;
    pressArmed_ = false;
    lastRelease_ = 0;
    firstClickRelease_ = 0;
    clickCount_ = 0;
    rawPressCount_ = 0;
    resetClicks();
  }

  // Apps-API accessors. Hidden from firmware-side callers (the click
  // flags + `ButtonEvent::fromButtonState` are richer); these exist so
  // the PalaAPI shim has a stable, non-poking surface to wrap.

  // Is the button physically held down right now? Reads the debounced
  // state; matches the old `api_buttonPressed`.
  bool isPressed() const { return stablePressed_; }

  // Returns the accumulated raw short-press count and resets it to zero
  // atomically (from the caller's view — `poll()` doesn't run during
  // this call). Matches the old `api_pendingPresses`.
  uint32_t consumePressCount() {
    uint32_t n = rawPressCount_;
    rawPressCount_ = 0;
    return n;
  }

  // Read the accumulated raw short-press count without resetting it. Used
  // by the lifetime-stats counter in `loop()`, which diffs against its
  // last-seen value each tick. Independent of `consumePressCount` — the
  // apps API can still consume + reset; stats clamps `lastSeen` to the
  // current value on the next tick to recover.
  uint32_t peekPressCount() const { return rawPressCount_; }

  bool anyClick() const {
    return shortClick_ || doubleClick_ || tripleClick_ || quadClick_ || longClick_;
  }

  // True iff the classifier is mid-sequence — at least one release has been
  // accepted but not yet committed (waiting on trailing silence to decide
  // short vs double vs triple). Light-sleep uses this to pick a shorter
  // timer-wake interval so the trailing-silence check fires in time.
  bool hasPendingClicks() const { return clickCount_ > 0; }

  void poll();
};

extern ButtonState g_btns;

// Screen-facing collapsed view of `ButtonState`'s output flags. The screen
// dispatcher calls `fromButtonState(g_btns)` once per loop and hands the
// result to the current screen.
struct ButtonEvent {
  enum Kind { None, Short, Double, Triple, Quad, Long } kind = None;

  static ButtonEvent fromButtonState(const ButtonState& b) {
    ButtonEvent e;
    if (b.longClick_)        e.kind = Long;
    else if (b.quadClick_)   e.kind = Quad;
    else if (b.tripleClick_) e.kind = Triple;
    else if (b.doubleClick_) e.kind = Double;
    else if (b.shortClick_)  e.kind = Short;
    return e;
  }

  bool any() const { return kind != None; }
};

// ----------------------------------------------------------------------------
//  Lifecycle / interrupt plumbing
// ----------------------------------------------------------------------------
void btnISR();  // IRAM_ATTR is on the definition only — repeating it here
                // makes GCC place the decl and defn in differently-suffixed
                // .iram1.* sections, triggering an attribute-conflict warning.
void clearButtonQueue();
void resetInputFrontend();

// Synthesize an edge into the ISR queue from a normal (non-IRQ) context.
// Used after light-sleep GPIO wake: the ext0 wakeup path detects the level
// but doesn't necessarily fire the edge ISR, so the press that woke us
// would otherwise be invisible to the classifier. Safe to call even if the
// real ISR also fires — debounce naturally suppresses duplicates.
void injectButtonEdgeNow(bool pressed);

// True iff the ISR ring buffer has un-drained edges. Used by the main loop's
// light-sleep gate to close the race where an edge is queued by the ISR
// between the most recent `poll()` and the gate check itself: without this,
// `hasPendingClicks()` would still be false (the release isn't reflected in
// `clickCount_` until poll runs) and we'd sleep, missing the release until
// the timer wake fires. Atomic single-byte read; no lock needed.
bool buttonQueueNonEmpty();

// Inspect the ISR-overflow counter; if it's climbed past the recovery
// threshold, atomically reset it and resync the button state machine.
// Called once per loop iteration after polling. Returns true if a recovery
// happened (purely informational; main loop ignores the result).
bool maybeRecoverFromIsrOverflow();

// ----------------------------------------------------------------------------
//  User-activity timer (drives the deep-sleep deadline)
// ----------------------------------------------------------------------------
void markUserActivity();

// Milliseconds since the most recent accepted user input. The sleep
// deadline check in main.cpp compares this against Sleep::idleTimeoutMs().
uint32_t userIdleMs();

// Block the calling task until the classifier emits any click event,
// then return it. Drives the apps API's `waitForEvent`; firmware code
// stays event-driven via the main loop and does not call this.
//
// Implementation: a tight `poll()` + 1ms-delay loop, same shape as
// `loop()` itself except it doesn't dispatch screens or gate sleep.
// While this blocks, the main loop is paused — sleep timeouts, screen
// redraws, and ISR-overflow recovery are all on hold until the user
// presses something. That's intentional: apps are short-lived foreground
// work and own the device for their duration.
//
// Marks user activity on both entry and exit so the deep-sleep deadline
// is fresh whenever the app returns to the host.
ButtonEvent waitForNextEvent();

#endif  // PALA_HAL_INPUT_H
