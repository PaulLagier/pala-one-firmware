#ifndef PALA_CONFIG_H
#define PALA_CONFIG_H

// ============================================================================
//  Project-wide compile-time configuration.
//
//  IMPORTANT: This header is included transitively by the pure/ modules
//  (paginator, list_codec, bookmarks_codec, etc.), which are also compiled
//  into the host-test build. It MUST therefore remain host-compatible:
//
//    - No <Arduino.h>, <WiFi.h>, or any library header.
//    - No code, only `#define`s, `static const` constants, type aliases,
//      and `extern` declarations of host-safe types.
//    - If you need to add hardware- or library-specific declarations,
//      put them in a different header that only firmware sources include.
//
//  The build will catch violations (the host test compile will fail), but
//  treat this comment as the convention so the surprise is small.
// ============================================================================

#include "src/pure/arduino_compat.h"

// FW_VERSION and BUILD_GIT_HASH are injected by scripts/build_info.py from
// `git describe --tags --always --dirty` and the short SHA respectively, at
// PlatformIO build time. Host-test and Arduino IDE builds skip that script,
// so provide fallbacks so the preprocessor never sees an undefined macro.
// The Arduino IDE path is for developer iteration — releases go through the
// PIO + tagged-CI flow, where the real values get injected.
#ifndef FW_VERSION
#define FW_VERSION "dev"
#endif

#ifndef BUILD_GIT_HASH
#define BUILD_GIT_HASH "unknown"
#endif

// DEBUG_BUILD = 1 shows the git hash in the on-device library screen header
// (the front face of the device). About screen + web UI always show the
// hash regardless — those are debug surfaces. Defaults on for local + main-
// branch CI builds; the release workflow passes `-DDEBUG_BUILD=0` for tags.
#ifndef DEBUG_BUILD
#define DEBUG_BUILD 1
#endif

#if DEBUG_BUILD
  #define LIB_HEADER_TITLE "Pala One " BUILD_GIT_HASH
#else
  #define LIB_HEADER_TITLE "Pala One"
#endif

// Language selection (LANG_EN / LANG_ES_LA) and the LANG_EN fallback live in
// src/lang/lang.h itself — included at the end of this header so every TU
// that pulls in config.h transitively sees the D_* macros.

static const int SCREEN_W = 250;
static const int SCREEN_H = 122;

static const uint8_t MAX_BOOKMARKS = 12;
static const int MAX_BOOKS = 80;
static const int MAX_FOLDERS = 32;
static const int MAX_FOLDER_PATH = 63;  // chars, excluding null
static const int MAX_PAGES = 10000;
static const int MAX_LIBRARY_ENTRIES = (MAX_BOOKS * 2) + (MAX_FOLDERS * 2) + 8;
static const int MAX_LIST_ITEMS = 16;
static const int MAX_LIST_TEXT = 64;

// Apps catalog upper bounds. Match the field widths in PalaAppHeader.name
// (32) and a generous absolute-path limit for /apps/<filename>.bin (80).
static const int MAX_APPS = 16;
static const int MAX_APP_NAME = 32;
static const int MAX_APP_PATH = 80;

// Largest app binary the loader will accept. Caps RAM exposure from a
// malformed/oversized upload: the loader allocates MALLOC_CAP_EXEC heap
// of exactly fileSize bytes, so this bounds the worst-case footprint.
// 48 KB matches the value the original monolithic firmware used; revisit
// after measuring free exec-capable heap on a real device.
static const size_t MAX_APP_BINARY = 48u * 1024u;

// Max silence after the most recent release before we commit a click sequence.
// The press-in-progress gate in input.cpp's trailing-silence check means this
// effectively bounds "dead time between release and the next press" — once a
// press starts, the commit pauses until that release lands. So the user can
// take this long to *start* their next click; the press itself can take as
// long as it wants (up to LONG_MS).
static const uint32_t MAX_CLICK_GAP_MS = 175;

// Max total duration of a multi-click sequence, measured from the first release.
// Conceptually: "the whole multi-click input has to complete within this window."
// Combined with MAX_CLICK_GAP_MS this caps both per-gap and overall sloppiness.
static const uint32_t MAX_CLICK_SEQUENCE_MS = 550;

static const uint32_t LONG_MS = 850;

// Hold this long (without a preceding click) and the classifier emits
// VeryLong instead of Long. Long and VeryLong — plus the click-then-hold
// chord — are independently bindable to reader actions (bookmark / lock /
// menu / none) via the web settings UI.
static const uint32_t VERY_LONG_MS = 2000;

static const uint32_t DEBOUNCE_MS = 14;

static const uint32_t SAVE_EVERY_MS = 7000;
static const uint32_t TOAST_MS = 650;
static const uint32_t UPLOAD_AUTO_EXIT_MS = 15UL * 60UL * 1000UL;
static const uint32_t BAT_CACHE_MS = 180000; // 3 min — battery changes slowly
static const uint32_t BAT_CHARGING_CHECK_MS = 2000;
static const float BAT_CHARGING_VOLTAGE = 4.09f;

static const int FULL_REFRESH_EVERY_N_PAGES = 100;
static const int MENU_FULL_REFRESH_EVERY = 60;

// Buffer size for file read operations. Bigger is not necessarily
// better as you might wind up just pulling in more data than you need
// to fill a page. This was a value that worked well with both ASCII
// unicode inputs.
static const size_t FILE_STREAM_BUF_BYTES = 512;

static const int MARGIN_X = 6;
static const int TOP_PAD = 3;
static const int BOT_PAD = 0;
static const int STATUS_H = 8;

static const bool SHOW_PROGRESS_BAR = true;
static const bool SHOW_PAGE_NUMBER = true;
static const bool ENABLE_DEEP_SLEEP = true;

#define BTN 0
#define HAS_BATTERY 1
#if HAS_BATTERY
  #define BAT_ADC_CTRL 19
  #define BAT_ADC_IN   20
#endif

// NOTE: `#define FS LittleFS` lives in state.h AFTER all system headers, so it
// doesn't collide with the `class FS` declared inside Arduino's FS.h.

// Fonts live behind the role API in `ui/font.h` (Font::useBody/useBold/
// useUiSmall/useUiTiny). No code outside font.cpp references u8g2 font
// tables directly.

// Language strings (D_* macros) live in src/lang/. Included here so every
// TU that pulls in config.h transitively sees the macros without per-file
// includes. lang.h is pure preprocessor — no Arduino dependencies, host-test
// safe.
#include "src/lang/lang.h"

#endif  // PALA_CONFIG_H
