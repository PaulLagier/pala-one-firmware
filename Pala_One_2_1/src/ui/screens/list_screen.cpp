#include "src/ui/screens/list_screen.h"

#include "src/hal/display.h"
#include "src/storage/list_items.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/ui/reader_actions.h"
void ListScreen::onEnter() {
  draw();
}

void ListScreen::draw() {
  prepareMenuFrame();
  Font::useBody();
  int y = drawSectionHeader(D_LIST_HEADER);

  if (!listHasVisibleItems()) {
    drawMenuRow(y, D_LIST_NONE, false);
    display.update();
    return;
  }

  const int strikeYOffset = (u8g2.getFontAscent() - u8g2.getFontDescent()) / 3;
  const int lineH = menuLineH();
  const int maxWidth = SCREEN_W - UI_LIST_LEFT - MARGIN_X;

  auto drawStrike = [&](int rowY, const String& s) {
    int w = u8g2.getUTF8Width(s.c_str());
    gfx.drawFastHLine(UI_LIST_LEFT, rowY - strikeYOffset, w, 1);
  };

  drawScrollableList(y, g_list.count, g_list.selectedIndex,
    [&](int idx, int rowY, bool selected, int budget) {
      String label = String(g_list.items[idx].text);
      bool done = g_list.items[idx].done;
      String line1, line2;

      if (selected) {
        splitListLabelForDisplay(label, maxWidth, line1, line2);
      } else {
        line1 = label;
        while (line1.length() > 0 && u8g2.getUTF8Width(line1.c_str()) > maxWidth) {
          line1.remove(line1.length() - 1);
        }
      }

      drawMenuRow(rowY, line1, selected);
      if (done) drawStrike(rowY, line1);

      if (selected && line2.length() > 0 && budget >= 2) {
        int row2Y = rowY + lineH;
        Font::useBold();
        u8g2.setCursor(UI_LIST_LEFT, row2Y);
        u8g2.print(line2.c_str());
        Font::useBody();
        if (done) drawStrike(row2Y, line2);
        return 2;
      }
      return 1;
    });

  display.update();
}

void ListScreen::onButton(const ButtonEvent& e) {
  if (!listHasVisibleItems()) {
    nextScreen = &g_libraryScreen;
    return;
  }

    if (Gestures::resolveLegacyAction(e, ButtonEvent::Short, ACTION_NEXT)){

      g_list.selectedIndex++;
      if (g_list.selectedIndex >= g_list.count) g_list.selectedIndex = 0;
      draw();
      return;
    }

    if (Gestures::isNonLegacyAction(e, ACTION_PREV)){

      g_list.selectedIndex--;
      if (g_list.selectedIndex < 0) g_list.selectedIndex = g_list.count - 1;
      draw();
      return;
    }

    if (Gestures::resolveLegacyAction(e, ButtonEvent::Long, ACTION_MENU)){
      if (g_list.selectedIndex >= 0 && g_list.selectedIndex < g_list.count) {
        g_list.items[g_list.selectedIndex].done = g_list.items[g_list.selectedIndex].done ? 0 : 1;
        saveListItems();
        draw();
      }
      return;
    }

    if (Gestures::resolveLegacyAction(e, ButtonEvent::Triple, ACTION_HOME)){
      nextScreen = &g_libraryScreen;
      return;
    }
}
