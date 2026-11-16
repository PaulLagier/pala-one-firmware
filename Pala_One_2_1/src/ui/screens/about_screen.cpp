#include "src/ui/screens/about_screen.h"
#include "src/hal/display.h"
#include "src/ui/font.h"
#include "src/ui/reader_actions.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screens/settings_screen.h"
#include "src/ui/widgets.h"
#include "src/ui/pala_chrome.h"
#include "src/storage/navigation_profile.h"

namespace {
const char *actionLabel(ButtonAction action) {
  switch (action) {
  case ACTION_ROTATE:
    return D_ACTION_ROTATE_LABEL;
  case ACTION_BOOKMARK:
    return D_ABOUT_ACTION_BOOKMARK;
  case ACTION_LOCK:
    return D_ABOUT_ACTION_LOCK;
  case ACTION_MENU:
    return D_ABOUT_ACTION_MENU;
  default:
    return D_ABOUT_ACTION_NONE;
  }
}
} // namespace
void AboutScreen::onEnter() {
  nextScreen = nullptr;
  page_ = Page::Firmware;
  selection_ = 0;
  helpPage_ = 0;
  draw();
}
void AboutScreen::draw() {
  prepareMenuFrame();
  Font::useMenu();
  if (page_ == Page::Menu) {
    drawPalaList(D_ABOUT_HEADER, 3, selection_, [](int index) -> String {
      if (index == 0)
        return D_ABOUT_FIRMWARE;
      if (index == 1)
        return D_ABOUT_CONTROLS;
      return D_READER_BACK_LIBRARY;
    });
  } else {
    String first, second;
    if (page_ == Page::Firmware) {
      drawBackSectionHeader(D_ABOUT_FIRMWARE);
#if defined(DISPLAY_V1_1)
      first = "Pala One · V1.1";
#else
      first = "Pala One · V1.2";
#endif
      second = FW_VERSION;
    } else {
      drawBackSectionHeader(D_ABOUT_CONTROLS);

      switch (helpPage_) {
      case 0:
        first = D_ABOUT_MENU_NEXT;
        second = D_ABOUT_MENU_OPEN;
        break;
      case 1:
        first = D_ABOUT_READER_NEXT;
        second = D_ABOUT_READER_PREVIOUS;
        break;
      case 2:
        first = D_ABOUT_GESTURE_HOME;
        second = String(D_ABOUT_HOLD_PREFIX) + actionLabel(Gestures::actionLong());
        break;
      default:
        first = String(D_ABOUT_LONG_HOLD_PREFIX) + actionLabel(Gestures::actionExtraLong());
        second = String(D_ABOUT_CLICK_HOLD_PREFIX) + actionLabel(Gestures::actionClickHold());
        break;
      }
    }
    PalaChrome::message(gfx, u8g2, first.c_str(), second.c_str());
    Font::useBody();
  }
  display.update();
}
void AboutScreen::onButton(const ButtonEvent &e) {
  if (!e.any())
    return;
  if (e.kind == ButtonEvent::Triple) {
    nextScreen = &g_libraryScreen;
    return;
  }
  if (e.kind == ButtonEvent::Short) {
    if (page_ == Page::Firmware) {
      page_ = Page::Controls;
      helpPage_ = 0;
    } else if (++helpPage_ >= 4) {
      page_ = Page::Firmware;
      helpPage_ = 0;
    }
    draw();
    return;
  }
  if (e.kind == ButtonEvent::Long || e.kind == ButtonEvent::Double)
    nextScreen = NavigationProfile::readingFocused() ? static_cast<Screen *>(&g_settingsScreen)
                                                     : static_cast<Screen *>(&g_libraryScreen);
}
