#include "src/ui/toast.h"

#include "src/config.h"        // TOAST_MS, MARGIN_X, SCREEN_H, SCREEN_W, STATUS_H
#include "src/hal/display.h"   // u8g2 + gfx
#include "src/ui/font.h"

namespace Toast {

// File-private state. Nothing outside this TU references it.
static String   s_msg;
static uint32_t s_untilMs = 0;  // 0 == not active

static bool isExpired() {
  return s_untilMs != 0 && (int32_t)(millis() - s_untilMs) > 0;
}

void show(const String& msg) {
  s_msg = msg;
  s_untilMs = millis() + TOAST_MS;
}

bool isActive() {
  return s_untilMs != 0 && !isExpired();
}

bool clearIfExpired() {
  if (!isExpired()) return false;
  s_msg = "";
  s_untilMs = 0;
  return true;
}

void reset() {
  s_msg = "";
  s_untilMs = 0;
}

void draw() {
  if (!isActive()) return;

  const int yTop = SCREEN_H - STATUS_H;
  gfx.fillRect(0, yTop, SCREEN_W, STATUS_H, 0);

  Font::useToast();   // Latin Extended — translated strings may carry accents
  u8g2.setForegroundColor(1);  // guard: ensure black text regardless of prior state
  int textY = SCREEN_H - 1;
  u8g2.setCursor(MARGIN_X, textY);
  u8g2.print(s_msg.c_str());
  Font::useBody();
}

}  // namespace Toast
