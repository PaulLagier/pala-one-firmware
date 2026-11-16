// Tests for src/pure/hold_gesture.cpp.
//
// The classifier is the deterministic heart of the three remappable hold
// gestures (Long / VeryLong / ClickHold). The on-device input.cpp drives
// it from the ISR-fed state machine; here we exercise it directly with
// concrete (duration, click_count) pairs.

#include "test_framework.h"
#include "pure/hold_gesture.h"

// ============================================================================
//  Release-time classifier
// ============================================================================

TEST_CASE("classifyHoldRelease — short release is not a hold") {
  CHECK_EQ(classifyHoldRelease(0,             0), HoldNone);
  CHECK_EQ(classifyHoldRelease(LONG_MS - 1,   0), HoldNone);
}

TEST_CASE("classifyHoldRelease — solo hold at LONG_MS is Long") {
  CHECK_EQ(classifyHoldRelease(LONG_MS,                  0), HoldLong);
  CHECK_EQ(classifyHoldRelease(LONG_MS + 1,              0), HoldLong);
  CHECK_EQ(classifyHoldRelease(VERY_LONG_MS - 1,         0), HoldLong);
}

TEST_CASE("classifyHoldRelease — solo hold at VERY_LONG_MS is VeryLong") {
  CHECK_EQ(classifyHoldRelease(VERY_LONG_MS,             0), HoldVeryLong);
  CHECK_EQ(classifyHoldRelease(VERY_LONG_MS + 1,         0), HoldVeryLong);
  CHECK_EQ(classifyHoldRelease(10 * VERY_LONG_MS,        0), HoldVeryLong);
}

TEST_CASE("classifyHoldRelease — click then hold of any length is ClickHold") {
  // ClickHold consumes the pending single; duration past LONG_MS doesn't
  // upgrade it to VeryLong (the gesture identity is "click then hold",
  // not "very long click then hold").
  CHECK_EQ(classifyHoldRelease(LONG_MS,                  1), HoldClickHold);
  CHECK_EQ(classifyHoldRelease(VERY_LONG_MS,             1), HoldClickHold);
  CHECK_EQ(classifyHoldRelease(10 * VERY_LONG_MS,        1), HoldClickHold);
}

TEST_CASE("classifyHoldRelease — short release with pending click is None") {
  // Below LONG_MS the release is just another click in the sequence, not
  // a hold. The accumulator handles it, not the hold classifier.
  CHECK_EQ(classifyHoldRelease(LONG_MS - 1,              1), HoldNone);
  CHECK_EQ(classifyHoldRelease(0,                        1), HoldNone);
}

TEST_CASE("classifyHoldRelease — multi-click followed by hold is None") {
  // Tap-tap-hold isn't a defined gesture. The pending two clicks commit
  // on their own; the hold contributes nothing.
  CHECK_EQ(classifyHoldRelease(LONG_MS,                  2), HoldNone);
  CHECK_EQ(classifyHoldRelease(VERY_LONG_MS,             2), HoldNone);
  CHECK_EQ(classifyHoldRelease(LONG_MS,                  3), HoldNone);
  CHECK_EQ(classifyHoldRelease(LONG_MS,                255), HoldNone);
}

// ============================================================================
//  In-progress (still-pressed) classifier
// ============================================================================

TEST_CASE("classifyHoldInProgress — below thresholds is None") {
  CHECK_EQ(classifyHoldInProgress(0,                     0), HoldNone);
  CHECK_EQ(classifyHoldInProgress(LONG_MS - 1,           0), HoldNone);
  CHECK_EQ(classifyHoldInProgress(VERY_LONG_MS - 1,      0), HoldNone);
}

TEST_CASE("classifyHoldInProgress — plain Long is deferred to release") {
  // At LONG_MS with no pending click, we can't yet tell whether the user
  // is going to a VeryLong. The release-time classifier handles the
  // Long-vs-VeryLong decision once the press ends.
  CHECK_EQ(classifyHoldInProgress(LONG_MS,               0), HoldNone);
  CHECK_EQ(classifyHoldInProgress(LONG_MS + 100,         0), HoldNone);
  CHECK_EQ(classifyHoldInProgress(VERY_LONG_MS - 1,      0), HoldNone);
}

TEST_CASE("classifyHoldInProgress — VeryLong fires at threshold") {
  CHECK_EQ(classifyHoldInProgress(VERY_LONG_MS,          0), HoldVeryLong);
  CHECK_EQ(classifyHoldInProgress(VERY_LONG_MS + 1,      0), HoldVeryLong);
  CHECK_EQ(classifyHoldInProgress(10 * VERY_LONG_MS,     0), HoldVeryLong);
}

TEST_CASE("classifyHoldInProgress — ClickHold fires at LONG_MS with pending click") {
  CHECK_EQ(classifyHoldInProgress(LONG_MS,               1), HoldClickHold);
  CHECK_EQ(classifyHoldInProgress(LONG_MS + 1,           1), HoldClickHold);
  CHECK_EQ(classifyHoldInProgress(VERY_LONG_MS,          1), HoldClickHold);
}

TEST_CASE("classifyHoldInProgress — multi-click held is None") {
  // Same rationale as release-time: tap-tap-hold etc. isn't a gesture.
  CHECK_EQ(classifyHoldInProgress(LONG_MS,               2), HoldNone);
  CHECK_EQ(classifyHoldInProgress(VERY_LONG_MS,          2), HoldNone);
  CHECK_EQ(classifyHoldInProgress(VERY_LONG_MS,          3), HoldNone);
}

TEST_CASE("button response presets preserve the balanced default for invalid input") {
  CHECK_EQ(validatedClickGap(120),120u);CHECK_EQ(validatedClickGap(175),175u);
  CHECK_EQ(validatedClickGap(250),250u);CHECK_EQ(validatedClickGap(0),175u);
  CHECK_EQ(validatedClickGap(0xffffffffu),175u);
}

#include "pure/input_timing.h"
TEST_CASE("custom timings reject inconsistent and overflowed candidates") {
  InputTiming t;CHECK(validInputTiming(t));
  t.gap=1000;CHECK(!validInputTiming(t));t.sequence=1000;CHECK(validInputTiming(t));
  t.hold=200;CHECK(!validInputTiming(t));t.hold=2000;CHECK(!validInputTiming(t));
  t.veryLong=3000;CHECK(validInputTiming(t));t.debounce=51;CHECK(!validInputTiming(t));
  uint32_t n=17;for(const char* bad:{"","-1","1.5","175x"," 175","4294967296"}){CHECK(!parseTimingNumber(bad,n));CHECK_EQ(n,17u);}
  CHECK(parseTimingNumber("4294967295",n));CHECK_EQ(n,UINT32_MAX);
  CHECK_EQ(classifyHoldRelease(599,0,600,1800),HoldNone);
  CHECK_EQ(classifyHoldRelease(600,0,600,1800),HoldLong);
  CHECK_EQ(classifyHoldRelease(1800,0,600,1800),HoldVeryLong);
  CHECK_EQ(classifyHoldInProgress(600,1,600,1800),HoldClickHold);
}
