#include "src/ui/font.h"

#include <U8g2_for_Adafruit_GFX.h>

#include "src/config.h"
#include "src/hal/display.h"  // u8g2 instance
#include "src/state.h"        // prefs

namespace Font {

// File-private font tables. The rest of the codebase only ever sees the
// role accessors (useBody, useBold, ...).
static const uint8_t* s_body    = u8g2_font_helvR08_te;
static const uint8_t* s_bold    = u8g2_font_helvB08_te;
// _tf = ASCII only. Translated strings must NOT use these — see font.h.
static const uint8_t* s_uiSmall = u8g2_font_6x10_tf;
static const uint8_t* s_uiTiny  = u8g2_font_5x8_tf;
// _te = Latin Extended. Used by toasts so translations with accents render.
static const uint8_t* s_toast   = u8g2_font_helvR08_te;

// Owned settings. `s_size` mirrors which Helvetica size is currently active;
// `s_lineGap` is the user-configurable extra spacing between body lines.
static int s_size    = 10;
static int s_lineGap = 0;

// Layout-metrics cache. Invalid after the mutators below; recomputed on the
// next bodyLayout() call.
static LayoutMetrics s_layout;
static bool          s_layoutValid = false;

// NVS keys — file-private; nothing outside font.cpp should reference them.
static constexpr const char* kKeyBodySize = "cfg_font";
static constexpr const char* kKeyLineGap  = "cfg_lgap";

// In-memory body-size apply: select font tables + invalidate the layout
// cache. Any out-of-set size falls back to 10. Does NOT persist — public
// callers go through setBodySize() / loadSettings().
static void applyBodySize(int sz) {
  switch (sz) {
    case 8:  s_body = u8g2_font_helvR08_te; s_bold = u8g2_font_helvB08_te; break;
    case 10: s_body = u8g2_font_helvR10_te; s_bold = u8g2_font_helvB10_te; break;
    case 12: s_body = u8g2_font_helvR12_te; s_bold = u8g2_font_helvB12_te; break;
    case 14: s_body = u8g2_font_helvR14_te; s_bold = u8g2_font_helvB14_te; break;
    default: s_body = u8g2_font_helvR10_te; s_bold = u8g2_font_helvB10_te; sz = 10; break;
  }
  s_size = sz;
  s_layoutValid = false;
}

// In-memory line-gap apply: clamp + invalidate the layout cache. Does NOT
// persist — public callers go through setLineGap() / loadSettings().
static void applyLineGap(int gap) {
  if (gap < 0) gap = 0;
  if (gap > 4) gap = 4;
  s_lineGap = gap;
  s_layoutValid = false;
}

void useBody()     { u8g2.setFont(s_body); }
void useBold()     { u8g2.setFont(s_bold); }
void useToast()    { u8g2.setFont(s_toast); }
void useUiSmall()  { u8g2.setFont(s_uiSmall); }
void useUiTiny()   { u8g2.setFont(s_uiTiny); }
void useAppLarge() { u8g2.setFont(u8g2_font_helvB14_te); }

const LayoutMetrics& bodyLayout() {
  if (!s_layoutValid) {
    useBody();
    s_layout.ascent  = u8g2.getFontAscent();
    s_layout.descent = u8g2.getFontDescent();
    s_layout.lineH   = (s_layout.ascent - s_layout.descent) + s_lineGap;

    int w = SCREEN_W - (MARGIN_X * 2);
    if (w < 50) w = 50;
    s_layout.maxWidth = w;

    int maxHeight = SCREEN_H - TOP_PAD - BOT_PAD;
    if (SHOW_PROGRESS_BAR || SHOW_PAGE_NUMBER) maxHeight -= STATUS_H;

    s_layout.maxLines = maxHeight / s_layout.lineH;
    if (s_layout.maxLines < 1) s_layout.maxLines = 1;
    s_layoutValid = true;
  }
  return s_layout;
}

void loadSettings() {
  applyBodySize(prefs.getInt(kKeyBodySize, 10));
  applyLineGap(prefs.getInt(kKeyLineGap, 0));
}

void setBodySize(int sz) {
  applyBodySize(sz);
  prefs.putInt(kKeyBodySize, s_size);  // s_size reflects validation fallback
}

void setLineGap(int gap) {
  applyLineGap(gap);
  prefs.putInt(kKeyLineGap, s_lineGap);  // s_lineGap reflects clamp
}

int currentBodySize() { return s_size; }
int currentLineGap()  { return s_lineGap; }

}  // namespace Font
