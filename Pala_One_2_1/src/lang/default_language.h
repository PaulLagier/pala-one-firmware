#pragma once
// PlatformIO supplies one project-wide LANG_* flag. For Arduino IDE, choose the
// fallback here so every .cpp and the sketch use the same initial dictionary.
// The device language can subsequently be changed from Pala or the browser.
#if !defined(LANG_EN) && !defined(LANG_ES_LA)
#define LANG_EN
// #define LANG_ES_LA
#endif
