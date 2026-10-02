#ifndef PALA_UI_READER_ACTIONS_H
#define PALA_UI_READER_ACTIONS_H

#include "src/hal/input.h"  // ButtonEvent::Kind

// ============================================================================
//  Reader hold-gesture bindings
//
//  Maps the three remappable hold gestures (Long / VeryLong / ClickHold)
//  to one of the reader's actions (none / bookmark / lock / menu) and
//  persists the choice. Lives here, not in `src/hal/input.h`, because the
//  bindable actions are reader-screen concepts — the input layer just
//  classifies button events; what an event *means* belongs to the screen
//  that consumes it. If another screen ever wants its own gesture-to-action
//  table it can do so without touching the HAL.
//
//  Defaults (chosen to be useful out of the box):
//    Long      = Bookmark — the most common action while reading
//    VeryLong  = Lock     — a deliberate "I'm putting it down" gesture
//    ClickHold = Menu     — easy chord, doesn't fight short-click paging
// ============================================================================

enum ButtonAction
{
  ACTION_NONE = 0,     // No action
  ACTION_NEXT = 1,     // Next item/page
  ACTION_PREV = 2,     // Previous item/page
  ACTION_MENU = 3,  // Select current item or open book menu
  ACTION_LOCK = 4,     // Lock/unlock device
  ACTION_HOME = 5,     // Go to main menu
  ACTION_BOOKMARK = 6, // Bookmark current page
  ACTION_ROTATE = 7,   // Flip screen rotation
  // Keep ACTION_ROTATE as the one with highest index or adjust Gestures::clamp accordingly
};

namespace Gestures {

// NVS load on boot — call once from setup() after `prefs.begin`.
void loadSettings();

// Current bound action for each remappable gesture.
ButtonAction actionShort();
ButtonAction actionDouble();
ButtonAction actionTriple();
ButtonAction actionLong();       // plain long press (>= kDefaultLongMs, < kDefaultVeryLongMs, no preceding click)
ButtonAction actionExtraLong();  // very-long press (>= kDefaultVeryLongMs, no preceding click)
ButtonAction actionClickHold();  // short click then immediate long hold

// Apply + persist a binding. Out-of-range values clamp to ACTION_NONE.
void setActionShort(ButtonAction a);
void setActionDouble(ButtonAction a);
void setActionTriple(ButtonAction a);
void setActionLong(ButtonAction a);
void setActionExtraLong(ButtonAction a);
void setActionClickHold(ButtonAction a);
void setLegacyControls(bool legacy);

bool legacyControlsOn();

// Helps dealing with the conflict between legacy and standard actions/gestures
// If the user as selected legacy actions (Gestures::legacyControlsOn()) then the function returns true if the kind of
// event `e` matches `legacyGesture`. Otherwise it checks if event `e` is associated with action `targetAction`
bool resolveLegacyAction(const ButtonEvent &e, const ButtonEvent::Kind &legacyGesture, ButtonAction targetAction);

// Returns true if legacy actions are off and ButtonEvent `e` matches the ButtonAction `action`
bool isNonLegacyAction(const ButtonEvent &e, ButtonAction action);

// Convenience: which action (if any) is currently bound to the gesture
// kind that just fired. Returns ACTION_NONE for non-remappable kinds
// (Quad, None).
ButtonAction actionFor(ButtonEvent::Kind kind);

} // namespace Gestures

#endif  // PALA_UI_READER_ACTIONS_H
