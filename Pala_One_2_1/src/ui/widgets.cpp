#include "src/ui/widgets.h"

#include "src/hal/battery.h"
#include "src/hal/display.h"
#include "src/ui/font.h"
#include "src/ui/pala_chrome.h"


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
    setDisplayRefreshMode(true);
    s_menuDrawsSinceFull = 0;
  } else {
    setDisplayRefreshMode(false);
  }
  beginPageCanvas();
  s_menuDrawsSinceFull++;
}

void drawCenter(const char* a, const char* b) {
  setDisplayRefreshMode(true);
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

int drawSectionHeader(const char* title) {
  PalaChrome::header(u8g2, title);
#if HAS_BATTERY
  drawBatteryTopRight();
#endif
  Font::useMenu();
  return 42;
}

int drawBackSectionHeader(const char* title, bool focused) {
  PalaChrome::backHeader(gfx,u8g2,title,focused);
#if HAS_BATTERY
  drawBatteryTopRight();
#endif
  Font::useMenu();
  return 32;
}

void drawMenuRow(int yBaseline, const String& label, bool selected, int extraIndent) {
  u8g2.setForegroundColor(1);
  if (selected) Font::useMenuBold();
  else          Font::useMenu();
  u8g2.setCursor(UI_LIST_LEFT + extraIndent, yBaseline);
  u8g2.print(label.c_str());
  Font::useMenu();
}

int menuLineH() {
  int ascent = u8g2.getFontAscent();
  int descent = u8g2.getFontDescent();
  return (ascent - descent) + 1;
}

void drawScrollableList(int contentTopY, int itemCount, int selectedIndex,
                        const DrawListRowFn& drawRow) {
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

  int y = contentTopY;
  int rowsUsed = 0;
  for (int idx = top; idx < itemCount && rowsUsed < visibleRows; idx++) {
    int budget = visibleRows - rowsUsed;
    int consumed = drawRow(idx, y, idx == selectedIndex, budget);
    if (consumed < 1) consumed = 1;
    y += consumed * lineH;
    rowsUsed += consumed;
  }
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
    for (int i = 1; i <= (int)in.length(); i++) {
      if (i < (int)in.length() && (static_cast<unsigned char>(in[i]) & 0xC0) == 0x80) continue;
      String left = in.substring(0, i);
      if (u8g2.getUTF8Width(left.c_str()) > maxWidth) {
        bestBreak = i;
        do { --bestBreak; } while (bestBreak > 0 && (static_cast<unsigned char>(in[bestBreak]) & 0xC0) == 0x80);
        break;
      }
    }
  }

  if (bestBreak < 0) return;

  line1 = in.substring(0, bestBreak);
  line1.trim();
  line2 = in.substring(bestBreak);
  line2.trim();

  while (line2.length() > 0 && u8g2.getUTF8Width(line2.c_str()) > maxWidth) {
    int end=line2.length()-1;
    while(end>0&&(static_cast<unsigned char>(line2[end])&0xC0)==0x80)--end;
    line2.remove(end);
  }
}

void drawPalaList(const char* title, int itemCount, int selectedIndex,
                  const std::function<String(int)>& label) {
  PalaChrome::menuList(gfx,u8g2,title,itemCount,selectedIndex,label);
#if HAS_BATTERY
  drawBatteryTopRight();
#endif
  Font::useBody();
}
