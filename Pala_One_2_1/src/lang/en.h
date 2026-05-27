#ifndef PALA_LANG_EN_H
#define PALA_LANG_EN_H

// ============================================================================
//  English (en) string table — canonical key set.
//  See src/lang/lang.h for the authoring rule + selection mechanism.
// ============================================================================

// ----------------------------------------------------------------------------
//  Boot / fatal screens (Pala_One_2_1.ino)
// ----------------------------------------------------------------------------
#define D_BOOT_STORAGE_ERROR        "Storage error"
#define D_BOOT_TRY_FACTORY_RESET    "Try factory reset"

// ----------------------------------------------------------------------------
//  About screen (src/ui/screens/about_screen.cpp)
// ----------------------------------------------------------------------------
#define D_ABOUT_HEADER              "Device"
#define D_ABOUT_FIRMWARE_PREFIX     "Firmware "
#define D_ABOUT_GESTURE_NEXT        "1x next / down"
#define D_ABOUT_GESTURE_OPEN        "2x open / select"
#define D_ABOUT_GESTURE_HOME        "3x home"
#define D_ABOUT_GESTURE_BOOKMARK    "Hold bookmark"

// ----------------------------------------------------------------------------
//  Library screen — section title + system menu entries
//  (src/ui/screens/library_screen.cpp). The "+ " / "- " expansion indicators
//  in entryLabel() are visual symbols and intentionally NOT translated.
// ----------------------------------------------------------------------------
#define D_MENU_BOOKMARKS            "Bookmarks"
#define D_MENU_LIST                 "List"
#define D_MENU_APPS                 "Apps"
#define D_MENU_STATISTICS           "Statistics"
#define D_MENU_DEVICE               "Device"
#define D_MENU_UPLOAD               "Upload"
#define D_LIBRARY_OPEN_FAILED       "Open failed"
#define D_LIBRARY_TRY_UPLOAD        "Try upload again"

// ----------------------------------------------------------------------------
//  Statistics screen (src/ui/screens/statistics_screen.cpp). The *_FMT
//  strings are snprintf templates with %u / %llu placeholders — keep the
//  positional order across translations.
// ----------------------------------------------------------------------------
#define D_STATS_HEADING                "Statistics"
#define D_STATS_STREAK_CURRENT_FMT     "Current streak: %u days"
#define D_STATS_STREAK_LONGEST_FMT     "Longest: %u  Sessions: %u"
#define D_STATS_LIFETIME_PAGES_FMT     "Pages turned: %llu"
#define D_STATS_LIFETIME_PRESSES_FMT   "Button presses: %llu"

// ----------------------------------------------------------------------------
//  List screen (src/ui/screens/list_screen.cpp)
// ----------------------------------------------------------------------------
#define D_LIST_HEADER               "List"
#define D_LIST_NONE                 "No items"

// ----------------------------------------------------------------------------
//  Upload screen (src/ui/screens/upload_screen.cpp)
// ----------------------------------------------------------------------------
#define D_UPLOAD_HEADER             "Upload"
#define D_UPLOAD_WIFI               "Wi-Fi"
#define D_UPLOAD_PASSWORD           "Password"
#define D_UPLOAD_OPEN               "Open"
#define D_UPLOAD_CONNECTING         "Connecting"
#define D_UPLOAD_CONNECTED          "Connected"
#define D_UPLOAD_HOTSPOT_HINT_L1    "Press button to"
#define D_UPLOAD_HOTSPOT_HINT_L2    "use hotspot instead"

// ----------------------------------------------------------------------------
//  Apps screen (src/ui/screens/apps_screen.cpp)
// ----------------------------------------------------------------------------
#define D_APPS_HEADER               "Apps"
#define D_APPS_NONE                 "No apps installed"

// ----------------------------------------------------------------------------
//  Bookmarks screens
//  (src/ui/screens/bookmarks/{book_select_screen,bookmark_list_screen}.cpp)
// ----------------------------------------------------------------------------
#define D_BOOKMARKS_HEADER          "Bookmarks"
#define D_BOOKMARKS_NO_BOOKS        "No books"
#define D_BOOKMARKS_NONE            "No bookmarks"
#define D_BOOKMARKS_OPEN_FAILED     "Open failed"

// ----------------------------------------------------------------------------
//  Reader (src/ui/reader.cpp)
// ----------------------------------------------------------------------------
#define D_READER_BOOK_EMPTY         "Book empty"
#define D_READER_BACK_LIBRARY       "Back to library"

// ----------------------------------------------------------------------------
//  App loader error overlay (src/ui/pala_api_impl.cpp paintLoadError)
// ----------------------------------------------------------------------------
#define D_APP_ERR_TITLE             "App error"
#define D_APP_ERR_NULL_PATH         "null path"
#define D_APP_ERR_NOT_FOUND         "App not found"
#define D_APP_ERR_TOO_SMALL         "App too small"
#define D_APP_ERR_INVALID_FILE      "Invalid file"
#define D_APP_ERR_TOO_LARGE         "App too large"
#define D_APP_ERR_SIZE_LIMIT        "> 48 KB"
#define D_APP_ERR_READ              "Read error"
#define D_APP_ERR_PARTIAL_READ      "Partial read"
#define D_APP_ERR_NO_EXEC_MEM       "No exec memory"
#define D_APP_ERR_BAD_FILE          "Bad app file"
#define D_APP_ERR_WRONG_MAGIC       "Wrong magic"
#define D_APP_ERR_API_MISMATCH      "API mismatch"
#define D_APP_ERR_API_FMT           "API v%u, need v%u"
#define D_APP_ERR_BAD_ENTRY         "Bad entry offset"
#define D_APP_ERR_BAD_RELOC         "Bad reloc table"
#define D_APP_ERR_RELOC_RANGE       "Reloc out of range"

// ----------------------------------------------------------------------------
//  Bookmark add toasts (src/pure/bookmarks_codec.cpp)
//  These are pointers returned from a pure module and rendered via Toast::show
//  in reader_screen.cpp.
// ----------------------------------------------------------------------------
#define D_TOAST_BOOKMARK_EXISTS     "Bookmark exists"
#define D_TOAST_BOOKMARK_SAVED      "Bookmark saved"

// ----------------------------------------------------------------------------
//  Lock / screensaver (src/ui/sleep.cpp, Pala_One_2_1.ino)
// ----------------------------------------------------------------------------
#define D_SCREENSAVER_LOCKED        "Locked"
#define D_TOAST_UNLOCKED            "Unlocked"

// ============================================================================
//  Web UI (captive portal) — strings embedded in HTML via adjacent-literal
//  concatenation. All endpoints declare Content-Type: charset=utf-8 already,
//  so accented characters survive transit unchanged.
// ============================================================================

// ----------------------------------------------------------------------------
//  Shared chrome / storage card (src/web/chrome.{h,cpp})
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Navigation links (used across multiple route handlers)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Home page (src/web/files.cpp handleRoot)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Files page (src/web/files.cpp handleFiles + folder/move/jump forms)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies (src/web/files.cpp, bookmarks.cpp,
//  apps_upload.cpp). These reach the browser on misformed requests; they
//  surface as page content if the user navigates a bad URL.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  List page (src/web/list.cpp)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Reset page (src/web/reset.cpp)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Settings page (src/web/settings.cpp)
// ----------------------------------------------------------------------------

// Buttons / remappable hold-gestures section.
#define D_WEB_BUTTONS_HEADING       "Buttons"
#define D_WEB_BUTTONS_HINT          "1 click = next, 2 = previous, 3 = home. The three holds below are remappable."
#define D_WEB_BUTTONS_LONG          "Long press"
#define D_WEB_BUTTONS_EXTRA_LONG    "Extra-long press"
#define D_WEB_BUTTONS_CLICK_HOLD    "Click, then hold"
#define D_WEB_BUTTONS_SAVE          "Save buttons"
#define D_WEB_BUTTONS_LOCK_HINT     "If locked, repeat any hold gesture to unlock."
#define D_WEB_BUTTONS_ACTION_NONE     "None"
#define D_WEB_BUTTONS_ACTION_BOOKMARK "Bookmark page"
#define D_WEB_BUTTONS_ACTION_LOCK     "Lock device"
#define D_WEB_BUTTONS_ACTION_MENU     "Open menu"

// ----------------------------------------------------------------------------
//  Upload (book + sleep image) routes (src/web/upload.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_UPLOAD_ERR_FALLBACK   "Upload failed"
#define D_WEB_ERR_LIBRARY_FULL      "Library full"
#define D_WEB_ERR_NOT_ENOUGH_SPACE  "Not enough free space"
#define D_WEB_ERR_CANT_CREATE_TEMP_BOOK "Cannot create temp upload file"
#define D_WEB_ERR_WRITE_FAILED      "Write failed (out of space?)"
#define D_WEB_ERR_FINALIZE_UPLOAD   "Failed to finalize upload"
#define D_WEB_ERR_EMPTY_UPLOAD      "Empty upload"
#define D_WEB_ERR_UPLOAD_ABORTED    "Upload aborted"

// ----------------------------------------------------------------------------
//  App upload route (src/web/apps_upload.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_APP_UPLOAD_ERR_FALLBACK "App upload failed"
#define D_WEB_APP_VALID_OK          "OK"
#define D_WEB_APP_VALID_TOO_SMALL   "Invalid app (file too small)"
#define D_WEB_APP_VALID_BAD_MAGIC   "Invalid app (bad magic)"
#define D_WEB_APP_VALID_BAD_ENTRY   "Invalid app (bad entry offset)"
#define D_WEB_APP_VALID_BAD_RELOC   "Invalid app (bad reloc table)"
#define D_WEB_APP_VALID_API_FMT     "Invalid app (API v%u, need v%u)"
#define D_WEB_APP_VALID_INVALID     "Invalid app"
#define D_WEB_APPS_DIR_FULL         "Apps directory full"
#define D_WEB_ERR_CANT_CREATE_TEMP_APP "Cannot create temp app file"
#define D_WEB_APP_TOO_LARGE         "App too large (> 48 KB)"
#define D_WEB_APP_BINARY_TOO_SMALL  "App binary too small"
#define D_WEB_APP_CANT_READ_HEADER  "Could not read app header"
#define D_WEB_APP_FINALIZE_FAILED   "Failed to finalize app upload"
#define D_WEB_APP_UPLOAD_ABORTED    "App upload aborted"

// ----------------------------------------------------------------------------
//  Bookmarks web page (src/web/bookmarks.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_BOOKMARKS_OPEN_FAILED_CARD "Open failed"
#define D_WEB_BOOKMARK_PAGE_EMPTY   "(empty)"
#define D_WEB_BOOKMARK_OPEN_FAILED_DOT "Open failed."

// Bookmark export plaintext labels (the .txt file downloads).
// Separators (==== / ----) stay verbatim and are NOT translated.
#define D_WEB_BMEXPORT_BOOK         "Book: "
#define D_WEB_BMEXPORT_BOOKMARKS    "Bookmarks: "
#define D_WEB_BMEXPORT_BOOKMARK_LBL "Bookmark "
#define D_WEB_NO_BOOKMARKS_THIS_BOOK "No bookmarks for this book"

// ----------------------------------------------------------------------------
//  In-browser reader + find/jump (src/web/find.cpp).
// ----------------------------------------------------------------------------
#define D_WEB_READ_TITLE            "Read"
#define D_WEB_READ_SUBTITLE         "Browse and search the book in your browser. Use Jump to set the device's resume point."
#define D_WEB_READ_BYTES_LABEL      "bytes"
#define D_WEB_READ_CURRENT_PAGE_LABEL "current page:"
#define D_WEB_READ_FIND_PLACEHOLDER "Find in book"
#define D_WEB_READ_FIND_ALL         "Find all"
#define D_WEB_READ_FIND_PREV        "Prev"
#define D_WEB_READ_FIND_NEXT        "Next"
#define D_WEB_READ_JUMP_HERE        "Jump to here"
#define D_WEB_READ_LOADING          "Loading book text..."
#define D_WEB_READ_PAGE_PLACEHOLDER "Page number"
#define D_WEB_READ_JUMP_PAGE        "Jump to page"
#define D_WEB_READ_JUMP_HINT        "Saves the next-open page directly."
#define D_WEB_READ_AND_FIND_LINK    "Read &amp; find"

// ----------------------------------------------------------------------------
//  Font family + bionic reading + reading-position retention
//  (src/web/settings.cpp). Layout-affecting settings; changes trigger the
//  reader to remap its byte-offset cursor under the new layout.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Screensaver settings card link (src/web/settings.cpp).
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Screensaver editor + multi-slot manager (src/web/screensavers.cpp).
//  JS-internal status / error strings emitted by the editor are NOT yet i18n'd;
//  they live inside the PROGMEM script block. Add D_WEB_SS_JS_* macros and a
//  data-attribute pass-through if/when that's wanted.
// ----------------------------------------------------------------------------

#endif  // PALA_LANG_EN_H
