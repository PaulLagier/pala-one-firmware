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
//  Unlock policy is permissive: any of {Long, VeryLong, ClickHold} unlocks,
//  not strictly the gesture currently bound to ACTION_LOCK. After a
//  deep-sleep wake the firmware can't reliably reconstruct a chord
//  (the wake press's start edge happens before the ISR is attached), so
//  strict matching would leave a user unable to unlock if they bound LOCK
//  to the chord.
// ============================================================================
namespace Lock {

// NVS load on boot — call once from setup() after `prefs.begin`.
void loadSettings();

// Is the device currently locked?
bool isLocked();

// Engage the lock and persist. Caller is expected to then put the device
// to sleep — Lock does not itself enter deep sleep so the call site can
// arrange any other pre-sleep work (toast, save, etc.) first.
void engage();

// Disengage the lock and persist.
void disengage();

// Does this event count as an unlock gesture under the permissive policy?
bool isUnlockGesture(const ButtonEvent& ev);

}  // namespace Lock

#endif  // PALA_UI_LOCK_H
