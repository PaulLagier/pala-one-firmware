#include "src/ui/centered_chrome.h"
#include "src/ui/pala_chrome.h"
#include "src/ui/screens/apps_screen.h"

#include "src/hal/display.h"
#include "src/storage/app_catalog.h"
#include "src/ui/font.h"
#include "src/ui/pala_api_impl.h"          // runApp
#include "src/ui/screens/library_screen.h" // g_libraryScreen
#include "src/ui/widgets.h"

// Cursor persists across visits — entering Apps doesn't reset to 0,
// matching how LibraryScreen / ListScreen behave.
static int s_cursor = 0;

void AppsScreen::onEnter() {
  // Re-scan the catalog on entry so newly uploaded apps appear without
  // a reboot. Cheap (one /apps/ directory walk).
  loadApps();
  if (s_cursor < 0) s_cursor = 0;
  if (s_cursor > g_apps.count) s_cursor = g_apps.count;
  draw();
}

void AppsScreen::draw() {
  prepareMenuFrame();
  if(g_apps.count==0){drawSectionHeader("");CenteredChrome::header(u8g2,D_APPS_HEADER);CenteredChrome::message(gfx,u8g2,D_APPS_NONE);CenteredChrome::card(gfx,u8g2,89,D_READER_BACK_LIBRARY,true);display.update();return;}
  drawSectionHeader("");
  CenteredChrome::list(gfx,u8g2,D_APPS_HEADER, g_apps.count + 1, s_cursor,
    [](int idx) { return idx == g_apps.count ? String(D_READER_BACK_LIBRARY)
      : String(g_apps.entries[idx].name); });

  display.update();
}

void AppsScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;

  if (e.kind == ButtonEvent::Triple) {
    nextScreen = &g_libraryScreen;
    return;
  }

  if (e.kind == ButtonEvent::Short) {
    s_cursor = (s_cursor + 1) % (g_apps.count + 1);
    draw();
    return;
  }

  if (e.kind == ButtonEvent::Double || e.kind == ButtonEvent::Long) {
    if (s_cursor == g_apps.count) { nextScreen = &g_libraryScreen; return; }
    if (s_cursor < 0 || s_cursor >= g_apps.count) return;

    // runApp blocks for the entire app lifetime; when it returns the
    // app's framebuffer is whatever the app last drew. Repaint our menu
    // so the screen reflects "back in Apps" again.
    const char* path = g_apps.entries[s_cursor].path;
    runApp(path);
    draw();
    return;
  }


}
