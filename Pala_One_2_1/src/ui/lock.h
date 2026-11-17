#ifndef PALA_UI_LOCK_H
#define PALA_UI_LOCK_H

#include "src/hal/input.h"   // ButtonEvent

// ============================================================================
//  Lock — owned device-lock state.
//
//  When the user binds ACTION_LOCK to a hold gesture and triggers it, we
//  set the lock flag, persist it, and enter deep sleep. Lock state survives
//  power cycles via NVS, so a locked device that fully powers down still
//  comes back locked.
//
//  Custom controls unlock only with a gesture bound to ACTION_LOCK. Legacy
//  controls, or automatic locking with no assigned lock gesture, retain the
//  Long / VeryLong / ClickHold fallback. Wake input is captured early enough
//  to reconstruct click-then-hold gestures.
// ============================================================================
namespace Lock {

// NVS load on boot — call once from setup() after `prefs.begin`.
void loadSettings();

// Is the device currently locked?
bool isLocked();

// Only controls the screensaver badge; locking and unlock gestures are unchanged.
// Enabled by default for compatibility with existing saved settings.
bool iconEnabled();
void setIconEnabled(bool enabled);

// Engage the lock and persist. Caller is expected to then put the device
// to sleep — Lock does not itself enter deep sleep so the call site can
// arrange any other pre-sleep work (toast, save, etc.) first.
void engage();

// Disengage the lock and persist.
void disengage();

// Does this event match the configured lock gesture or the hold fallback?
bool isUnlockGesture(const ButtonEvent& ev);

}  // namespace Lock

#endif  // PALA_UI_LOCK_H
