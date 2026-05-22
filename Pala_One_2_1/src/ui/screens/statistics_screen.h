#ifndef PALA_UI_SCREENS_STATISTICS_SCREEN_H
#define PALA_UI_SCREENS_STATISTICS_SCREEN_H

#include "src/ui/screen.h"

// Read-only dashboard showing the current reading streak, longest streak,
// total sessions, and lifetime page-turn / button-press counters. Snapshot
// at onEnter — values don't update mid-screen. Any button click returns to
// the library.
class StatisticsScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;
};

extern StatisticsScreen g_statsScreen;

#endif  // PALA_UI_SCREENS_STATISTICS_SCREEN_H
