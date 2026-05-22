#include "src/ui/screens/about_screen.h"

#include "src/hal/display.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"

void AboutScreen::onEnter() {
  draw();
}

void AboutScreen::draw() {
  prepareMenuFrame();
  Font::useBody();
  int ascent = u8g2.getFontAscent();
  int lineH = (ascent - u8g2.getFontDescent()) + Font::currentLineGap() + 1;
  int y = drawSectionHeader(D_ABOUT_HEADER);

  String rows[5] = {
    D_ABOUT_FIRMWARE_PREFIX FW_BUILD,
    D_ABOUT_GESTURE_NEXT,
    D_ABOUT_GESTURE_OPEN,
    D_ABOUT_GESTURE_HOME,
    D_ABOUT_GESTURE_BOOKMARK
  };

  for (int i = 0; i < 5; i++) {
    if (i == 0) Font::useBold();
    else        Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(rows[i].c_str());
    y += lineH;
  }

  display.update();
}

void AboutScreen::onButton(const ButtonEvent& e) {
  if (e.any()) nextScreen = &g_libraryScreen;
}
