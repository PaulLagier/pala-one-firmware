#ifndef PALA_HAL_DISPLAY_H
#define PALA_HAL_DISPLAY_H

#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "src/config.h"
#include "src/state.h"

bool displayFlipped();
void setDisplayFlipped(bool flipped);
void loadDisplaySettings();

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
#if PALA_DIAGNOSTICS
    size_t idx = (size_t)y * 32 + x / 8;
    uint8_t mask = 0x80 >> (x % 8);
    if (color) capture_[idx] |= mask;
    else capture_[idx] &= ~mask;
#endif
    uint16_t c = color ? BLACK : WHITE;
    int16_t xx = displayFlipped() ? x : (SCREEN_W - 1) - x;
    int16_t yy = displayFlipped() ? y : (SCREEN_H - 1) - y;
    disp.drawPixel(xx, yy, c);
  }

#if PALA_DIAGNOSTICS
  void clearCapture() { memset(capture_, 0, sizeof(capture_)); }
  bool capturePixel(int x,int y)const{if(x<0||y<0||x>=SCREEN_W||y>=SCREEN_H)return false;return (capture_[size_t(y)*32+x/8]&(0x80>>(x%8)))!=0;}
  uint32_t captureHash() const {
    uint32_t hash = 2166136261UL;
    for (uint8_t byte : capture_) hash = (hash ^ byte) * 16777619UL;
    return hash;
  }
  void dumpCapture() {
    Serial.println("[snapshot] 250 122");
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < sizeof(capture_); ++i) {
      Serial.write(hex[capture_[i] >> 4]); Serial.write(hex[capture_[i] & 15]);
      if (i % 32 == 31) Serial.println();
    }
    Serial.println("[/snapshot]");
  }
#endif
private:
#if PALA_DIAGNOSTICS
  uint8_t capture_[32 * 122] = {};
#endif
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
// Retain controller configuration between frames of the same refresh mode.
void setDisplayRefreshMode(bool full);
#if PALA_DIAGNOSTICS
void forceDisplayModeSetupForTest(bool force);
#endif

#endif  // PALA_HAL_DISPLAY_H
