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

static String actionLabel(ButtonAction action) {
  switch (action) {
    case ACTION_BOOKMARK:
      return D_ACTION_BOOKMARK_LABEL;
    case ACTION_LOCK:
      return D_ACTION_LOCK_LABEL;
    case ACTION_MENU:
      return D_ACTION_MENU_LABEL;
    case ACTION_ROTATE:
      return D_ACTION_ROTATE_LABEL;
    case ACTION_NONE:
    default:
      return D_ACTION_NONE_LABEL;
    }
}



void AboutScreen::draw() {
  prepareMenuFrame();
  Font::useUiSmall();
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

  // Left padding to align all the action labels
  int leftPad = MARGIN_X;

  for (int i = 0; i < rowNumber; i++) {
    Font::useBold();
    int leftStrWidth = MARGIN_X;
    
    leftStrWidth += u8g2.getUTF8Width(rows[i][0].c_str());
    leftStrWidth += u8g2.getUTF8Width(D_ABOUT_GESTURE_SEPARATOR);
    if (leftStrWidth > leftPad) {
      leftPad = leftStrWidth;
    }
  }

    for (int i = 0; i < rowNumber; i++)
    {
      // if (i == 0) Font::useBold();
      // else        Font::useBody();
      int x = MARGIN_X;
      u8g2.setCursor(x, y);
      Font::useBold();
      u8g2.print(rows[i][0].c_str());
      x += u8g2.getUTF8Width(rows[i][0].c_str()) + 1;
      u8g2.setCursor(x, y);
      u8g2.print(D_ABOUT_GESTURE_SEPARATOR);
      x += u8g2.getUTF8Width(D_ABOUT_GESTURE_SEPARATOR) + 1;
      Font::useBody();
      if (i == 0) {
        u8g2.setCursor(x, y);
      }
      else {
        u8g2.setCursor(leftPad, y);
      }
      u8g2.print(rows[i][1].c_str());

      y += lineH;
    }

  display.update();
}

void AboutScreen::onButton(const ButtonEvent& e) {
  if (e.any()) nextScreen = &g_libraryScreen;
}
