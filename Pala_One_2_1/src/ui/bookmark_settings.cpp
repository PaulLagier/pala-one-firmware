#include "src/ui/bookmark_settings.h"
#include "src/state.h"
namespace BookmarkSettings {
static constexpr const char *key = "cfg_bm_toggle";
bool toggleEnabled() { return prefs.getBool(key, false); }
bool setToggleEnabled(bool enabled) {
  if (toggleEnabled() == enabled)
    return true;
  prefs.putBool(key, enabled);
  return toggleEnabled() == enabled;
}
} // namespace BookmarkSettings
