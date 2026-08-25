#include "src/ui/widgets.h"

#include "src/hal/battery.h"
#include "src/hal/display.h"
#include "src/pure/text_util.h"   // truncateWithEllipsis
#include "src/ui/font.h"
#include "src/ui/screen_settings.h"
#include "src/ui/icons.h"

static const int UI_HEADER_TOP = 6;
static const int UI_HEADER_GAP = 6;

// Set for the duration of a `drawScrollableList` call whose list is longer
// than its visible window, so `menuRowMaxWidth` knows to hold back the
// gutter. Cleared on every exit path — a standalone `drawMenuRow` (the
// "no items" rows several screens draw) must get the full width.
static bool s_listGutterActive = false;

// u8g2 measurement, wrapped as a plain function so its address can be handed
// to the pure truncation helper. Measures under whatever font is current,
// so callers must select the font first.
static int measureCurrentFont(const char* s) {
  return u8g2.getUTF8Width(s);
}

static String fitLabelWithEllipsis(const String& in, int maxWidth) {
  return truncateWithEllipsis(in, maxWidth, measureCurrentFont);
}

// Counter that drives the periodic full refresh of menu screens (clears
// e-ink ghosting). File-private; nothing outside `prepareMenuFrame` reads it.
static int s_menuDrawsSinceFull = 0;

// One-shot override: when set, the next `prepareMenuFrame` does a full
// refresh regardless of the counter. Set by `forceNextMenuFrameFull` at
// transitions into a menu overlay so the underlying screen (typically the
// reader page) doesn't ghost through a partial refresh.
static bool s_forceMenuFrameFull = false;

void forceNextMenuFrameFull() { s_forceMenuFrameFull = true; }

void prepareMenuFrame() {
  bool doFull = s_forceMenuFrameFull
             || (s_menuDrawsSinceFull >= MENU_FULL_REFRESH_EVERY);
  s_forceMenuFrameFull = false;
  if (doFull) {
    display.fastmodeOff();
    s_menuDrawsSinceFull = 0;
  } else {
    display.fastmodeOn();
  }
  beginPageCanvas();
  s_menuDrawsSinceFull++;
}

void drawCenter(const char* a, const char* b) {
  display.fastmodeOff();
  beginPageCanvas();
  Font::useBody();

  const int lineH = 16;
  int y = (SCREEN_H / 2) - lineH / 2;
  if (b) y -= lineH / 2;

  int wA = u8g2.getUTF8Width(a);
  u8g2.setCursor((SCREEN_W - wA) / 2, y);
  u8g2.print(a);

  if (b) {
    y += lineH;
    int wB = u8g2.getUTF8Width(b);
    u8g2.setCursor((SCREEN_W - wB) / 2, y);
    u8g2.print(b);
  }
  display.update();
}

int drawSectionHeader(const char* title, bool drawBattery, bool drawIcons) {
  Font::useBold();
  int ascent = u8g2.getFontAscent();
  int yTitle = UI_HEADER_TOP + ascent - 2;

  // Indicators first: the title is clamped to whatever they leave. Drawing
  // the title first would let the tray's self-clear chop it mid-glyph, which
  // it will at the larger body sizes — a size-14 bold title plus two icons
  // overruns the header on its own.
  int right = SCREEN_W - MARGIN_X;
#if HAS_BATTERY
  if (drawBattery && ScreenSettings::batteryIndicatorsEnabled()) {
    drawBatteryTopRight();
  }
#endif
  if (drawIcons) {
    right = Icons::drawStatusTray(Icons::trayRightEdge(drawBattery));
  }

  // Re-select bold: the indicators above leave the face set to whatever they
  // last needed, and yTitle was computed from bold's ascent.
  Font::useBold();
  u8g2.setCursor(MARGIN_X, yTitle);
  u8g2.print(fitLabelWithEllipsis(String(title), right - MARGIN_X - 4).c_str());

  int lineY = yTitle + 4;
  gfx.drawFastHLine(MARGIN_X, lineY, SCREEN_W - (MARGIN_X * 2), 1);

  int contentTop = lineY + UI_HEADER_GAP + 11;

  Font::useBody();
  return contentTop;
}

int menuRowMaxWidth(int extraIndent) {
  int right = SCREEN_W - MARGIN_X - (s_listGutterActive ? UI_LIST_GUTTER_W : 0);
  return right - UI_LIST_LEFT - extraIndent;
}

int drawMenuRow(int yBaseline, const String& label, bool selected, int extraIndent) {
  u8g2.setForegroundColor(1);
  if (selected) Font::useBold();
  else          Font::useBody();

  // Truncate under the row's own font — bold is wider, so a selected row
  // fits fewer characters than the same label unselected.
  String shown = fitLabelWithEllipsis(label, menuRowMaxWidth(extraIndent));
  int drawnW = u8g2.getUTF8Width(shown.c_str());

  u8g2.setCursor(UI_LIST_LEFT + extraIndent, yBaseline);
  u8g2.print(shown.c_str());
  Font::useBody();
  return drawnW;
}

int menuLineH() {
  int ascent = u8g2.getFontAscent();
  int descent = u8g2.getFontDescent();
  return (ascent - descent) + Font::currentLineGap() + 1;
}

// A 7x5 triangle inside the 8px gutter, pointing up or down.
static void drawScrollArrow(int x, int y, bool up) {
  if (up) gfx.fillTriangle(x + 3, y,     x, y + 4, x + 6, y + 4, 1);
  else    gfx.fillTriangle(x + 3, y + 4, x, y,     x + 6, y,     1);
}

void drawScrollableList(int contentTopY, int itemCount, int selectedIndex,
                        const DrawListRowFn& drawRow) {
  s_listGutterActive = false;
  if (itemCount <= 0) return;

  int lineH = menuLineH();

  // A row of N items uses (N-1)*lineH + textHeight pixels — the last row
  // doesn't need a trailing inter-line gap. Crediting that leftover gap
  // lets us fit one more row than `available / lineH` would suggest.
  // `descent` is negative (e.g. -2), so adding it tightens the bound by
  // |descent| to keep descenders on-screen.
  int descent = u8g2.getFontDescent();
  int available = SCREEN_H - BOT_PAD + descent - contentTopY;
  int visibleRows = (available / lineH) + 1;
  if (visibleRows < 1) visibleRows = 1;

  if (selectedIndex < 0) selectedIndex = 0;
  if (selectedIndex >= itemCount) selectedIndex = itemCount - 1;

  // Center the selected item in the visible window, then clamp at the
  // ends so we don't leave blank rows past the list.
  int top = selectedIndex - (visibleRows / 2);
  if (top < 0) top = 0;
  if (top > itemCount - visibleRows) top = max(0, itemCount - visibleRows);

  // Whether the list scrolls at all is known before drawing anything, so the
  // gutter can be reserved in time for the rows to be measured against it.
  s_listGutterActive = (itemCount > visibleRows);

  int y = contentTopY;
  int rowsUsed = 0;
  int idx = top;                        // hoisted: needed after the loop
  for (; idx < itemCount && rowsUsed < visibleRows; idx++) {
    int budget = visibleRows - rowsUsed;
    int consumed = drawRow(idx, y, idx == selectedIndex, budget);
    if (consumed < 1) consumed = 1;
    y += consumed * lineH;
    rowsUsed += consumed;
  }

  if (s_listGutterActive) {
    const int gx = SCREEN_W - MARGIN_X - UI_LIST_GUTTER_W;
    // `top` is clamped above and navigation wraps, so these mean "there are
    // items outside the current window", not "a next page exists".
    if (top > 0) drawScrollArrow(gx, contentTopY - 6, true);
    // Tested against `idx`, not `top + rowsUsed`: a row callback may consume
    // two rows for one item (ListScreen's selected continuation line), so
    // rowsUsed can reach visibleRows having advanced idx fewer times.
    if (idx < itemCount) drawScrollArrow(gx, SCREEN_H - BOT_PAD - 5, false);
  }

  s_listGutterActive = false;
}

void splitListLabelForDisplay(const String& in, int maxWidth, String& line1, String& line2) {
  line1 = in;
  line2 = "";
  if (u8g2.getUTF8Width(in.c_str()) <= maxWidth) return;

  int bestBreak = -1;
  for (int i = 0; i < (int)in.length(); i++) {
    if (in[i] != ' ') continue;
    String left = in.substring(0, i);
    left.trim();
    if (left.length() == 0) continue;
    if (u8g2.getUTF8Width(left.c_str()) <= maxWidth) bestBreak = i;
    else break;
  }

  if (bestBreak < 0) {
    for (int i = 1; i < (int)in.length(); i++) {
      String left = in.substring(0, i);
      if (u8g2.getUTF8Width(left.c_str()) > maxWidth) {
        bestBreak = max(1, i - 1);
        break;
      }
    }
  }

  if (bestBreak < 0) return;

  line1 = in.substring(0, bestBreak);
  line1.trim();
  line2 = in.substring(bestBreak);
  line2.trim();

  // Only line 2 gets an ellipsis. Line 1 already fits by construction, and
  // marking it would read as "Foo... bar" when the text simply continues.
  line2 = fitLabelWithEllipsis(line2, maxWidth);
}
