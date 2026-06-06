#include "src/ui/screens/statistics_screen.h"

#include "src/config.h"               // MARGIN_X, SCREEN_W, STATUS_H
#include "src/hal/display.h"
#include "src/pure/streak_log.h"      // STREAK_DAY_UNSET
#include "src/storage/statistics.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"

void StatisticsScreen::onEnter() {
  draw();
}

void StatisticsScreen::draw() {
  StatisticsSnapshot s = Statistics::snapshot();

  prepareMenuFrame();
  Font::useBody();
  int ascent = u8g2.getFontAscent();
  int lineH  = (ascent - u8g2.getFontDescent()) + Font::currentLineGap() + 1;
  int y = drawSectionHeader(D_STATS_HEADING);

  char buf[64];

  // Row 1 (bold): current streak.
  snprintf(buf, sizeof(buf), D_STATS_STREAK_CURRENT_FMT, (unsigned)s.currentStreak);
  Font::useBold();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(buf);
  Font::useBody();
  y += lineH;

  // Row 2: longest streak + total sessions on one line if it fits, else two.
  snprintf(buf, sizeof(buf), D_STATS_STREAK_LONGEST_FMT,
           (unsigned)s.longestStreak, (unsigned)s.totalSessions);
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(buf);
  y += lineH;

  // Row 3: lifetime pages.
  snprintf(buf, sizeof(buf), D_STATS_LIFETIME_PAGES_FMT, (unsigned long long)s.pagesRead);
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(buf);
  y += lineH;

  // Row 4: lifetime button presses.
  snprintf(buf, sizeof(buf), D_STATS_LIFETIME_PRESSES_FMT, (unsigned long long)s.buttonPresses);
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(buf);

  // Bottom row: 30-day bitmap. Each cell = 1 day. Right-most cell = today
  // (bit 0). A filled rect means "logged that day"; an outlined rect means
  // "not logged". Sits in the bottom strip above the statusbar reserve.
  static const int CELLS = 30;
  const int cellW = 6;
  const int cellH = 6;
  const int totalW = CELLS * cellW;
  const int x0 = (SCREEN_W - totalW) / 2;
  const int yTop = SCREEN_H - STATUS_H - cellH - 1;

  for (int i = 0; i < CELLS; i++) {
    // i = 0 is the leftmost cell (oldest, 29 days ago); CELLS-1 is rightmost (today).
    int bit = (CELLS - 1) - i;
    bool logged = (s.bitmap >> bit) & 1u;
    int x = x0 + i * cellW;
    if (logged) gfx.fillRect(x, yTop, cellW - 1, cellH, 1);
    else        gfx.drawRect(x, yTop, cellW - 1, cellH, 1);
  }

  display.update();
}

void StatisticsScreen::onButton(const ButtonEvent& e) {
  if (e.any()) nextScreen = &g_libraryScreen;
}
