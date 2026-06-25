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
//  Theme toggle in the page header (src/web/chrome.cpp webPageStart). The
//  title is emitted inside a single-quoted HTML attribute, so it follows the
//  no-apostrophe rule from lang.h.
// ----------------------------------------------------------------------------
#define D_WEB_THEME_TOGGLE_TITLE    "Light or dark appearance"
#define D_WEB_THEME_DARK_MODE       "Dark mode"
#define D_WEB_THEME_LIGHT_MODE      "Light mode"

// ----------------------------------------------------------------------------
//  Home page (src/web/files.cpp handleRoot)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Files page (src/web/files.cpp handleFiles + folder/move/jump forms)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies (src/web/files.cpp, bookmarks.cpp,
//  apps_upload.cpp, find.cpp, screensavers.cpp). These reach the browser on
//  misformed requests; they surface as page content if the user navigates a
//  bad URL. The lowercase ones are argument-validation failures, the
//  capitalized ones are operation failures — keep that split in translation.
//  FILE_NAME_TOO_LONG's %d is MAX_FILE_NAME; %s is file name.
// ----------------------------------------------------------------------------
#define D_WEB_ERR_MISSING_ID            "missing id"
#define D_WEB_ERR_BAD_ID                "bad id"
#define D_WEB_ERR_MISSING_FOLDER        "missing folder"
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
#define D_WEB_ERR_MISSING_ID_PAGE       "missing id/page"
#define D_WEB_ERR_MISSING_BOOK_IDX      "missing book/idx"
#define D_WEB_ERR_BAD_BOOK              "bad book"
#define D_WEB_ERR_BAD_IDX               "bad idx"
#define D_WEB_ERR_MISSING_BOOK          "missing book"
#define D_WEB_ERR_MISSING_NAME          "missing name"
#define D_WEB_ERR_INVALID_NAME          "invalid name"
#define D_WEB_ERR_MISSING_OFFSET        "missing offset"
#define D_WEB_ERR_OPEN_FAILED           "Open failed"
#define D_WEB_ERR_NOT_FOUND             "Not found"
#define D_WEB_ERR_APP_NOT_FOUND         "App not found"

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
#define D_WEB_BUTTONS_HINT          "You can remap click patterns to different actions"
#define D_WEB_BUTTONS_LONG          "Long press"
#define D_WEB_BUTTONS_SHORT         "Single click"
#define D_WEB_BUTTONS_DOUBLE        "Double click"
#define D_WEB_BUTTONS_TRIPLE        "Triple click"
#define D_WEB_BUTTONS_EXTRA_LONG    "Extra-long press"
#define D_WEB_BUTTONS_CLICK_HOLD    "Click, then hold"
#define D_WEB_BUTTONS_SAVE          "Save buttons"
#define D_WEB_BUTTONS_LOCK_HINT     "If locked, repeat any hold gesture to unlock."
#define D_WEB_BUTTONS_ACTION_NONE     "None"
#define D_WEB_BUTTONS_ACTION_BOOKMARK "Bookmark page"
#define D_WEB_BUTTONS_ACTION_LOCK     "Lock device"
#define D_WEB_BUTTONS_ACTION_MENU     "OK/Open menu(Context dependent)"
#define D_WEB_BUTTONS_ACTION_ROTATE   "Flip screen orientation"
#define D_WEB_BUTTONS_ACTION_NEXT "Next item/page"
#define D_WEB_BUTTONS_ACTION_PREV "Previous item/page"
#define D_WEB_BUTTONS_ACTION_HOME "Go to main menu"
#define D_WEB_BUTTONS_LEGACY_HINT "If enabled, mapping single, double and triple clicks has no effect."

// Click timing section.
#define D_WEB_TIMINGS_HEADING        "Click timing"
#define D_WEB_TIMINGS_INTRO          "Tune the behavior for clicks and hold gestures."
#define D_WEB_TIMINGS_GAP_LABEL      "Click gap"
#define D_WEB_TIMINGS_GAP_HINT       "How long a pause between releases may last before the current click burst is committed."
#define D_WEB_TIMINGS_SEQUENCE_LABEL "Click sequence"
#define D_WEB_TIMINGS_SEQUENCE_HINT  "Maximum total time allowed for a multi-click burst, measured from the first release."
#define D_WEB_TIMINGS_LONG_LABEL     D_WEB_BUTTONS_LONG " threshold"
#define D_WEB_TIMINGS_LONG_HINT      "Press duration that turns a hold into a " D_WEB_BUTTONS_LONG " instead of another " D_WEB_BUTTONS_SHORT "."
#define D_WEB_TIMINGS_VLONG_LABEL    D_WEB_BUTTONS_EXTRA_LONG " threshold"
#define D_WEB_TIMINGS_VLONG_HINT     "Press duration that upgrades a solo hold from " D_WEB_BUTTONS_LONG " to " D_WEB_BUTTONS_EXTRA_LONG "."
#define D_WEB_TIMINGS_DEBOUNCE_LABEL "Debounce"
#define D_WEB_TIMINGS_DEBOUNCE_HINT  "Events closer together than this are treated as switch bounce and ignored."
#define D_WEB_TIMINGS_RESET          "Reset"
#define D_WEB_TIMINGS_SAVE           "Save timings"
#define D_WEB_TIMINGS_DEFAULT_PREFIX "Default: "
// Device personalization card (src/web/settings.cpp).
#define D_WEB_DEVICE_HEADING        "Device"
#define D_WEB_DEVICE_INTRO          "Personalize the device name shown on the library screen header."
#define D_WEB_HEADER_TITLE_LABEL    "Header title"
#define D_WEB_HEADER_TITLE_HINT     "Shown at the top of the library screen. Leave empty for no title."
#define D_WEB_HEADER_TITLE_RESET    "Reset to default"
#define D_WEB_FLIP_SCREEN           "Flip screen orientation"
#define D_WEB_BATTERY_INDICATORS_LABEL "Hide battery indicators"
#define D_WEB_BATTERY_INDICATORS_HINT "When checked, battery status is visible on the " D_ABOUT_HEADER " screen only."

// Header status icons card (src/web/settings.cpp) — see src/ui/icons.h.
#define D_WEB_ICONS_HEADING         "Status icons"
#define D_WEB_ICONS_INTRO           "Small indicators shown in the menu header for states that use extra battery."
#define D_WEB_ICON_SLEEP_LABEL      "Show sleep-blocked icon"
#define D_WEB_ICON_SLEEP_HINT       "A crossed-out moon, shown while something is keeping the device awake - a connected computer, or an upload in progress."

// Wi-Fi card (src/web/settings.cpp) — the saved network the upload screen
// joins.
#define D_WEB_WIFI_HEADING            "Wi-Fi"
#define D_WEB_WIFI_INTRO              "The network the device joins for file upload. For security reasons, saved passwords will never be shown."
#define D_WEB_WIFI_SSID_LABEL         "SSID"
#define D_WEB_WIFI_PASSWORD_LABEL     "Password"
#define D_WEB_WIFI_SSID_PLACEHOLDER   "Network name"
#define D_WEB_WIFI_PASSWORD_PLACEHOLDER "Password"
#define D_WEB_WIFI_PASSWORD_SAVED_PLACEHOLDER "<saved>"
#define D_WEB_WIFI_SHOW_PASSWORD      "Show password"
#define D_WEB_WIFI_SAVE_BUTTON        "Save network"
#define D_WEB_WIFI_HINT               "Stored on device. Leave the SSID blank to forget it."

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

// Status strings emitted by the find/jump JS. These are substituted into
// single-quoted JS string literals inside a PROGMEM <script> block, so the
// no-apostrophe / no-backslash rule from lang.h applies here too. The *_FMT
// values are expanded by window.palaFmt (src/web/chrome.cpp): %1, %2, %3 are
// positional and MAY be reordered by a translation.
#define D_WEB_READ_JS_NO_MATCHES    "No matches"
#define D_WEB_READ_JS_MATCH_FMT     "Match %1 of %2  (byte %3)"
#define D_WEB_READ_JS_ENTER_PHRASE  "Enter a phrase to find."
#define D_WEB_READ_JS_FIND_FIRST    "Find something first."
#define D_WEB_READ_JS_SAVING        "Saving..."
#define D_WEB_READ_JS_SAVED_FMT     "Saved. Open the book on the device to jump to byte %1."
#define D_WEB_READ_JS_SAVE_HTTP_FMT "Save failed: HTTP %1"
#define D_WEB_READ_JS_SAVE_FAIL_FMT "Save failed: %1"
#define D_WEB_READ_JS_ERROR         "error"
#define D_WEB_READ_JS_LOADED_FMT    "Loaded %1 bytes. Enter a phrase to find."
#define D_WEB_READ_JS_LOAD_FAILED   "Could not load book text."

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
//  The editor's JS status strings are the D_WEB_SS_JS_* block at the end of
//  this section; they are concatenated straight into the PROGMEM script.
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

// Editor JS status strings. Same single-quote / backslash restriction and
// same window.palaFmt positional %N expansion as the D_WEB_READ_JS_* block.
// The "no image" reset reuses D_WEB_SS_NO_IMAGE, which is also the initial
// server-rendered value of the same element.
#define D_WEB_SS_JS_PX_SUFFIX        " px"
#define D_WEB_SS_JS_PREVIEW_FMT      "Preview: %1x%2  threshold %3  bytes %4"
#define D_WEB_SS_JS_DECODE_FAILED    "Could not decode image."
#define D_WEB_SS_JS_READ_FAILED      "Could not read image."
#define D_WEB_SS_JS_PREVIEW_NOT_READY "Preview is not ready yet."
#define D_WEB_SS_JS_UPLOADING        "Uploading..."
#define D_WEB_SS_JS_HTTP_FMT         "HTTP %1"
#define D_WEB_SS_JS_UPLOAD_COMPLETE  "Upload complete. Refreshing..."
#define D_WEB_SS_JS_UPLOAD_FAIL_FMT  "Upload failed: %1"
#define D_WEB_SS_JS_ERROR            "error"

#endif  // PALA_LANG_EN_H
