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
#endif

#endif  // PALA_HAL_BATTERY_H
