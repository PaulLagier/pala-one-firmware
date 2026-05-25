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
//  All D_* values are plain string-literal #defines. Adjacent string-literal
//  concatenation is exploited at call sites, e.g.
//    out += "<h2>" D_WEB_FILES_HEADING "</h2>";
//  The compiler folds these into a single literal — zero runtime cost.
//
//  Inclusion order: en.h is always included first as the canonical baseline.
//  The target language file is then included, overriding each translated key
//  via #undef + #define. Keys missing from the target language silently keep
//  their English value — compile-time enforcement was intentionally replaced
//  by test-time enforcement to allow incremental translation. Run the host
//  tests (`ctest`) to detect missing translations.
//
//  Authoring rule:
//    1. Add new keys to en.h FIRST. Then mirror them in the target language.
//    2. Target language files must #undef each key before redefining it.
//    3. Preserve %u / %d / %s placeholders verbatim across languages.
//    4. HTML entities (&middot; &mdash; &#10003; &#9888; &times; &#8592;)
//       and tags (<b> </b>) survive intact across translations.
//    5. D_WEB_CONFIRM_* strings are inlined inside JS confirm('...') calls in
//       HTML onclick attributes. They MUST NOT contain a single quote (') or
//       backslash (\) — either breaks attribute parsing and the destructive
//       form submits without prompting the user. If a translation needs a
//       contraction, rephrase or use a different punctuation mark.
// ============================================================================

#if !defined(LANG_EN) && !defined(LANG_ES_LA)
  #pragma message ("No LANG_* selected; defaulting to LANG_EN. Pick LANG_EN or LANG_ES_LA explicitly to silence this.")
  #define LANG_EN
#endif

// English baseline — always included, defines every D_* key.
#include "src/lang/en.h"

// Target language overlay — #undef + #define for each translated key.
// Keys not present in the target file keep the English value from above.
#if defined(LANG_ES_LA)
  #include "src/lang/es_la.h"
#endif

#endif  // PALA_LANG_H
