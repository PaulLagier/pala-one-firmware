#ifndef PALA_HAL_DISPLAY_H
#define PALA_HAL_DISPLAY_H

#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "src/config.h"
#include "src/state.h"
#include "src/ui/screen_settings.h"

// ============================================================================
//  Display adapter — wraps the Heltec EInk display so Adafruit_GFX can draw
//  to it. Rotates the screen 180° because of the panel orientation.
// ============================================================================
class HeltecGFXAdapter : public Adafruit_GFX {
public:
  explicit HeltecGFXAdapter(EInkDisplay& d)
    : Adafruit_GFX(SCREEN_W, SCREEN_H), disp(d) {}

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return;
    uint16_t c = color ? BLACK : WHITE;
    int16_t xx = (SCREEN_W - 1) - x;
    int16_t yy = (SCREEN_H - 1) - y;
    if (!ScreenSettings::isScreenFlipped())
    {
      disp.drawPixel(xx, yy, c);
    }
    else {
      disp.drawPixel(x, y, c);
    }
  }

private:
  EInkDisplay& disp;
};

extern HeltecGFXAdapter gfx;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

// ============================================================================
//  Drawing primitives
// ============================================================================

// Set up the device for any drawing pass: clear the offscreen buffer (unless
// the caller has already managed that) and put u8g2 into the project's
// transparent-foreground mode. Higher-level helpers (drawCenter,
// prepareMenuFrame in ui/widgets.h) call this internally.
void beginPageCanvas(bool clearMem = true);

#endif  // PALA_HAL_DISPLAY_H
