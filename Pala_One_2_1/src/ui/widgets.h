#ifndef PALA_UI_WIDGETS_H
#define PALA_UI_WIDGETS_H

#include <functional>

#include "src/config.h"
#include "src/state.h"

// ============================================================================
//  Shared drawing helpers used by multiple screens
// ============================================================================
// Draws the title, optionally the battery indicator, and optionally the
// status icon tray (see src/ui/icons.h). The title is clamped with an
// ellipsis to whatever width the tray leaves it, so a long title can no
// longer run underneath the indicators. Returns `contentTop`.
//
// `drawIcons` exists for AboutScreen, which suppresses the normal battery
// and then draws the wide *extended* battery block itself — a tray anchored
// to the right margin would land underneath it.
int drawSectionHeader(const char* title, bool drawBattery = true, bool drawIcons = true);

// Full-screen single- or two-line centered message. Always full-refreshes
// (no fastmode), so use it sparingly — for hard errors or status notices
// the user should notice. `b` is optional second line.
void drawCenter(const char* a, const char* b = nullptr);

// Set up the canvas for a menu-screen draw: every Nth call does a full
// refresh to clear e-ink ghosting; the rest go through fastmode. Counts
// independently of the reader's full-refresh schedule.
void prepareMenuFrame();

// Force the *next* call to `prepareMenuFrame` to do a full refresh,
// regardless of the periodic counter. Used at transitions into a menu
// overlay where the previous screen (e.g. the reader page) would otherwise
// ghost through a partial refresh. No-op if a full refresh would have
// happened anyway.
void forceNextMenuFrameFull();

// Draw one row of a scrollable menu list. Text starts at
// `UI_LIST_LEFT + extraIndent` on the given baseline; bold if selected.
// Resets font to Body afterwards. Most callers pass no indent;
// LibraryScreen passes its folder-depth + system-nudge offset.
//
// Labels too wide for the row are truncated with an ellipsis — doing it here
// rather than per-screen is what gives every menu the same behaviour without
// each one repeating the width arithmetic. Returns the pixel width actually
// drawn, which ListScreen needs so its strike-through matches the visible
// text rather than the original label.
int drawMenuRow(int yBaseline, const String& label, bool selected, int extraIndent = 0);

// Width available to a menu row's text at the given indent. Accounts for the
// scroll-indicator gutter, which is only reserved while a list is actually
// scrolling. Call it during a `drawScrollableList` callback.
int menuRowMaxWidth(int extraIndent = 0);

// Row height for the menu screens. Used internally by drawScrollableList;
// also exposed so callers that draw extra rows (e.g. ListScreen's selected
// continuation) can advance y consistently with the widget.
int menuLineH();

void splitListLabelForDisplay(const String& in, int maxWidth, String& line1, String& line2);

// ============================================================================
//  Scrollable list widget
//
//  Computes how many rows fit, which window of items is visible, and where
//  the selected item should sit (centered). Calls `drawRow` once per visible
//  item; the callback returns how many vertical rows it consumed (>= 1).
//  Most callbacks return 1; ListScreen's returns 2 for a selected item with
//  a continuation line. `budget` tells the callback how many rows remain in
//  the visible window — useful for "only draw the continuation if there's
//  room" decisions.
//
//  Caller responsibility: set the desired font before calling (the widget
//  reads metrics off whatever font is current to compute line height) and
//  pass `contentTopY` from `drawSectionHeader`.
// ============================================================================
using DrawListRowFn = std::function<int(int idx, int y, bool selected, int budget)>;

void drawScrollableList(int contentTopY,
                        int itemCount,
                        int selectedIndex,
                        const DrawListRowFn& drawRow);

// Leftmost x for menu rows.
static const int UI_LIST_LEFT = MARGIN_X + 4;

// Right gutter reserved for the scroll indicator arrows, so long row labels
// never collide with them. Reserved only while a list actually scrolls —
// see menuRowMaxWidth().
static const int UI_LIST_GUTTER_W = 8;

#endif  // PALA_UI_WIDGETS_H
