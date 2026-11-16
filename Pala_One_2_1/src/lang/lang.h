#pragma once
#include "src/lang/default_language.h"
// Firmware uses runtime catalogs. Host adapter tests can retain literal tables;
// dedicated runtime tests enable PALA_RUNTIME_LANG and exercise the same core.
#if defined(ARDUINO) || defined(PALA_RUNTIME_LANG)
#include "src/lang/locale_core.h"
#include "src/lang/join.h"
#else
#if !defined(LANG_EN) && !defined(LANG_ES_LA) && !defined(LANG_FR)
  #pragma message ("No LANG_* selected; defaulting to LANG_EN. Pick LANG_EN, LANG_ES_LA or LANG_FR explicitly to silence this.")
  #define LANG_EN
#endif

#if defined(LANG_ES_LA)
  #include "src/lang/es_la.h"
#elif defined(LANG_FR)
  #include "src/lang/fr.h"
#elif defined(LANG_EN)
  #include "src/lang/en.h"
#endif


#endif
