#ifndef PALA_UI_SCREENS_SETTINGS_SCREEN_H
#define PALA_UI_SCREENS_SETTINGS_SCREEN_H

#include "src/ui/screen.h"

//  SettingsScreen — on-device editor for the layout settings the web
//  /settings page exposes (font size, font family, line spacing, compact
//  paragraph gaps, bionic reading). Reached from the library's "Settings"
//  system entry.
class SettingsScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;
};

extern SettingsScreen g_settingsScreen;

#endif  // PALA_UI_SCREENS_SETTINGS_SCREEN_H
