#include "src/pure/hold_gesture.h"

HoldGestureKind classifyHoldRelease(uint32_t dur_ms, uint8_t click_count, uint32_t longMs, uint32_t veryLongMs) {
  if (dur_ms < longMs) return HoldNone;
  if (click_count == 1) return HoldClickHold;
  if (click_count >= 2) return HoldNone;
  if (dur_ms >= veryLongMs) return HoldVeryLong;
  return HoldLong;
}

HoldGestureKind classifyHoldInProgress(uint32_t held_ms, uint8_t click_count, uint32_t longMs, uint32_t veryLongMs) {
  if (click_count == 1 && held_ms >= longMs) return HoldClickHold;
  if (click_count == 0 && held_ms >= veryLongMs) return HoldVeryLong;
  return HoldNone;
}

uint32_t validatedClickGap(uint32_t value) {
  return value == 120 || value == 175 || value == 250 ? value : MAX_CLICK_GAP_MS;
}
