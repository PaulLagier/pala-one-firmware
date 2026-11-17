#include "src/ui/lock.h"

#include "src/state.h" // prefs
#include "src/ui/reader_actions.h"
#include <algorithm>
#include <initializer_list>

namespace Lock {

static constexpr const char *kKey = "cfg_locked";

static bool s_locked = false;
static bool s_iconEnabled = true;
static constexpr const char *kIconKey = "cfg_lockicon";

void loadSettings() {
  s_locked = prefs.getBool(kKey, false);
  s_iconEnabled = prefs.getBool(kIconKey, true);
}
bool iconEnabled() { return s_iconEnabled; }
void setIconEnabled(bool enabled) {
  if (s_iconEnabled == enabled)
    return;
  s_iconEnabled = enabled;
  prefs.putBool(kIconKey, enabled);
}

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
    const auto kinds = {ButtonEvent::Short,    ButtonEvent::Double, ButtonEvent::Triple,
                        ButtonEvent::Quad,     ButtonEvent::Long,   ButtonEvent::VeryLong,
                        ButtonEvent::ClickHold};
    if (std::any_of(kinds.begin(), kinds.end(), [](ButtonEvent::Kind kind) {
          return Gestures::actionFor(kind) == ACTION_LOCK;
        }))
      return Gestures::actionFor(ev.kind) == ACTION_LOCK;
  }
  return ev.kind == ButtonEvent::Long || ev.kind == ButtonEvent::VeryLong ||
         ev.kind == ButtonEvent::ClickHold;
}

} // namespace Lock
