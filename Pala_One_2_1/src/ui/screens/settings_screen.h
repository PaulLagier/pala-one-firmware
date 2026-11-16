#pragma once
#include "src/ui/screen.h"
// Small device hub. Reading preferences are managed by app/browser.
class SettingsScreen : public Screen {
public:
  void onEnter() override;
  void draw() override;
  void onButton(const ButtonEvent& event) override;
private:
  int selection_=0;
};
extern SettingsScreen g_settingsScreen;
