#include "src/hal/display.h"

namespace { bool s_displayFlipped = false; }
bool displayFlipped() { return s_displayFlipped; }
void loadDisplaySettings() { s_displayFlipped = prefs.getBool("cfg_scr_flip", false); }
void setDisplayFlipped(bool flipped) {
  if (flipped == s_displayFlipped) return;
  s_displayFlipped = flipped;
  prefs.putBool("cfg_scr_flip", flipped);
}
U8G2_FOR_ADAFRUIT_GFX u8g2;
HeltecGFXAdapter gfx(display);

// ============================================================================
//  Drawing primitives
// ============================================================================
void beginPageCanvas(bool clearMem) {
  if (clearMem) {
    display.clearMemory();
#if PALA_DIAGNOSTICS
    gfx.clearCapture();
#endif
  }
  display.landscape();
  u8g2.setFontMode(1);
  u8g2.setForegroundColor(1);
  u8g2.setBackgroundColor(0);
}

namespace {
int s_refreshMode = -1;
#if PALA_DIAGNOSTICS
bool s_forceModeSetup = false;
#endif
}
#if PALA_DIAGNOSTICS
extern bool pala_eink_burst_enabled;
void forceDisplayModeSetupForTest(bool force) {
  s_forceModeSetup = force;
  pala_eink_burst_enabled = !force;
}
#endif
void setDisplayRefreshMode(bool full) {
  const int mode = full ? 1 : 0;
  bool force = false;
#if !defined(DISPLAY_V1_2)
  // Reuse is validated on the connected V1.2 panel. Keep the V1.1 driver
  // behavior unchanged until the same hardware test can be performed.
  force = true;
#endif
#if PALA_DIAGNOSTICS
  force = force || s_forceModeSetup;
#endif
  if (!force && s_refreshMode == mode) return;
  if (full) display.fastmodeOff();
  else display.fastmodeOn();
  s_refreshMode = mode;
}
