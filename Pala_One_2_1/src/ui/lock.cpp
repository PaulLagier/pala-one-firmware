#include "src/ui/lock.h"

#include "src/state.h"   // prefs

namespace Lock {

static constexpr const char* kKey = "cfg_locked";

static bool s_locked = false;

void loadSettings() {
  s_locked = prefs.getBool(kKey, false);
}

bool isLocked() { return s_locked; }

void engage() {
  if (s_locked) return;
  s_locked = true;
  prefs.putBool(kKey, true);
}

void disengage() {
  if (!s_locked) return;
  s_locked = false;
  prefs.putBool(kKey, false);
}

bool isUnlockGesture(const ButtonEvent& ev) {
  return ev.kind == ButtonEvent::Long
      || ev.kind == ButtonEvent::VeryLong
      || ev.kind == ButtonEvent::ClickHold;
}

}  // namespace Lock
