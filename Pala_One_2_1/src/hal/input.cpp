#include "src/hal/input.h"

#include <esp_timer.h>

#include "src/pure/hold_gesture.h"  // classifyHoldRelease / classifyHoldInProgress
#include "src/storage/click_timings.h"

// Local helper: map a HoldGestureKind decision into the matching output
// flag. Used at both classification sites (release path + in-progress).
// Returns true if any flag was set — caller uses that to decide whether
// to consume the press (clear pressArmed_) and zero clickCount_.
static bool applyHoldKindTo(ButtonState& s, HoldGestureKind k) {
  switch (k) {
    case HoldLong:      s.longClick_      = true; return true;
    case HoldVeryLong:  s.veryLongClick_  = true; return true;
    case HoldClickHold: s.clickHoldClick_ = true; return true;
    case HoldNone:
    default:            return false;
  }
}

ButtonState g_btns;

// Time of the last accepted user input. Used by the loop to decide when
// to drop the device into deep sleep. main.cpp reads via the `userIdleMs()`
// accessor below, never the raw timestamp.
static uint32_t s_lastUserActionMs = 0;

// ============================================================================
//  Button ISR queue — implementation detail of the input frontend.
//  Lives here (not in input.h) because nothing outside this translation unit
//  needs to see the type or the instance.
// ============================================================================
static const uint8_t BTN_Q = 64;
static const uint32_t BTN_QUEUE_RECOVER_THRESHOLD = 10;

struct ButtonQueue {
  volatile uint8_t head = 0;          // producer cursor
  volatile uint8_t tail = 0;          // consumer cursor
  volatile bool state[BTN_Q];          // per-slot pressed/released
  volatile uint32_t timeMs[BTN_Q];     // per-slot edge timestamp
  volatile uint32_t isrDropCount = 0;  // # of old entries dropped on overrun
};
static ButtonQueue s_btnQ;

// ============================================================================
//  Input frontend — overview
//
//  The single hardware button feeds a producer/consumer pipeline:
//
//      btnISR()        -->  ring buffer  -->  ButtonState::poll()
//      (producer, IRQ)      (s_btnQ, 64 slots)  (consumer, called from loop)
//
//  Why a queue at all?
//    The ISR runs on every electrical edge — within microseconds. The
//    consumer only runs from `loop()`, which can block 200-300ms during an
//    e-ink redraw. Without buffering, edges that fall inside a redraw are
//    lost. With buffering, the ISR captures WHEN (uint32_t ms via
//    esp_timer_get_time/1000) and WHAT (pressed=true / released=false) and
//    returns; the consumer interprets them when it can.
//
//  Ring buffer (s_btnQ, declared just above):
//    s_btnQ.state[BTN_Q]    parallel slot arrays. one slot = (state, timeMs).
//    s_btnQ.timeMs[BTN_Q]   BTN_Q = 64 — plenty for any realistic click burst.
//    s_btnQ.head            producer cursor. ISR writes here, then advances.
//    s_btnQ.tail            consumer cursor. poll() reads here, then advances.
//    empty:  head == tail
//    full:   (head + 1) % BTN_Q == tail
//    s_btnQ.isrDropCount      bumped when the ISR overruns tail (see below).
//
//  Atomicity:
//    head/tail are single-byte volatiles; on ESP32-S3 a byte write is
//    atomic, so the ISR can advance head without a lock and the consumer
//    can snapshot head without a lock. The consumer DOES wrap "read slot +
//    advance tail" in noInterrupts() so it sees a coherent (state, time,
//    tail) trio even if the ISR fires mid-drain.
//
//  Overflow policy:
//    If the buffer is full when the ISR fires, the ISR drops the NEW edge
//    (bumps s_btnQ.isrDropCount, returns without advancing head). Tail
//    belongs to ButtonState::poll() and the ISR must not touch it — moving
//    tail from an interrupt would race poll()'s read-then-advance and
//    either deliver events out of order or corrupt the ring's invariants.
//    The main loop watches s_btnQ.isrDropCount; once it crosses
//    BTN_QUEUE_RECOVER_THRESHOLD (= 10) it calls clearButtonQueue() +
//    g_btns.resetState() on the theory that something is wrong (stuck pin,
//    bouncy contact) and the current click sequence is junk anyway.
//    Dropping NEW edges is less ideal than dropping OLD ones (the user
//    loses the most recent input), but the only safe place to drop old
//    edges from is the consumer side; doing it from the producer was the
//    race this fix removes.
//
//  IRAM_ATTR on btnISR: required so the ISR can execute even while the
//  flash cache is invalidated (e.g., during a LittleFS write).
//
//  Why store edge TIMES, not just sequence?
//    Debounce and click classification both reason about ELAPSED ms between
//    edges. If poll() is delayed by a long redraw, the original edge times
//    survive in the queue — we don't confuse "edges happened close together"
//    with "we got around to handling them close together".
//
//  Walked-through example — clean short click around t = 1000:
//
//    t=1000: press.    ISR queues (true,  1000),  head++
//    t=1080: release.  ISR queues (false, 1080),  head++
//    t~1090: loop() -> poll():
//      snapshot head, drain (tail..head):
//        (true, 1000):  debounce OK, transition -> pressStart=1000, armed
//        (false, 1080): debounce OK, transition -> dur=80 < kDefaultLongMs,
//                                                  clickCount=1, lastRelease=1080
//      trailing-silence check: now-lastRelease = 1090-1080 = 10 < kDefaultGapMs.
//      Do NOT emit yet — could still become a double.
//    t=1100..1400: poll() called each loop, queue empty, still waiting.
//    t~1400: poll(): now-lastRelease = 1400-1080 = 320 > kDefaultGapMs.
//            -> shortClick = true for ONE poll, consumed by the screen.
//
//  Walked-through example — bouncy release:
//
//    t=1000: press.                      ISR: (true,  1000)
//    t=1080: real release.               ISR: (false, 1080)
//    t=1083: contact bounces closed.     ISR: (true,  1083)
//    t=1087: bounce settles open.        ISR: (false, 1087)
//    poll() drains:
//      (true,  1000): accept            -> stablePressed=true, pressStart=1000
//      (false, 1080): 1080-1000=80 > kDefaultDebounceMs=14, accept release
//      (true,  1083): 1083-1080=3   <= kDefaultDebounceMs, reject
//      (false, 1087): 1087-1080=7   <= kDefaultDebounceMs, reject
//    Result: clean clickCount = 1, no spurious double-click.
//
//  Walked-through example — ISR overflow during a long redraw:
//
//    Reader full-refresh page render takes ~300ms. User panic-mashes the
//    button 70 times while it's drawing.
//      Edges 1..63 fill the buffer.
//      Edge 64 fills the last free slot (head wraps to one before tail).
//      Edge 65 onward: the ISR sees buffer full -> bumps s_btnQ.isrDropCount
//        and returns. Edges 1..64 stay in the buffer; new edges are lost.
//    Redraw finishes; loop() iterates. It reads s_btnQ.isrDropCount, sees it >
//    BTN_QUEUE_RECOVER_THRESHOLD, calls clearButtonQueue() + resetState().
//    The mash is discarded; the next intentional press starts cleanly.
//
//  Walked-through example — ISR fires DURING poll()'s drain:
//
//    poll() snapshots head=12 at t=2000 and starts draining slots 5..11.
//    At t=2005 the ISR fires and writes slot 12, head becomes 13.
//    poll() doesn't see slot 12 this iteration — it's iterating against the
//    snapshot. That edge is picked up by the NEXT poll() call, which
//    re-snapshots head. Worst-case latency: one loop iteration.
// ============================================================================
void markUserActivity() {
  s_lastUserActionMs = millis();
}

uint32_t userIdleMs() {
  return (uint32_t)(millis() - s_lastUserActionMs);
}

static inline uint32_t isrNowMs() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void clearButtonQueue() {
  noInterrupts();
  s_btnQ.head = 0;
  s_btnQ.tail = 0;
  interrupts();
}

bool maybeRecoverFromIsrOverflow() {
  if (s_btnQ.isrDropCount <= BTN_QUEUE_RECOVER_THRESHOLD) return false;
  noInterrupts();
  s_btnQ.isrDropCount = 0;
  interrupts();
  clearButtonQueue();
  g_btns.resetState();
  return true;
}

void IRAM_ATTR btnISR() {
  uint8_t next = (uint8_t)((s_btnQ.head + 1) % BTN_Q);
  if (next == s_btnQ.tail) {
    // Queue full: drop the NEW event. Advancing tail from the ISR would
    // race ButtonState::poll() (the only legitimate tail consumer) and
    // deliver events out of order.
    s_btnQ.isrDropCount = s_btnQ.isrDropCount + 1;  // C++20 deprecates ++ on volatile
    return;
  }
  s_btnQ.state[s_btnQ.head] = (digitalRead(BTN) == LOW);
  s_btnQ.timeMs[s_btnQ.head] = isrNowMs();
  s_btnQ.head = next;
}

bool buttonQueueNonEmpty() {
  return s_btnQ.head != s_btnQ.tail;
}

void injectButtonEdgeNow(bool pressed) {
  noInterrupts();
  uint8_t next = (uint8_t)((s_btnQ.head + 1) % BTN_Q);
  if (next == s_btnQ.tail) {
    // Queue full: drop the NEW event (see btnISR for why).
    s_btnQ.isrDropCount = s_btnQ.isrDropCount + 1;  // C++20 deprecates ++ on volatile
    interrupts();
    return;
  }
  s_btnQ.state[s_btnQ.head] = pressed;
  s_btnQ.timeMs[s_btnQ.head] = isrNowMs();
  s_btnQ.head = next;
  interrupts();
}

// ----------------------------------------------------------------------------
// ButtonState::poll — interpret raw ISR edges as click events.
//
// Three pipeline stages, in order:
//
//   ISR queue  -->  debounce  -->  classifier
//   (s_btnQ)        (kDefaultDebounceMs)   (this function's job)
//
// The classifier is awkward for one reason: the meaning of a click depends
// on what comes AFTER it. A short click is only a short click once enough
// silence has passed to rule out a double. So we have three emit points:
//   1. Inside the drain loop, on a release whose duration >= kDefaultLongMs —
//      defensive fallback; in normal operation point 2 fires first.
//   2. After the drain loop, when stablePressed_ AND pressArmed_ AND the
//      hold has crossed kDefaultLongMs — fires the long-click while the button
//      is still held, so the user gets feedback at the threshold instead
//      of after release. Sets pressArmed_=false to make the eventual
//      release a no-op (the release path's `if (pressArmed_)` guard skips).
//   3. After the drain loop, when a click sequence (short/double/triple)
//      has settled past its trailing-silence window.
//
// State that persists across calls — defined in input.h on ButtonState.
// Trailing-underscore names are member variables (vs. locals).
//   stablePressed_       last accepted physical state (true = pressed)
//   lastStableChange_    edge-time of last accepted transition (debounce ref)
//   pressStart_          edge-time of the current press's down-edge
//   pressArmed_          has this press not yet produced an event?
//                        true between press and the event it generates
//                        (either a release-time classification or a
//                        hold-detected long-click). Cleared when the press
//                        produces its event, or by clearButtonQueue() /
//                        resetState() so a release we didn't see the press
//                        for is ignored.
//   lastRelease_         edge-time of the most recent release
//   firstClickRelease_   edge-time of the first release in this sequence
//   clickCount_          releases accumulated, not yet emitted
//
// Output flags (cleared at the top of every poll, set when an event emits):
//   shortClick_, doubleClick_, tripleClick_, quadClick_, longClick_
//
// Time fields are all milliseconds. Edge times come from the ISR's
// esp_timer_get_time()/1000; the trailing-silence check uses millis().
// Both are the same clock domain (since boot), just different read paths.
//
// Flow:
//   1. Clear last poll's output flags.
//   2. Snapshot the ISR queue head (so we don't chase a moving target),
//      then drain entries [tail, head):
//        - Reject edges within kDefaultDebounceMs of the last accepted change.
//        - Reject edges that don't actually flip stablePressed_.
//        - On a down-edge: record pressStart_, arm the press.
//        - On an up-edge that's armed:
//            * duration >= kDefaultLongMs  -> emit longClick_, reset clickCount_
//            * else                 -> clickCount_++, remember releases
//   3. After draining, if a press is still held past kDefaultLongMs, emit
//      longClick_ now and clear pressArmed_ so the eventual release is
//      ignored. This is the normal path for long-click emission.
//   4. After draining, if a click sequence is pending, commit it when ANY of:
//        - gap elapsed:      now - lastRelease_      > kDefaultGapMs
//        - sequence timeout: now - firstClickRelease_ > kDefaultSequenceMs
//        - count >= 4:       quad commits immediately (nothing it could become)
//      Then map clickCount_ to shortClick_ / doubleClick_ / tripleClick_ /
//      quadClick_ and reset.
// ----------------------------------------------------------------------------
void ButtonState::poll() {
  resetClicks();

  // Take a snapshot of the producer cursor so we can drain a fixed range of the
  // queue without worrying about concurrent ISR writes. We DO let the ISR
  // write new events into the queue as we drain; those will be picked up by the
  // NEXT poll() call, which re-snapshots head.
  uint8_t headSnapshot;
  noInterrupts();
  headSnapshot = s_btnQ.head;
  interrupts();

  while (s_btnQ.tail != headSnapshot) {
    noInterrupts();
    bool edgePressed = s_btnQ.state[s_btnQ.tail];
    uint32_t edgeTime = s_btnQ.timeMs[s_btnQ.tail];
    s_btnQ.tail = (uint8_t)((s_btnQ.tail + 1) % BTN_Q);
    interrupts();

    if ((uint32_t)(edgeTime - lastStableChange_) <= ClickTimings::debounceMs()) continue;
    if (edgePressed == stablePressed_) continue;

    bool prevPressed = stablePressed_;
    stablePressed_ = edgePressed;
    lastStableChange_ = edgeTime;

    // On a clean press, start the timer and arm for a future release.
    if (!prevPressed) {
      pressStart_ = edgeTime;
      pressArmed_ = true;
    }

    // On a clean release, if we're armed, classify the click by duration and
    // accumulated count. Note: in normal operation, the hold-detection block
    // below this loop fires the long-click and clears pressArmed_ before the
    // release ever lands, so the `dur >= kDefaultLongMs` branch here is a defensive
    // fallback (e.g., for the unlikely case where poll() didn't run during
    // the hold).
    if (prevPressed) {
      if (pressArmed_) {
        uint32_t dur = (uint32_t)(edgeTime - pressStart_);
        HoldGestureKind k = classifyHoldRelease(dur, clickCount_);
        if (applyHoldKindTo(*this, k)) {
          clickCount_ = 0;
        } else if (dur < ClickTimings::longMs()) {
          // Short release — accumulate into the click sequence. Bump the
          // apps-API raw counter *before* the multi-click accumulator so
          // apps see every short release even when the classifier later
          // groups them into a double/triple.
          rawPressCount_++;
          clickCount_++;
          lastRelease_ = edgeTime;
          if (clickCount_ == 1) firstClickRelease_ = edgeTime;
        }
        // dur >= kDefaultLongMs but classifyHoldRelease returned None (count >= 2)
        // → pending clicks commit on their own; the hold contributes nothing.
      }
      pressArmed_ = false;
      pressStart_ = 0;
    }
  }

  // In-progress hold detection. ClickHold and VeryLong emit at the
  // earliest threshold the gesture is unambiguously identified, then
  // consume the press so the eventual release is ignored (cleared
  // pressArmed_ → release path's `if (pressArmed_)` guard skips). Plain
  // Long is deferred to release because at kDefaultLongMs we can't yet tell
  // whether the user is on their way to a VeryLong — that case lives
  // in the release classifier above.
  if (stablePressed_ && pressArmed_) {
    uint32_t held = (uint32_t)(millis() - pressStart_);
    HoldGestureKind k = classifyHoldInProgress(held, clickCount_);
    if (applyHoldKindTo(*this, k)) {
      clickCount_ = 0;
      pressArmed_ = false;
    }
  }

  // Trailing-silence check: commit the accumulated click count when ANY of:
  //   1. The user paused for kDefaultGapMssince the most recent release
  //      AND isn't currently pressing — an in-progress press might still be
  //      part of the sequence (e.g., a slightly-late double-click), so we
  //      wait for its release to find out. Without this, a press starting at
  //      gap+epsilon would commit a single and then count the late release
  //      as a fresh sequence — turning one intended double into two singles.
  //   2. The whole sequence has run past kDefaultSequenceMs from the first
  //      release — caps slow multi-clicks. Same press-in-progress gate.
  //   3. We hit a 4th click — quad needs no wait, there's nothing it could
  //      become next. Commits even if a 5th press is in progress (it'll
  //      start a fresh sequence, which is what the user means by pressing
  //      again after the quad).
  // The mapping from final count to which flag to emit is the only thing that
  // varies by count; the timer logic is uniform.
  //
  // Edge case (knowingly accepted): tap-then-immediate-long-hold loses the
  // tap. The long branch above resets clickCount_, so the pending single is
  // dropped. Rationale: long-press is the more deliberate action; preserving
  // its reliability matters more than chasing a flow that doesn't occur at
  // human speeds in practice.
  if (clickCount_ > 0) {
    uint32_t now = millis();
    bool gapElapsed      = (uint32_t)(now - lastRelease_)       > ClickTimings::maxClickGapMs();
    bool sequenceTimeout = (uint32_t)(now - firstClickRelease_) > ClickTimings::maxClickSequenceMs();
    bool quadReady       = clickCount_ >= 4;
    bool readyByTime     = (gapElapsed || sequenceTimeout) && !stablePressed_;

    if (readyByTime || quadReady) {
      if      (clickCount_ == 1) shortClick_  = true;
      else if (clickCount_ == 2) doubleClick_ = true;
      else if (clickCount_ == 3) tripleClick_ = true;
      else                        quadClick_   = true;
      clickCount_ = 0;
    }
  }
}

ButtonEvent waitForNextEvent() {
  markUserActivity();
  while (true) {
    g_btns.poll();
    maybeRecoverFromIsrOverflow();
    ButtonEvent e = ButtonEvent::fromButtonState(g_btns);
    if (e.any()) {
      // Consume the flag we're about to return so the next call doesn't
      // re-see it. (`poll()` clears flags at its top, so a same-tick
      // re-entry would already be empty, but `resetClicks()` here makes
      // the contract explicit and protects against callers that loop
      // tighter than poll's tick boundaries.)
      g_btns.resetClicks();
      markUserActivity();
      return e;
    }
    delay(1);
  }
}

void resetInputFrontend() {
  // Wait for the button that triggered this transition (wake or triple-click)
  // to be physically released, then debounce. This prevents that single press
  // from leaking into the new mode as an accidental action.
  // We do NOT clear the whole ISR queue — any presses that arrive AFTER
  // release are intentional and should be processed normally.
  uint32_t start = millis();
  while (digitalRead(BTN) == LOW && (uint32_t)(millis() - start) < 600) delay(1);
  delay(ClickTimings::debounceMs() + 2); // minimal debounce after release

  // Discard only events that happened BEFORE this moment (the transition press).
  // Events queued after the release are kept.
  noInterrupts();
  uint8_t headNow = s_btnQ.head;
  interrupts();
  s_btnQ.tail = headNow; // advance tail to head = discard old events only
  g_btns.resetState();
  markUserActivity();
}
