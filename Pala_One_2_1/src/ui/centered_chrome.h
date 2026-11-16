#pragma once
#include "src/ui/pala_chrome.h"

// Center the final fitted string, including its ellipsis, rather than the
// original width. Bounds are symmetric around the physical display center.
namespace CenteredChrome {
inline int centered(U8G2_FOR_ADAFRUIT_GFX &font, const char *text, int baseline, int width = 216,
                    int center = 125) {
  return PalaChrome::centered(font, text, baseline, width, center);
}
inline void header(U8G2_FOR_ADAFRUIT_GFX &font, const char *title, int baseline = 18) {
  PalaChrome::header(font, title, baseline);
}
inline void card(Adafruit_GFX &gfx, U8G2_FOR_ADAFRUIT_GFX &font, int top, const char *title,
                 bool selected) {
  PalaChrome::card(gfx, font, top, title, selected);
}
inline void action(Adafruit_GFX &gfx, U8G2_FOR_ADAFRUIT_GFX &font, int x, const char *text,
                   bool selected) {
  PalaChrome::splitAction(gfx, font, x, text, selected);
}
inline void message(Adafruit_GFX &gfx, U8G2_FOR_ADAFRUIT_GFX &font, const char *text,
                    const char *second = nullptr) {
  PalaChrome::message(gfx, font, text, second);
}
inline void line(U8G2_FOR_ADAFRUIT_GFX &font, const char *text, int baseline, int width = 216) {
  centered(font, text, baseline, width);
}
// Settings share the three-row geometry and separated label/value columns.
constexpr int settingRows = PalaChrome::visibleCards;
constexpr int settingTop = PalaChrome::firstCardTop;
constexpr int settingStep = PalaChrome::cardStep;
inline void tallCard(Adafruit_GFX &gfx, U8G2_FOR_ADAFRUIT_GFX &font, int top, const char *title,
                     bool selected) {
  PalaChrome::menuCard(gfx, font, top, title, selected);
}
inline void settingCard(Adafruit_GFX &gfx, U8G2_FOR_ADAFRUIT_GFX &font, int top, const char *label,
                        const char *value, bool selected) {
  PalaChrome::settingCard(gfx, font, top, label, value, selected);
}
inline void tallChoice(Adafruit_GFX &gfx, U8G2_FOR_ADAFRUIT_GFX &font, int top, const char *label,
                       bool selected, bool checked) {
  PalaChrome::menuChoice(gfx, font, top, label, selected, checked);
}
template <class Label>
inline void list(Adafruit_GFX &gfx, U8G2_FOR_ADAFRUIT_GFX &font, const char *title, int count,
                 int selected, Label label) {
  PalaChrome::menuList(gfx, font, title, count, selected, label);
}
} // namespace CenteredChrome
