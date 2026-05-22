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

enum ButtonAction {
  ACTION_NONE     = 0,
  ACTION_BOOKMARK = 1,
  ACTION_LOCK     = 2,
  ACTION_MENU     = 3,
};

namespace Gestures {

// NVS load on boot — call once from setup() after `prefs.begin`.
void loadSettings();

// Current bound action for each remappable gesture.
ButtonAction actionLong();       // plain long press (>= LONG_MS, < VERY_LONG_MS, no preceding click)
ButtonAction actionExtraLong();  // very-long press (>= VERY_LONG_MS, no preceding click)
ButtonAction actionClickHold();  // short click then immediate long hold

// Apply + persist a binding. Out-of-range values clamp to ACTION_NONE.
void setActionLong(ButtonAction a);
void setActionExtraLong(ButtonAction a);
void setActionClickHold(ButtonAction a);

// Convenience: which action (if any) is currently bound to the gesture
// kind that just fired. Returns ACTION_NONE for non-remappable kinds
// (Short, Double, Triple, Quad, None).
ButtonAction actionFor(ButtonEvent::Kind kind);

}  // namespace Gestures

#endif  // PALA_UI_READER_ACTIONS_H
