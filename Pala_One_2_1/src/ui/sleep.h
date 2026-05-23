#ifndef PALA_UI_SLEEP_H
#define PALA_UI_SLEEP_H

#include "src/pure/arduino_compat.h"  // uint32_t

// ============================================================================
//  Sleep module — owns the deep-sleep entry sequence and the user-tunable
//  idle-timeout setting (NVS key `cfg_sleep`, default 120s, range [10, 3600])
//  plus the no-screensaver toggle (NVS key `cfg_noscr`, default false).
// ============================================================================
namespace Sleep {

// Read persisted settings from NVS into Sleep's internal state.
// Call once from setup() after `prefs.begin`.
void loadSettings();

// Apply + persist the idle timeout. Clamps to [10, 3600].
void setIdleTimeout(int secs);

// Current applied values.
int      idleTimeoutSecs();   // for the web settings UI selects
uint32_t idleTimeoutMs();     // for the main loop's sleep deadline check

// No-screensaver mode. When true: Sleep::enter() skips drawSleepScreen() so
// the last page stays visible on the e-ink panel, and setup() skips the
// boot-time display.clear() when waking from deep sleep so the panel is not
// blanked before the page is redrawn in fast mode.
bool noScreensaver();
void setNoScreensaver(bool val);

// Enter deep sleep right now. Notifies the active screen, draws the sleep
// image, releases peripherals, then `esp_deep_sleep_start`s.
void enter();

// Pause the CPU until the button is pressed or a short timer fires. Caller
// passes `tightTick` = true when something needs polling within a few tens
// of ms (mid-sequence click classifier, active toast); false otherwise (a
// loose heartbeat — the button still wakes us instantly regardless).
//
// Returns after wake. If the button caused the wake, a synthetic press
// edge is queued so the click classifier sees it. Safe to call from the
// main loop on any screen where `allowSleep()` is true.
void idleLightSleep(bool tightTick);

}  // namespace Sleep

#endif  // PALA_UI_SLEEP_H
