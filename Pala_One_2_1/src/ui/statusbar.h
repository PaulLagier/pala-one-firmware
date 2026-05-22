#ifndef PALA_UI_STATUSBAR_H
#define PALA_UI_STATUSBAR_H

// ============================================================================
//  Statusbar — owns the reader's bottom-of-screen progress display mode.
//
//  Three modes:
//    Full     — 8px reserve, page-progress bar + page number (the default)
//    Minimal  — 1px reserve, single-pixel progress fraction
//    Hidden   — 0px reserve, no statusbar (maximum reading area)
//
//  The reader's drawStatusBar reads the current mode and dispatches; the
//  font module reads the reserve height when computing how many text lines
//  fit on a page, so a mode change must invalidate the layout cache.
// ============================================================================
namespace Statusbar {

enum Mode {
  Full    = 0,
  Minimal = 1,
  Hidden  = 2,
};

// NVS load on boot — call once from setup() after `prefs.begin`.
void loadSettings();

// Current mode. Reader + font.
Mode mode();

// Pixels reserved at the bottom of the screen for the statusbar in the
// current mode. Used by `Font::bodyLayout()` to size the text area.
int  reserveH();

// Apply + persist a new mode. Invalidates the font layout cache so the
// next render uses the new reserve height. Clamps unknown values to Full.
void setMode(Mode m);

// Cycle to the next mode (Full -> Minimal -> Hidden -> Full). Used by the
// short-click on the reader menu's statusbar row.
void cycleMode();

}  // namespace Statusbar

#endif  // PALA_UI_STATUSBAR_H
