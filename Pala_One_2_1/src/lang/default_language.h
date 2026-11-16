#pragma once
// Initial default for every translation unit. PlatformIO supplies LANG_*;
// Arduino IDE users choose the fallback here. Runtime selection is persistent.
#if (defined(LANG_EN) + defined(LANG_ES_LA) + defined(LANG_FR)) > 1
#error Select only one initial language
#endif
#if !defined(LANG_EN) && !defined(LANG_ES_LA) && !defined(LANG_FR)
#define LANG_EN
// Replace the preceding line with LANG_ES_LA or LANG_FR to change the fallback.
#endif
