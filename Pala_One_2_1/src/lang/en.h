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
#define D_ABOUT_FIRMWARE_PREFIX     "Firmware"
#define D_ABOUT_GESTURE_CLICK       "click"
#define D_ABOUT_GESTURE_CLICK_2     "click x 2"
#define D_ABOUT_GESTURE_CLICK_3     "click x 3"
#define D_ABOUT_GESTURE_CLICK_HOLD  "click + hold"
#define D_ABOUT_GESTURE_HOLD        "hold"
#define D_ABOUT_GESTURE_LONG_HOLD   "long hold"
#define D_ABOUT_GESTURE_SEPARATOR   ": "

// ----------------------------------------------------------------------------
// Action names  (src/ui/reader_actions.cpp)
// ----------------------------------------------------------------------------
#define D_ACTION_NONE_LABEL     "None"
#define D_ACTION_NEXT_LABEL     "Next"
#define D_ACTION_OPEN_LABEL     "Open/Select"
#define D_ACTION_BOOKMARK_LABEL "Bookmark"
#define D_ACTION_HOME_LABEL     "Home"
#define D_ACTION_LOCK_LABEL     "Lock"
#define D_ACTION_MENU_LABEL     "Menu"
#define D_ACTION_ROTATE_LABEL   "Flip screen"

// ----------------------------------------------------------------------------
//  Library screen — section title + system menu entries
//  (src/ui/screens/library_screen.cpp). The "+ " / "- " expansion indicators
//  in entryLabel() are visual symbols and intentionally NOT translated.
// ----------------------------------------------------------------------------
#define D_MENU_BOOKMARKS            "Bookmarks"
#define D_MENU_LIST                 "List"
#define D_MENU_SETTINGS             "Layout Settings"
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
//  Settings screen (src/ui/screens/settings_screen.cpp). The D_WEB_* labels
//  below were shared with the old server-rendered Web UI; the SPA now carries
//  its own strings in webui/i18n/, so they live here for the device screen.
// ----------------------------------------------------------------------------
#define D_SETTINGS_HEADER           "Layout Settings"

#define D_WEB_FONT_SIZE_LABEL       "Font size"
#define D_WEB_LINE_SPACING_LABEL    "Line spacing"
#define D_WEB_FONT_FAMILY_LABEL     "Font family"
#define D_WEB_FONT_FAMILY_HELVETICA "Helvetica"
#define D_WEB_FONT_FAMILY_DYSLEXIC  "OpenDyslexic"
#define D_WEB_BIONIC_LABEL          "Bionic reading"

#define D_SETTINGS_GAP_0            "0 - compact"
#define D_SETTINGS_GAP_1            "1 - normal"
#define D_SETTINGS_GAP_2            "2 - relaxed"
#define D_SETTINGS_GAP_3            "3 - loose"

#define D_SETTINGS_SIZE_8           "8 - tiny"
#define D_SETTINGS_SIZE_10          "10 - small"
#define D_SETTINGS_SIZE_12          "12 - medium"
#define D_SETTINGS_SIZE_14          "14 - large"

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
//  Update screen (src/ui/screens/update_screen.cpp)
// ----------------------------------------------------------------------------
#define D_MENU_UPDATE               "Firmware Update"
#define D_UPDATE_HEADER             "Firmware Update"
#define D_UPDATE_VERSION_PREFIX     "Current Version: "
#define D_UPDATE_CHANNEL_LABEL      "Select Firmware Channel:"
#define D_UPDATE_CHAN_STABLE        "Stable"
#define D_UPDATE_CHAN_DEV           "Dev"
#define D_UPDATE_BTN_CHECK          "[ Check for update ]"
#define D_UPDATE_NO_CREDS_L1        "No Wi-Fi credentials."
#define D_UPDATE_NO_CREDS_L2        "Setup via web installer."
#define D_UPDATE_CONNECTING         "Connecting..."
#define D_UPDATE_CONN_FAILED        "Wi-Fi connection failed"
#define D_UPDATE_CHECKING           "Checking..."
#define D_UPDATE_SERVER_FAIL        "Cannot reach update server"
#define D_UPDATE_UP_TO_DATE         "Already up to date"
#define D_UPDATE_AVAILABLE_PREFIX   "Available: "
#define D_UPDATE_BTN_INSTALL        "[ Install update ]"
#define D_UPDATE_INSTALLING         "Installing..."
#define D_UPDATE_DOWNLOAD_FAILED    "Download failed"
#define D_UPDATE_REBOOT_MSG         "Update installed"
#define D_UPDATE_REBOOT_HINT        "2x to reboot"

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
#define D_READER_INDEXING_TITLE     "Preparing book..."
#define D_READER_INDEXING_DETAIL    "Rebuilding page index"

// ----------------------------------------------------------------------------
//  Reader menu overlay (src/ui/reader_menu.cpp). Both *_FMT strings are
//  snprintf templates rendered into a char[64]; keep the argument order —
//  PAGE_OF_FMT takes (page, total, percent), PAGE_PCT_FMT takes (page,
//  percent). STATUSBAR_FMT's %s is one of the D_STATUSBAR_MODE_* labels.
// ----------------------------------------------------------------------------
#define D_READER_MENU_HEADING       "Reading"
#define D_READER_MENU_PAGE_OF_FMT   "Page %d of %d  (%d%%)"
#define D_READER_MENU_PAGE_PCT_FMT  "Page %d  (%d%% of book)"
#define D_READER_MENU_STATUSBAR_FMT "Statusbar: %s"
#define D_READER_MENU_HINT          "click: cycle  2x: close"
#define D_STATUSBAR_MODE_FULL       "Full"
#define D_STATUSBAR_MODE_MINIMAL    "Minimal"
#define D_STATUSBAR_MODE_HIDDEN     "Hidden"

// ----------------------------------------------------------------------------
//  Toasts (src/ui/screen_settings.cpp, src/storage/statistics.cpp)
// ----------------------------------------------------------------------------
#define D_TOAST_SCREEN_FLIPPED      "Flipped"
#define D_TOAST_STREAK_DAY_FMT      "Reading streak: day %u"


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
//  Web UI (captive portal) — only the plain-text bodies the C++ routes send
//  back (errors, bookmark export). The SPA's own strings live in
//  webui/i18n/*.js. All endpoints declare charset=utf-8, so accented
//  characters survive transit unchanged.
// ============================================================================

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies (src/web/api_files.cpp, apps_upload.cpp,
//  find.cpp, screensavers.cpp). The SPA shows them after its own localised
//  "Server error:" prefix. The lowercase ones are argument-validation
//  failures, the capitalized ones are operation failures — keep that split
//  in translation.
//  FILE_NAME_TOO_LONG's %d is MAX_FILE_NAME; %s is file name.
// ----------------------------------------------------------------------------
#define D_WEB_ERR_MISSING_ID            "missing id"
#define D_WEB_ERR_BAD_ID                "bad id"
#define D_WEB_ERR_BAD_FOLDER            "bad folder"
#define D_WEB_ERR_FOLDER_LIMIT          "folder limit reached"
#define D_WEB_ERR_MKDIR_FAILED          "mkdir failed"
#define D_WEB_ERR_FOLDER_NOT_FOUND      "folder not found"
#define D_WEB_ERR_FOLDER_NOT_EMPTY      "folder not empty"
#define D_WEB_ERR_DELETE_FAILED         "delete failed"
#define D_WEB_ERR_FOLDER_CREATE_FAILED  "folder create failed"
#define D_WEB_ERR_DEST_EXISTS           "destination exists"
#define D_WEB_ERR_MOVE_FAILED           "move failed"
#define D_WEB_ERR_FILE_NAME_TOO_LONG    "move failed: new path exceeds %d characters: %s"
#define D_WEB_ERR_MISSING_NAME          "missing name"
#define D_WEB_ERR_INVALID_NAME          "invalid name"
#define D_WEB_ERR_MISSING_OFFSET        "missing offset"
#define D_WEB_ERR_OPEN_FAILED           "Open failed"
#define D_WEB_ERR_NOT_FOUND             "Not found"
#define D_WEB_ERR_APP_NOT_FOUND         "App not found"

// ----------------------------------------------------------------------------
//  Book upload route (src/web/upload.cpp)
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
//  Bookmarks API + .txt export (src/web/api_bookmarks.cpp)
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
//  Screensaver thumbnail / download / upload routes (src/web/screensavers.cpp).
// ----------------------------------------------------------------------------

// Screensaver upload failures. These are set on the upload handler's error
// slot and returned as the 400 body by handleScreensaverUploadDone.
// EXACT_BYTES_FMT's %d is Screensavers::SCREENSAVER_BYTES.
#define D_WEB_SS_ERR_THUMB_NOT_FOUND "Thumbnail not found"
#define D_WEB_SS_ERR_NOT_FOUND       "Screensaver not found"
#define D_WEB_SS_ERR_UPLOAD_FAILED   "Upload failed"
#define D_WEB_SS_ERR_SLOTS_FULL      "All rotation slots are full"
#define D_WEB_SS_ERR_CANT_CREATE_TMP "Cannot create temp file"
#define D_WEB_SS_ERR_IMAGE_TOO_LARGE "Image file is too large"
#define D_WEB_SS_ERR_CHOOSE_IMAGE    "Please choose an image first."
#define D_WEB_SS_ERR_EXACT_BYTES_FMT "Image must be exactly %d bytes"
#define D_WEB_SS_ERR_SAVE_SLEEP      "Failed to save sleep image"
#define D_WEB_SS_ERR_SAVE_SLOT       "Failed to save rotation slot"

#endif  // PALA_LANG_EN_H
