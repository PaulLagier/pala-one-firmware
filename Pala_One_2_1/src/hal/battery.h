#ifndef PALA_HAL_BATTERY_H
#define PALA_HAL_BATTERY_H

#include "src/config.h"
#include "src/state.h"

#if HAS_BATTERY
void adcSetupOnce();
void updateBatteryCached(bool force = false);

/*
 * This is a more conservative version of updateBatteryCached that is
 * used when checking battery status in the background. It doesn't allow
 * bypassing the cache and will only check charging status if it would
 * be doing a full update anyway.
 */
void updateBatteryBackground();

void drawBatteryTopRight(bool extended = false);
void drawBatteryBottomLeft();
bool batteryChargingChanged();

/*
 * True iff the most recent reading is valid and below the low-battery
 * threshold.
 */
bool batteryLow();

/*
 * Leftmost x the top-right battery indicator can occupy, including the
 * charging bolt that hangs off its left side. Anything else anchored to the
 * right of the header (see Icons::drawStatusTray) lays itself out from here
 * so the geometry lives in one place.
 */
int batteryTopRightLeftEdge();
#else
// No battery indicator on this board: the right margin is entirely free.
// Defined rather than omitted so callers need no #if of their own.
inline int batteryTopRightLeftEdge() { return SCREEN_W - MARGIN_X; }
#endif

#endif  // PALA_HAL_BATTERY_H
