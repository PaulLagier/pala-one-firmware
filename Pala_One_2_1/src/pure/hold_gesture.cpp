#include "src/pure/hold_gesture.h"

HoldGestureKind classifyHoldRelease(uint32_t dur_ms, uint8_t click_count) {
  if (dur_ms < ClickTimings::longMs()) return HoldNone;
  if (click_count == 1) return HoldClickHold;
  if (click_count >= 2) return HoldNone;
  if (dur_ms >= ClickTimings::veryLongMs()) return HoldVeryLong;
  return HoldLong;
}

HoldGestureKind classifyHoldInProgress(uint32_t held_ms, uint8_t click_count) {
  if (click_count == 1 && held_ms >= ClickTimings::longMs()) return HoldClickHold;
  if (click_count == 0 && held_ms >= ClickTimings::veryLongMs()) return HoldVeryLong;
  return HoldNone;
}
