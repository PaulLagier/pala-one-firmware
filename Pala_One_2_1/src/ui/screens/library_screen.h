#ifndef PALA_UI_SCREENS_LIBRARY_SCREEN_H
#define PALA_UI_SCREENS_LIBRARY_SCREEN_H

#include "src/ui/screen.h"

class LibraryScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent &e) override;
  void draw() override;
};

extern LibraryScreen g_libraryScreen;

// Request a deferred transition to the library root from any context where
// `g_currentScreen` is set (i.e. mid-iteration of `loop()`). The actual
// state cleanup runs in `LibraryScreen::onEnter()` on the next dispatch.
void navigateToLibraryRoot();

// Full reset of the library screen's nav state: cursor to 0, all folders
// collapsed. Used by factory reset; not used on ordinary library entry
// (which preserves folder expansion).
void resetLibraryNav();

#endif // PALA_UI_SCREENS_LIBRARY_SCREEN_H
