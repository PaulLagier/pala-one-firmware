#ifndef PALA_LANG_H
#define PALA_LANG_H

// ============================================================================
//  Language selector — included transitively via src/config.h, so every
//  translation unit sees the D_* string-literal macros without an extra
//  per-file include.
//
//  Selection mechanism mirrors src/config.h's BOARD_V1_X / DISPLAY_V1_X
//  pattern: a single LANG_* build flag picks one per-language header.
//    PlatformIO: -D LANG_EN / -D LANG_ES_LA in build_flags.
//    Arduino IDE: uncomment LANG_EN or LANG_ES_LA in Pala_One_2_1.ino.
//
//  All D_* values are plain string-literal #defines, so they can be joined
//  with adjacent literals at call sites; the compiler folds these into a
//  single literal — zero runtime cost.
//
//  Scope: device-screen strings plus the plain-text bodies the web routes
//  return. The browser UI's strings live in webui/i18n/*.js, not here.
//
//  Authoring rule:
//    1. Add new keys to en.h FIRST. Then mirror them in es_la.h.
//    2. The key set across en.h and es_la.h MUST stay in sync — a missing
//       key in es_la.h produces a "use of undeclared identifier" against the
//       Spanish build.
//    3. Preserve %u / %d / %s placeholders verbatim across languages.
// ============================================================================

// Default to English when nothing was picked. Lives here (not in config.h) so
// that any TU which reaches lang.h before config.h still gets the fallback. Removing this block re-enables a hard #error
// when neither LANG_* is set.
#if !defined(LANG_EN) && !defined(LANG_ES_LA)
  #pragma message ("No LANG_* selected; defaulting to LANG_EN. Pick LANG_EN or LANG_ES_LA explicitly to silence this.")
  #define LANG_EN
#endif

#if defined(LANG_ES_LA)
  #include "src/lang/es_la.h"
#elif defined(LANG_EN)
  #include "src/lang/en.h"
#endif

#endif  // PALA_LANG_H
