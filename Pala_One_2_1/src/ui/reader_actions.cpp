#include "src/ui/reader_actions.h"

#include "src/state.h"  // prefs

namespace Gestures {

static constexpr const char* kKeyLegacyControls = "cfg_legaCont";

static constexpr const char* kKeyShort    = "cfg_btnS";
static constexpr const char* kKeyDouble    = "cfg_btnD";
static constexpr const char* kKeyTriple     = "cfg_btnT";
static constexpr const char* kKeyLong      = "cfg_btnL";
static constexpr const char* kKeyExtraLong = "cfg_btnXL";
static constexpr const char* kKeyClickHold = "cfg_btnCH";

// Defaults chosen to make the device useful out of the box:
static ButtonAction s_short    = ACTION_NEXT;
static ButtonAction s_double    = ACTION_PREV;
static ButtonAction s_triple     = ACTION_HOME;
static ButtonAction s_long      = ACTION_MENU;
static ButtonAction s_extraLong = ACTION_LOCK;
static ButtonAction s_clickHold = ACTION_BOOKMARK;
static bool s_legacyControls = true;

static ButtonAction clamp(int v) {
  if (v < ACTION_NONE || v > ACTION_ROTATE) return ACTION_NONE;
  return (ButtonAction)v;
}

void loadSettings() {
  s_short    = clamp(prefs.getInt(kKeyShort,      ACTION_NEXT));
  s_double    = clamp(prefs.getInt(kKeyDouble,      ACTION_PREV));
  s_triple    = clamp(prefs.getInt(kKeyTriple,      ACTION_HOME));
  s_long      = clamp(prefs.getInt(kKeyLong,      ACTION_MENU));
  s_extraLong = clamp(prefs.getInt(kKeyExtraLong, ACTION_LOCK));
  s_clickHold = clamp(prefs.getInt(kKeyClickHold, ACTION_BOOKMARK));
  s_legacyControls = prefs.getBool(kKeyLegacyControls, true);
}

ButtonAction actionShort()    { return s_short; }
ButtonAction actionDouble()    { return s_double; }
ButtonAction actionTriple()    { return s_triple; }
ButtonAction actionLong()      { return s_long; }
ButtonAction actionExtraLong() { return s_extraLong; }
ButtonAction actionClickHold() { return s_clickHold; }

static void persist(const char* key, ButtonAction& dest, ButtonAction value) {
  ButtonAction v = clamp(value);
  if (v == dest) return;
  dest = v;
  prefs.putInt(key, (int)v);
}

void setActionShort(ButtonAction a)    { persist(kKeyShort,      s_short,      a); }
void setActionDouble(ButtonAction a)    { persist(kKeyDouble,      s_double,      a); }
void setActionTriple(ButtonAction a)    { persist(kKeyTriple,      s_triple,      a); }
void setActionLong(ButtonAction a)      { persist(kKeyLong,      s_long,      a); }
void setActionExtraLong(ButtonAction a) { persist(kKeyExtraLong, s_extraLong, a); }
void setActionClickHold(ButtonAction a) { persist(kKeyClickHold, s_clickHold, a); }
void setLegacyControls(bool legacy) {
  s_legacyControls = legacy;
  prefs.putBool(kKeyLegacyControls, legacy);
}

bool legacyControlsOn() {
  return s_legacyControls;
}

bool resolveLegacyAction(const ButtonEvent &e, const ButtonEvent::Kind &legacyGesture, ButtonAction targetAction)
{
  ButtonAction ac = Gestures::actionFor(e.kind);
  bool legacyCont = Gestures::legacyControlsOn();
  if (legacyCont) {
    return e.kind == legacyGesture;
  }
  else {
    return ac == targetAction;
  }
}

bool isNonLegacyAction(const ButtonEvent &e, ButtonAction action) {
  if (legacyControlsOn()) {
    return false;
  }
  if (actionFor(e.kind) == action) {
    return true;
  }
  return false;
}

ButtonAction actionFor(ButtonEvent::Kind kind) {
  switch (kind) {
    case ButtonEvent::Short:      return s_short;
    case ButtonEvent::Double:      return s_double;
    case ButtonEvent::Triple:      return s_triple;
    case ButtonEvent::Long:      return s_long;
    case ButtonEvent::VeryLong:  return s_extraLong;
    case ButtonEvent::ClickHold: return s_clickHold;
    default:                     return ACTION_NONE;
  }
}

}  // namespace Gestures
