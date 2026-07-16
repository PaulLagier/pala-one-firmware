#include "src/ui/screens/about_screen.h"

#include "src/hal/display.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/ui/reader_actions.h"
#include "src/hal/battery.h"

void AboutScreen::onEnter() {
  draw();
}

void AboutScreen::draw() {
  prepareMenuFrame();
  Font::useBody();
  int ascent = u8g2.getFontAscent();
  int lineH = (ascent - u8g2.getFontDescent()) + Font::currentLineGap() + 3;
  int y = drawSectionHeader(D_ABOUT_HEADER, false);
  drawBatteryTopRight(true);
  const int rowNumber = 7;
  String rows[rowNumber][2] = {
      {D_ABOUT_FIRMWARE_PREFIX, FW_VERSION},
      {D_ABOUT_GESTURE_CLICK, D_ACTION_NEXT_LABEL},
      {D_ABOUT_GESTURE_CLICK_2, D_ACTION_OPEN_LABEL},
      {D_ABOUT_GESTURE_CLICK_3, D_ACTION_HOME_LABEL},
      {D_ABOUT_GESTURE_HOLD, actionLabel(Gestures::actionLong())},
      {D_ABOUT_GESTURE_LONG_HOLD, actionLabel(Gestures::actionExtraLong())},
      {D_ABOUT_GESTURE_CLICK_HOLD, actionLabel(Gestures::actionClickHold())},
  };

  String rows[5] = {
    D_ABOUT_FIRMWARE_PREFIX FW_VERSION,
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
  if (Gestures::legacyControlsOn()){
    if (e.any()) nextScreen = &g_libraryScreen;
  }
  else {
    if (Gestures::isNonLegacyAction(e, ACTION_HOME)) {
      nextScreen = &g_libraryScreen;
    }
  }}
