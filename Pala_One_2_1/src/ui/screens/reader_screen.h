#ifndef PALA_UI_SCREENS_READER_SCREEN_H
#define PALA_UI_SCREENS_READER_SCREEN_H

#include "src/ui/screen.h"

class ReaderScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;
  void onSleep() override;
  void onIdleTick() override;
};

extern ReaderScreen g_readerScreen;

#endif  // PALA_UI_SCREENS_READER_SCREEN_H
