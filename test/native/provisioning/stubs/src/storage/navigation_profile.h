#pragma once
#include "serial.h"
namespace NavigationProfile {
inline void loadSettings() {}
inline bool setReadingFocused(bool) { return true; }
} // namespace NavigationProfile
namespace Language {
inline bool setLanguage(const char *) { return true; }
} // namespace Language
