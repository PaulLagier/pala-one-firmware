#include "src/ui/screens/statistics_screen.h"

#include "src/config.h"               // MARGIN_X, SCREEN_W, STATUS_H
#include "src/hal/display.h"
#include "src/pure/streak_log.h"      // STREAK_DAY_UNSET
#include "src/storage/statistics.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"

static const int STATS_PAGE_COUNT = 2;

// Format a duration in seconds as a compact "Hh Mm" (or just "Mm" under an
// hour). Seconds are dropped — page-granularity reading time doesn't need
// them, and the e-ink rows are narrow.
static void fmtDuration(char* out, size_t n, uint64_t secs) {
  uint64_t mins = secs / 60;
  uint64_t hours = mins / 60;
  unsigned m = (unsigned)(mins % 60);
  if (hours > 0) snprintf(out, n, "%lluh %um", (unsigned long long)hours, m);
  else           snprintf(out, n, "%um", m);
}

void StatisticsScreen::onEnter() {
  page_ = 0;
  draw();
}

void StatisticsScreen::draw() {
  StatisticsSnapshot s = Statistics::snapshot();
  if (page_ == 0) drawStreakPage(s);
  else            drawTimePage(s);
}

void StatisticsScreen::drawStreakPage(const StatisticsSnapshot& s) {
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

void StatisticsScreen::drawTimePage(const StatisticsSnapshot& s) {
  prepareMenuFrame();
  Font::useBody();
  // Compact, fixed row spacing — independent of the reader's line-gap setting
  // so the five rows plus the footer hint always fit and never overlap.
  int lineH  = (u8g2.getFontAscent() - u8g2.getFontDescent()) + 2;
  int y = drawSectionHeader(D_STATS_TIME_HEADING);

  char dur[24];
  char buf[64];

  struct Row { const char* fmt; uint64_t secs; bool bold; };
  const Row rows[] = {
    { D_STATS_TIME_TODAY_FMT, s.todayReadSecs,     true  },
    { D_STATS_TIME_WEEK_FMT,  s.weekReadSecs,      false },
    { D_STATS_TIME_MONTH_FMT, s.monthReadSecs,     false },
    { D_STATS_TIME_YEAR_FMT,  s.yearReadSecs,      false },
    { D_STATS_TIME_AVG_FMT,   s.avgPerDayReadSecs, false },
  };

  for (const Row& r : rows) {
    fmtDuration(dur, sizeof(dur), r.secs);
    snprintf(buf, sizeof(buf), r.fmt, dur);
    if (r.bold) Font::useBold();
    else        Font::useBody();
    u8g2.setCursor(MARGIN_X, y);
    u8g2.print(buf);
    y += lineH;
  }
  Font::useBody();

  display.update();
}

void StatisticsScreen::onButton(const ButtonEvent& e) {
  switch (e.kind) {
    case ButtonEvent::Short:
      // One click cycles back and forth through the stats pages.
      page_ = (page_ + 1) % STATS_PAGE_COUNT;
      draw();
      break;
    case ButtonEvent::Double:
      // Two clicks exit to the library.
      nextScreen = &g_libraryScreen;
      break;
    default:
      break;  // ignore long / triple / hold gestures
  }
}
