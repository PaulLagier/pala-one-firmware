#ifndef PALA_UI_SCREENS_BOOKMARKS_PREVIEW_SCREEN_H
#define PALA_UI_SCREENS_BOOKMARKS_PREVIEW_SCREEN_H

#include "src/ui/screen.h"

class BookmarkPreviewScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;
  void onSleep() override;
private:
  bool readSelected_=true;
  void cancel();
};

extern BookmarkPreviewScreen g_bmPreviewScreen;

#endif  // PALA_UI_SCREENS_BOOKMARKS_PREVIEW_SCREEN_H
