#ifndef PALA_UI_SCREEN_ROTATION_H
#define PALA_UI_SCREEN_ROTATION_H

namespace ScreenSettings
{

    void loadSettings();
    void toggleScreenRotation();
    bool isScreenFlipped();
    void setScreenRotation(bool);
    bool batteryIndicatorsEnabled();
    void setBatteryIndicatorsEnabled(bool enabled);
}

#endif