#ifndef PALA_UI_ICONS_H
#define PALA_UI_ICONS_H

// ============================================================================
//  Icons — the header status tray.
//
//  Reports device states that quietly cost battery and are otherwise
//  invisible while browsing menus — currently just something holding deep
//  sleep off. Each icon is individually toggleable from the Web UI (NVS key
//  `cfg_ico_sleep`, default on).
//
//  Layout. The tray is right-anchored and packs leftward from whatever edge
//  `trayRightEdge` hands back — immediately left of the battery indicator on
//  screens that draw one, otherwise the right margin. Icons pack tightly
//  rather than occupying fixed slots, so they shift when one turns off;
//  reserving slots would cost the header ~14px permanently to display a
//  state that is usually absent.
//
//  Per-icon draw convention, for whoever adds the next glyph (issue #104's
//  lock icon is the expected one): a glyph function is
//  `void drawXxxGlyph(int x, int y)`, paints only inside
//  [x, x+w) x [y, y+kIconH) using colour 1, assumes nothing about what is
//  underneath, and does NOT clear — the tray clears each icon's box first.
//  Procedural drawing or an XBM blit are both fine; the glyph here is XBM
//  because hand-placed pixels beat anything Bresenham produces at 9px.
// ============================================================================
namespace Icons {

// NVS load on boot — call once from setup() after `prefs.begin`.
void loadSettings();

bool sleepIconEnabled();
void setSleepIconEnabled(bool val);

// Draw every enabled-and-active icon, packing leftward from `rightEdge`.
// Returns the leftmost x consumed, so the caller can clamp a title against
// it; returns `rightEdge` unchanged when nothing was drawn.
int drawStatusTray(int rightEdge);

// Right edge the tray should anchor to, given whether the caller drew the
// battery indicator.
int trayRightEdge(bool batteryDrawn);

// Latching change-detector, same contract as `batteryChargingChanged()`:
// true (once) when the set of icons that would be drawn has changed since
// the last call. The main loop uses it to repaint a menu whose header went
// stale without any button press — plugging in USB, for instance.
bool trayStateChanged();

}  // namespace Icons

#endif  // PALA_UI_ICONS_H
