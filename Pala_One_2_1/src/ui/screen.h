#ifndef PALA_UI_SCREEN_H
#define PALA_UI_SCREEN_H

#include "src/hal/input.h"           // ButtonState + ButtonEvent
#include "src/state.h"

class Screen {
public:
  virtual ~Screen() = default;
  virtual void onEnter() {}
  virtual void onButton(const ButtonEvent& e) = 0;
  virtual void draw() = 0;
  virtual void onIdleTick() {}

  // Called once just before the device deep-sleeps. Default: no-op — wake
  // state is empty during runtime (consumed at boot) and stays that way
  // unless ReaderScreen::onSleep explicitly sets it. Screens that need to
  // do per-sleep work (reader: save progress + arm wake; preview: close
  // open book) override this.
  virtual void onSleep() {}

  // May the device deep-sleep while this screen is active? Default yes;
  // UploadScreen overrides to false because a sleeping device can't keep its
  // Wi-Fi session (AP or STA) up.
  virtual bool allowSleep() const { return true; }

  Screen* nextScreen = nullptr;
};

extern Screen* g_currentScreen;

#endif  // PALA_UI_SCREEN_H
