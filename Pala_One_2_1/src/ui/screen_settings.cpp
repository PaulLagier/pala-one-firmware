#include "src/ui/screen_settings.h"
#include "src/state.h"
#include "src/ui/toast.h"

namespace ScreenSettings {
    static bool s_screenFlipped = false;
    static bool s_batteryIndicatorsEnabled = true;
    static constexpr const char *kKeyScreenFlipped = "cfg_scr_flip";
    static constexpr const char *kKeyBatteryIndicators = "cfg_batt_ind";

    void loadSettings() {
        s_screenFlipped = prefs.getBool(kKeyScreenFlipped, false);
        s_batteryIndicatorsEnabled = prefs.getBool(kKeyBatteryIndicators, true);
    }

    void toggleScreenRotation() {
        s_screenFlipped = !s_screenFlipped;
        Toast::show(D_TOAST_SCREEN_FLIPPED);
        prefs.putBool(kKeyScreenFlipped, s_screenFlipped);
    }
    
    bool isScreenFlipped() {
        return s_screenFlipped;
    }

    void setScreenRotation(bool inverted) {
        s_screenFlipped = inverted;
        prefs.putBool(kKeyScreenFlipped, s_screenFlipped);
    }

    bool batteryIndicatorsEnabled() {
        return s_batteryIndicatorsEnabled;
    }

    void setBatteryIndicatorsEnabled(bool enabled) {
        s_batteryIndicatorsEnabled = enabled;
        prefs.putBool(kKeyBatteryIndicators, enabled);
    }
}