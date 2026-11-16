#include "src/lang/plural.h"
#include "src/ui/screens/statistics_screen.h"
#include "src/config.h"
#include "src/hal/display.h"
#include "src/ui/font.h"
#include "src/ui/pala_chrome.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/storage/navigation_profile.h"
#include "src/ui/screens/settings_screen.h"

namespace {
void face(const uint8_t *font) {
  u8g2.setFont(font);
  u8g2.setFontMode(1);
  u8g2.setForegroundColor(1);
  u8g2.setBackgroundColor(0);
}
void text(const char *value, int x, int baseline) {
  u8g2.setCursor(x, baseline);
  u8g2.print(value);
}
// Fall back to compact, ungrouped digits rather than losing a counter's precision.
void metric(int x, const char *label, uint64_t value) {
  char number[24];
  snprintf(number, sizeof(number), "%llu", (unsigned long long)value);
  gfx.drawRoundRect(x, 92, 116, 25, 12, 1);
  face(pala_font_sans_r08);
  const int labelWidth = u8g2.getUTF8Width(label);
  face(pala_font_sans_b08);
  const int valueWidth = u8g2.getUTF8Width(number);
  if (labelWidth + valueWidth + 4 <= 94) {
    text(number, x + 105 - valueWidth, 108);
    face(pala_font_sans_r08);
    text(label, x + 11, 107);
  } else {
    face(pala_font_sans_r08);
    PalaChrome::fitted(u8g2, label, x + 11, 102, 94);
    face(pala_font_sans_b08);
    if (u8g2.getUTF8Width(number) > 96)
      face(pala_font_ui_tiny08);
    text(number, x + 105 - u8g2.getUTF8Width(number), 113);
  }
}
void smallMetric(const char *label, uint32_t value, int baseline) {
  char number[16];
  snprintf(number, sizeof(number), "%u", unsigned(value));
  face(pala_font_sans_b10);
  if (u8g2.getUTF8Width(number) > 43)
    face(pala_font_ui_tiny08);
  const int width = u8g2.getUTF8Width(number);
  text(number, 233 - width, baseline);
  face(pala_font_sans_r08);
  PalaChrome::fitted(u8g2, label, 143, baseline - 1, 233 - width - 148);
}
} // namespace
void StatisticsScreen::onEnter() {
  nextScreen = nullptr;
  snapshot_ = Statistics::snapshot();
  draw();
}
void StatisticsScreen::draw() {
  prepareMenuFrame();
  drawBackSectionHeader(D_STATS_HEADING);
  gfx.drawRoundRect(5, 31, 240, 40, 12, 1);
  gfx.drawFastVLine(132, 40, 24, 1);
  char number[16];
  snprintf(number, sizeof(number), "%u", unsigned(snapshot_.currentStreak));
  face(pala_font_ui_digits29);
  if (u8g2.getUTF8Width(number) > 35)
    face(pala_font_sans_b14);
  if (u8g2.getUTF8Width(number) > 35)
    face(pala_font_sans_b10);
  if (u8g2.getUTF8Width(number) <= 35) {
    text(number, 17, 61);
    face(pala_font_sans_r08);
    PalaChrome::fitted(u8g2, Language::days(snapshot_.currentStreak), 58, 47, 66);
    PalaChrome::fitted(u8g2, D_STATS_IN_ROW, 58, 59, 66);
  } else {
    face(pala_font_sans_b10);
    text(number, 17, 47);
    face(pala_font_sans_r08);
    const String unit = String(Language::days(snapshot_.currentStreak)) + " " + D_STATS_IN_ROW;
    PalaChrome::fitted(u8g2, unit.c_str(), 17, 63, 106);
  }
  smallMetric(D_STATS_RECORD_LABEL, snapshot_.longestStreak, 47);
  smallMetric(D_STATS_SESSIONS_LABEL, snapshot_.totalSessions, 62);
  for (int day = 0; day < 30; ++day) {
    const int x = 9 + day * 8;
    if ((snapshot_.bitmap >> (29 - day)) & 1u)
      gfx.fillCircle(x, 81, 3, 1);
    else
      gfx.drawCircle(x, 81, 3, 1);
  }
  metric(5, D_STATS_PAGE_LABEL, snapshot_.pagesRead);
  metric(129, D_STATS_PRESSES, snapshot_.buttonPresses);
  Font::useBody();
  display.update();
}
void StatisticsScreen::onButton(const ButtonEvent &event) {
  if (!event.any() || event.kind == ButtonEvent::Short)
    return;
  nextScreen = NavigationProfile::readingFocused() ? static_cast<Screen *>(&g_settingsScreen)
                                                   : static_cast<Screen *>(&g_libraryScreen);
}
