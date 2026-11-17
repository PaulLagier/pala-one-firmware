#include "src/web/display_settings.h"
#include "src/config.h"
#include "src/state.h"
#include "src/ui/lock.h"
#include "src/ui/statusbar.h"
#include "src/ui/widgets.h"
#include "src/ui/screen.h"

bool handleDisplaySettingsPost() {
  if (!server.hasArg("display_form"))
    return false;
  const String mode = server.arg("statusbar");
  // Do not use toInt(): missing or malformed input otherwise becomes Full.
  if (mode != "0" && mode != "1" && mode != "2") {
    server.send(400, "text/plain; charset=utf-8", D_WEB_SETTINGS_INVALID);
    return true;
  }
  Statusbar::setMode(Statusbar::Mode(mode[0] - '0'));
  Lock::setIconEnabled(server.hasArg("lock_icon"));
  // setMode marks pagination dirty; the reader redraw reuses its canonical
  // byte position. Other display preferences and form cards stay untouched.
  forceNextMenuFrameFull();
  if (g_currentScreen)
    g_currentScreen->draw();
  server.sendHeader("Location", "/settings");
  server.send(303, "text/plain", "");
  return true;
}
