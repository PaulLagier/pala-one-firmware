#ifndef PALA_UI_SCREENS_STATISTICS_SCREEN_H
#define PALA_UI_SCREENS_STATISTICS_SCREEN_H

#include "src/ui/screen.h"

struct StatisticsSnapshot;  // src/storage/statistics.h

// Read-only dashboard across two pages:
//   page 0 — reading streak, longest streak, total sessions, lifetime
//            page-turn / button-press counters, 30-day bitmap.
//   page 1 — today / week / month / year reading time + average per day.
// Snapshot is taken per draw. With the device's single button, a click
// advances to the next page; a click on the last page returns to the library.
class StatisticsScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;

private:
  void drawStreakPage(const StatisticsSnapshot& s);
  void drawTimePage(const StatisticsSnapshot& s);

  int page_ = 0;
};

extern StatisticsScreen g_statsScreen;

#endif  // PALA_UI_SCREENS_STATISTICS_SCREEN_H
