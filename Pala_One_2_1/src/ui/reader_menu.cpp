#include "src/ui/reader_menu.h"

#include "src/config.h"           // MARGIN_X, SCREEN_W, SCREEN_H
#include "src/hal/display.h"      // u8g2, display
#include "src/pure/paths.h"       // lastPathComponent, stripTxtExt
#include "src/ui/font.h"
#include "src/ui/reader.h"        // g_bookview
#include "src/ui/statusbar.h"
#include "src/ui/widgets.h"       // prepareMenuFrame, drawSectionHeader
#include "src/ui/reader_actions.h"

namespace ReaderMenu {

static bool s_active = false;

static const char* statusbarLabel() {
  switch (Statusbar::mode()) {
    case Statusbar::Minimal: return "Minimal";
    case Statusbar::Hidden:  return "Hidden";
    case Statusbar::Full:
    default:                 return "Full";
  }
}

bool isActive() { return s_active; }

void open() {
  s_active = true;
  // First paint of the menu sits over whatever the reader last drew. Use a
  // full refresh so reader text doesn't ghost through the partial-refresh
  // overlay; subsequent in-menu redraws (statusbar cycles) go through
  // prepareMenuFrame's normal fastmode path for snappy feedback.
  forceNextMenuFrameFull();
  draw();
}

void close() {
  s_active = false;
}

void draw() {
  prepareMenuFrame();
  Font::useBody();
  int ascent = u8g2.getFontAscent();
  int lineH = (ascent - u8g2.getFontDescent()) + Font::currentLineGap() + 1;
  int y = drawSectionHeader("Reading");

  // Book title (truncated by font; the screen clip handles overflow).
  String title = stripTxtExt(lastPathComponent(g_bookview.book.path()));
  Font::useBold();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(title.c_str());
  y += lineH;
  Font::useBody();

  size_t total = g_bookview.book.size();
  if (total == 0) total = 1;
  uint32_t pos = g_bookview.pages.offsets[g_bookview.cursor.pageIndex];
  int pct = (int)((pos * 100UL) / (uint32_t)total);

  // pages.count grows lazily as the reader walks the book; only show it as
  // a total when pagination has actually reached EOF. Otherwise the "of Y"
  // reads as a hard cap rather than "this is how many pages exist."
  char buf[64];
  if (g_bookview.pages.eofReached && g_bookview.pages.count > 0) {
    snprintf(buf, sizeof(buf), "Page %d of %d  (%d%%)",
             g_bookview.cursor.pageIndex + 1, g_bookview.pages.count, pct);
  } else {
    snprintf(buf, sizeof(buf), "Page %d  (%d%% of book)",
             g_bookview.cursor.pageIndex + 1, pct);
  }
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(buf);
  y += lineH;

  // Blank gap so the settings section reads as separate from progress info.
  y += lineH;

  // Statusbar mode row — short-click cycles through the three modes.
  snprintf(buf, sizeof(buf), "Statusbar: %s", statusbarLabel());
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(buf);

  // Footer hint at the very bottom.
  u8g2.setCursor(MARGIN_X, SCREEN_H - 2);
  u8g2.print("click: cycle  2x: close");

  display.update();
}

bool onButton(const ButtonEvent& ev) {
  if (!s_active) return false;
  if (!ev.any()) return false;

  if (Gestures::resolveLegacyAction(ev, ButtonEvent::Short, ACTION_NEXT)) {
    Statusbar::cycleMode();
    draw();
    return true;
  }

  // Any other click closes — caller redraws the underlying page.
  close();
  return true;
}

}  // namespace ReaderMenu
