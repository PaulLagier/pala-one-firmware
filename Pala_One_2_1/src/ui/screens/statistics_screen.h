#ifndef PALA_UI_SCREENS_STATISTICS_SCREEN_H
#define PALA_UI_SCREENS_STATISTICS_SCREEN_H

#include "src/ui/screen.h"
#include "src/storage/statistics.h"

// Read-only one-page dashboard. Snapshot is stable until the next visit.
// Long/Double activates the header return control; Short leaves values unchanged.
class StatisticsScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;
private:
  StatisticsSnapshot snapshot_{};
};

extern StatisticsScreen g_statsScreen;

#endif  // PALA_UI_SCREENS_STATISTICS_SCREEN_H
