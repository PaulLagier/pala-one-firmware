#ifndef PALA_HAL_BATTERY_H
#define PALA_HAL_BATTERY_H

#include "src/config.h"
#include "src/state.h"

#if HAS_BATTERY
void adcSetupOnce();
void updateBatteryCached(bool force = false);
void drawBatteryTopRight(bool extended = false);
bool batteryChargingChanged();
#endif

#endif  // PALA_HAL_BATTERY_H
