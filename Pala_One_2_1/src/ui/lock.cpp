#include "src/ui/lock.h"

#include "src/state.h" // prefs
#include "src/ui/reader_actions.h"
#include <initializer_list>

namespace Lock {

static constexpr const char *kKey = "cfg_locked";

static bool s_locked = false;

void loadSettings() { s_locked = prefs.getBool(kKey, false); }

bool isLocked() { return s_locked; }

void engage() {
  if (s_locked)
    return;
  s_locked = true;
  prefs.putBool(kKey, true);
}

void disengage() {
  if (!s_locked)
    return;
  s_locked = false;
  prefs.putBool(kKey, false);
}

bool isUnlockGesture(const ButtonEvent &ev) {
  if (!Gestures::legacyControlsOn()) {
    // Match saved lock bindings, including clicks. If none is assigned,
    // retain the hold fallback for automatic locking and older settings.
    for (auto kind : {ButtonEvent::Short, ButtonEvent::Double, ButtonEvent::Triple,
                      ButtonEvent::Long, ButtonEvent::VeryLong, ButtonEvent::ClickHold}) {
      if (Gestures::actionFor(kind) == ACTION_LOCK)
        return Gestures::actionFor(ev.kind) == ACTION_LOCK;
    }
  }
  return ev.kind == ButtonEvent::Long || ev.kind == ButtonEvent::VeryLong ||
         ev.kind == ButtonEvent::ClickHold;
}

} // namespace Lock
