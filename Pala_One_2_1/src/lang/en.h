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
#define D_MENU_DEVICE               "Device"
#define D_MENU_UPLOAD               "Upload"
#define D_LIBRARY_OPEN_FAILED       "Open failed"
#define D_LIBRARY_TRY_UPLOAD        "Try upload again"

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

// ============================================================================
//  Web UI (captive portal) — strings embedded in HTML via adjacent-literal
//  concatenation. All endpoints declare Content-Type: charset=utf-8 already,
//  so accented characters survive transit unchanged.
// ============================================================================

// ----------------------------------------------------------------------------
//  Shared chrome / storage card (src/web/chrome.{h,cpp})
// ----------------------------------------------------------------------------
#define D_WEB_STORAGE_HEADING       "Storage"
#define D_WEB_STORAGE_BOOKS         "Books"
#define D_WEB_STORAGE_USED          "Used"
#define D_WEB_STORAGE_FREE          "Free"
#define D_WEB_STORAGE_TOTAL         "Total"
#define D_WEB_STORAGE_PCT_SUFFIX    "% of internal storage currently used."

// ----------------------------------------------------------------------------
//  Navigation links (used across multiple route handlers)
// ----------------------------------------------------------------------------
#define D_WEB_NAV_HOME              "Home"
#define D_WEB_NAV_FILES             "Files"
#define D_WEB_NAV_BOOKMARKS         "Bookmarks"
#define D_WEB_NAV_LIST              "List"
#define D_WEB_NAV_SETTINGS          "Settings"
#define D_WEB_NAV_FACTORY_RESET     "Factory reset"
#define D_WEB_NAV_BACK              "Back"

// ----------------------------------------------------------------------------
//  Home page (src/web/files.cpp handleRoot)
// ----------------------------------------------------------------------------
#define D_WEB_HOME_TITLE            "Pala One"
#define D_WEB_HOME_FW_PREFIX        "Firmware "
#define D_WEB_HOME_MIDDOT_SEP       " &middot; "
#define D_WEB_HOME_BOOKS_SUFFIX     " books"
#define D_WEB_HOME_FREE_LABEL       "Free: "
#define D_WEB_HOME_STORAGE_WARN     "&#9888; Storage is not available or almost full. If uploads fail, delete books or use Factory reset from this web UI."
#define D_WEB_UPLOAD_BOOK_HEADING   "Upload book"
#define D_WEB_UPLOAD_BOOK_DESC      "Send UTF-8 plain text files to <b>/books</b> on the device, then sort them into folders from the Files page."
#define D_WEB_UPLOAD_BOOK_BUTTON    "Upload"
#define D_WEB_MANAGE_FILES_BUTTON   "Manage files"
#define D_WEB_INSTALL_APP_HEADING   "Install app"
#define D_WEB_INSTALL_APP_DESC      "Upload a Pala app binary (<b>.bin</b>) to <b>/apps</b>. The header is validated before commit; only files with the correct magic and API version are accepted. Open <b>Apps</b> from the library to launch."
#define D_WEB_INSTALL_APP_BUTTON    "Install app"
#define D_WEB_NOTES_HEADING         "Notes"
#define D_WEB_NOTES_DESC            "Uploaded books are normalized and compacted before saving, so a source TXT can be larger than the final stored file. The reader is optimized for UTF-8 plain text and Latin-based languages."

// ----------------------------------------------------------------------------
//  Files page (src/web/files.cpp handleFiles + folder/move/jump forms)
// ----------------------------------------------------------------------------
#define D_WEB_FILES_HEADING         "Files"
#define D_WEB_FILES_SUBTITLE        "Manage books, folders and library structure for Pala One."
#define D_WEB_CREATE_FOLDER_HEADING "Create folder"
#define D_WEB_CREATE_FOLDER_PLACEHOLDER "books or classics/english"
#define D_WEB_CREATE_FOLDER_BUTTON  "Create folder"
#define D_WEB_CREATE_FOLDER_HINT    "Folders live inside /books."
#define D_WEB_FOLDERS_HEADING       "Folders"
#define D_WEB_NO_FOLDERS            "No folders yet. Books currently live in the root of /books."
#define D_WEB_CONFIRM_DELETE_FOLDER "Delete folder? Only empty folders can be deleted."
#define D_WEB_DELETE_BUTTON         "Delete"
#define D_WEB_LIBRARY_FILES_HEADING "Library files"
#define D_WEB_LIBRARY_FULL_WARN     "&#9888; Library full (80 books max). Delete books to make room."
#define D_WEB_FOLDER_LIMIT_WARN     "&#9888; Folder limit reached (32 max)."
#define D_WEB_NO_BOOKS_UPLOADED     "No books uploaded yet."
#define D_WEB_BOOK_ROOT             "Root"
#define D_WEB_BOOK_BYTES_LABEL      " bytes"
#define D_WEB_BOOK_FOLDER_LABEL     " &middot; folder: "
#define D_WEB_BOOK_CURRENT_PAGE     " &middot; current page: "
#define D_WEB_JUMP_BUTTON           "Jump"
#define D_WEB_JUMP_HINT             "Set the page that should open next on the device."
#define D_WEB_JUMP_HINT2            "The first open may take a moment."
#define D_WEB_PAGE_PLACEHOLDER      "Page"
#define D_WEB_MOVE_BUTTON           "Move"
#define D_WEB_MOVE_HINT             "Use the exact folder path."
#define D_WEB_MOVE_PLACEHOLDER      "leave blank for root"
#define D_WEB_CONFIRM_DELETE_FILE   "Delete file?"
#define D_WEB_APPS_PAGE_HEADING     "Apps"
#define D_WEB_NO_APPS_INSTALLED     "No apps installed."
#define D_WEB_CONFIRM_DELETE_APP    "Delete app?"

// ----------------------------------------------------------------------------
//  Plain-text 4xx/5xx error bodies (src/web/files.cpp, bookmarks.cpp,
//  apps_upload.cpp). These reach the browser on misformed requests; they
//  surface as page content if the user navigates a bad URL.
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
#define D_WEB_ERR_MISSING_ID_PAGE       "missing id/page"
#define D_WEB_ERR_MISSING_BOOK_IDX      "missing book/idx"
#define D_WEB_ERR_BAD_BOOK              "bad book"
#define D_WEB_ERR_BAD_IDX               "bad idx"
#define D_WEB_ERR_MISSING_BOOK          "missing book"
#define D_WEB_ERR_MISSING_NAME          "missing name"
#define D_WEB_ERR_INVALID_NAME          "invalid name"

// ----------------------------------------------------------------------------
//  List page (src/web/list.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_LIST_HEADING          "List"
#define D_WEB_LIST_SUBTITLE         "Create a simple shopping or to-do list for Pala One."
#define D_WEB_LIST_EDIT_HEADING     "Edit list"
#define D_WEB_LIST_EDIT_DESC        "Items appear on the device only when at least one line contains text. Hold the button on the device to mark an item as done."
#define D_WEB_LIST_ITEM_PLACEHOLDER "List item"
#define D_WEB_LIST_SAVE_BUTTON      "Save list"
#define D_WEB_LIST_DELETE_DONE      "Delete checked items"
#define D_WEB_LIST_HINT             "Blank rows are ignored. Checked rows can be removed directly."

// ----------------------------------------------------------------------------
//  Reset page (src/web/reset.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_RESET_HEADING         "Factory Reset"
#define D_WEB_RESET_SUBTITLE        "Erase all books, bookmarks, progress, and custom assets."
#define D_WEB_RESET_CONFIRM_HEADING "Confirm reset"
#define D_WEB_RESET_WARNING         "This will delete ALL books, bookmarks and reading progress."
#define D_WEB_RESET_DETAIL          "The device filesystem will be formatted and settings will return to defaults."
#define D_WEB_RESET_YES_BUTTON      "Yes, reset"
#define D_WEB_RESET_COMPLETE_HEADING "Factory reset complete"
#define D_WEB_RESET_COMPLETE_DESC   "All books, bookmarks, progress and custom assets were removed. The device is now back to a clean state."
#define D_WEB_GO_HOME_BUTTON        "Go to home"
#define D_WEB_OPEN_FILES_BUTTON     "Open files"
#define D_WEB_RESET_SUCCESS_TITLE   "Reset complete"
#define D_WEB_RESET_SUCCESS_SUBTITLE "Pala One was reset successfully."
#define D_WEB_RESET_BANNER          "&#10003; Factory reset complete."

// ----------------------------------------------------------------------------
//  Settings page (src/web/settings.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_SETTINGS_TITLE        "Pala One Settings"
#define D_WEB_SETTINGS_SUBTITLE_PREFIX "Firmware "
#define D_WEB_SETTINGS_SUBTITLE_SUFFIX " configuration page stored directly on the device."
#define D_WEB_SETTINGS_BACK_NAV     "&#8592; Home"
#define D_WEB_READING_HEADING       "Reading"
#define D_WEB_FONT_SIZE_LABEL       "Font size"
#define D_WEB_FONT_SIZE_8           "8px &mdash; tiny"
#define D_WEB_FONT_SIZE_10          "10px &mdash; small"
#define D_WEB_FONT_SIZE_12          "12px &mdash; medium"
#define D_WEB_FONT_SIZE_14          "14px &mdash; large"
#define D_WEB_FONT_SIZE_HINT        "Controls how many lines fit on each page."
#define D_WEB_SLEEP_AFTER_LABEL     "Sleep after"
#define D_WEB_SLEEP_30S             "30 seconds"
#define D_WEB_SLEEP_1M              "1 minute"
#define D_WEB_SLEEP_2M              "2 minutes"
#define D_WEB_SLEEP_5M              "5 minutes"
#define D_WEB_SLEEP_10M             "10 minutes"
#define D_WEB_SLEEP_30M             "30 minutes"
#define D_WEB_SLEEP_HINT            "Auto-sleep keeps battery draw low while idle."
#define D_WEB_LINE_SPACING_LABEL    "Line spacing"
#define D_WEB_LINE_SPACING_0        "0 px &mdash; compact"
#define D_WEB_LINE_SPACING_1        "1 px &mdash; normal"
#define D_WEB_LINE_SPACING_2        "2 px &mdash; relaxed"
#define D_WEB_LINE_SPACING_3        "3 px &mdash; loose"
#define D_WEB_LINE_SPACING_HINT     "A small change here can make text much easier to scan."
#define D_WEB_NO_SCREENSAVER_LABEL  "No-screensaver mode"
#define D_WEB_NO_SCREENSAVER_HINT   "Device still sleeps on the normal timer and refreshes the screen before going to sleep. Then it shows the last page of the book, and skips a full refresh on wake, so you can continue reading with a single click of the button without the interruption of a display refresh."
#define D_WEB_SAVE_SETTINGS_BUTTON  "Save settings"
#define D_WEB_SETTINGS_NO_EXTRAS    "No extra files, scripts, or fonts."
#define D_WEB_SCREENSAVER_HEADING   "Screensaver"
#define D_WEB_SCREENSAVER_SPECS     "Upload raw XBM bytes: <b>3904 bytes</b>, 250&times;122 px, 1-bit, LSB-first, 32 bytes per row."
#define D_WEB_SCREENSAVER_TIP       "Tip: use <a class='link' href='https://javl.github.io/image2cpp/' target='_blank'>image2cpp</a> with <b>Plain bytes</b>. Invert colors if needed."
#define D_WEB_SCREENSAVER_ACTIVE    "&#10003; Custom screensaver active."
#define D_WEB_CONFIRM_DEL_SCREENSAVER "Delete custom screensaver?"
#define D_WEB_SCREENSAVER_DEFAULT   "Using built-in screensaver."
#define D_WEB_SLEEP_IMAGE_LABEL     "Sleep image file"
#define D_WEB_SCREENSAVER_UPLOAD_BUTTON "Upload image"

// ----------------------------------------------------------------------------
//  Upload (book + sleep image) routes (src/web/upload.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_UPLOAD_COMPLETE_HEADING "Upload complete"
#define D_WEB_UPLOAD_COMPLETE_DESC  "Your book is now stored on the device and available in the library."
#define D_WEB_UPLOAD_BOOK_LABEL     "Book"
#define D_WEB_UPLOAD_STORED_SIZE    "Stored size"
#define D_WEB_UPLOAD_BOOKS_NOW      "Books now"
#define D_WEB_UPLOAD_FREE_SPACE     "Free space"
#define D_WEB_UPLOAD_ANOTHER        "Upload another"
#define D_WEB_UPLOAD_BOOK_SAVED     "Book saved successfully."
#define D_WEB_UPLOAD_FINISHED       "&#10003; Upload finished."
#define D_WEB_UPLOAD_ERR_FALLBACK   "Upload failed"
#define D_WEB_ERR_LIBRARY_FULL      "Library full"
#define D_WEB_ERR_NOT_ENOUGH_SPACE  "Not enough free space"
#define D_WEB_ERR_CANT_CREATE_TEMP_BOOK "Cannot create temp upload file"
#define D_WEB_ERR_WRITE_FAILED      "Write failed (out of space?)"
#define D_WEB_ERR_FINALIZE_UPLOAD   "Failed to finalize upload"
#define D_WEB_ERR_EMPTY_UPLOAD      "Empty upload"
#define D_WEB_ERR_UPLOAD_ABORTED    "Upload aborted"
#define D_WEB_SLEEP_UPLOAD_ERR_FALLBACK "Sleep image upload failed"
#define D_WEB_SLEEP_UPLOAD_HEADING  "Screensaver updated"
#define D_WEB_SLEEP_UPLOAD_DESC     "Your custom sleep image was saved successfully and will be shown the next time the device goes to sleep."
#define D_WEB_BACK_TO_SETTINGS      "Back to settings"
#define D_WEB_SLEEP_UPLOAD_SUBTITLE "Screensaver saved successfully."
#define D_WEB_SLEEP_UPLOAD_BANNER   "&#10003; Custom sleep image uploaded."
#define D_WEB_SLEEP_ERR_TEMP        "Cannot create temp sleep file"
#define D_WEB_SLEEP_ERR_SIZE        "Sleep image must be exactly 3904 bytes"
#define D_WEB_SLEEP_ERR_SAVE        "Failed to save sleep image"
#define D_WEB_SLEEP_ERR_ABORTED     "Sleep image upload aborted"

// ----------------------------------------------------------------------------
//  App upload route (src/web/apps_upload.cpp)
// ----------------------------------------------------------------------------
#define D_WEB_APP_UPLOAD_ERR_FALLBACK "App upload failed"
#define D_WEB_APP_INSTALLED_HEADING "App installed"
#define D_WEB_APP_INSTALLED_DESC    "Open the device, scroll the library to <b>Apps</b>, and double-click to launch."
#define D_WEB_APP_LABEL             "App"
#define D_WEB_APPS_NOW              "Apps now"
#define D_WEB_APP_INSTALLED_SUBTITLE "App saved to /apps/."
#define D_WEB_APP_INSTALLED_BANNER  "&#10003; App ready to run."
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
#define D_WEB_BOOKMARKS_HEADING     "Bookmarks"
#define D_WEB_BOOKMARKS_SUBTITLE    "Saved reading positions for Pala One, grouped by book."
#define D_WEB_NO_BOOKS_YET          "No books available yet."
#define D_WEB_NO_BOOKMARKS_CARD     "No bookmarks"
#define D_WEB_BOOKMARKS_OPEN_FAILED_CARD "Open failed"
#define D_WEB_BOOKMARK_PILL_PREFIX  "Bookmark "
#define D_WEB_BOOKMARK_VIEW         "View"
#define D_WEB_CONFIRM_DELETE_BOOKMARK "Delete bookmark?"
#define D_WEB_BOOKMARK_DOWNLOAD_ALL "Download all bookmarks"
#define D_WEB_BOOKMARK_VIEW_HEADING "Bookmark View"
#define D_WEB_BOOKMARK_VIEW_SUBTITLE "Preview the saved page text for this bookmark."
#define D_WEB_BOOKMARK_VIEW_BACK_NAV "&#8592; Back"
#define D_WEB_BOOKMARK_PAGE_EMPTY   "(empty)"
#define D_WEB_BOOKMARK_OPEN_FAILED_DOT "Open failed."

// Bookmark export plaintext labels (the .txt file downloads).
// Separators (==== / ----) stay verbatim and are NOT translated.
#define D_WEB_BMEXPORT_BOOK         "Book: "
#define D_WEB_BMEXPORT_BOOKMARKS    "Bookmarks: "
#define D_WEB_BMEXPORT_BOOKMARK_LBL "Bookmark "
#define D_WEB_NO_BOOKMARKS_THIS_BOOK "No bookmarks for this book"

// ----------------------------------------------------------------------------
//  Font family + bionic reading + reading-position retention
//  (src/web/settings.cpp). Layout-affecting settings; changes trigger the
//  reader to remap its byte-offset cursor under the new layout.
// ----------------------------------------------------------------------------
#define D_WEB_READING_INTRO         "Changing the font, family, line spacing, or bionic mode keeps your place in the current book &mdash; the device re-flows pages around the byte you're reading and lands on the page that contains it."
#define D_WEB_FONT_FAMILY_LABEL     "Font family"
#define D_WEB_FONT_FAMILY_HELVETICA "Helvetica"
#define D_WEB_FONT_FAMILY_DYSLEXIC  "OpenDyslexic"
#define D_WEB_FONT_FAMILY_HINT      "OpenDyslexic uses heavier letter shapes designed for easier scanning."
#define D_WEB_BIONIC_LABEL          "Bionic reading"
#define D_WEB_BIONIC_HINT           "Bolds the leading characters of each word to help your eyes anchor."
#define D_WEB_SETTINGS_APPLY_HINT   "Changes apply to the next page render."

// ----------------------------------------------------------------------------
//  Screensaver settings card link (src/web/settings.cpp).
// ----------------------------------------------------------------------------
#define D_WEB_SCREENSAVER_CARD_DESC "Manage the image (or multi-image rotation) shown on the e-ink when the device sleeps."
#define D_WEB_SCREENSAVER_EDITOR_LINK "Open screensaver editor"
#define D_WEB_SCREENSAVER_EDITOR_HINT "Includes an in-browser bitmap editor and up to 8 rotation slots."

// ----------------------------------------------------------------------------
//  Screensaver editor + multi-slot manager (src/web/screensavers.cpp).
//  JS-internal status / error strings emitted by the editor are NOT yet i18n'd;
//  they live inside the PROGMEM script block. Add D_WEB_SS_JS_* macros and a
//  data-attribute pass-through if/when that's wanted.
// ----------------------------------------------------------------------------
#define D_WEB_SS_TITLE              "Screensavers"
#define D_WEB_SS_SUBTITLE           "Custom sleep images, multi-slot rotation, and in-firmware bitmap editor."
#define D_WEB_SS_ROTATION_HEADING   "Rotation"
#define D_WEB_SS_ROTATION_INTRO     "Pick what shows on the e-ink each time the device sleeps. Cycle walks the populated slots in order; Shuffle picks at random without immediate repeats."
#define D_WEB_SS_MODE_LABEL         "Mode"
#define D_WEB_SS_MODE_SINGLE        "Single image only"
#define D_WEB_SS_MODE_CYCLE         "Cycle through slots"
#define D_WEB_SS_MODE_SHUFFLE       "Shuffle slots"
#define D_WEB_SS_SLOTS_POPULATED    "Populated slots: "
#define D_WEB_SS_SAVE_MODE          "Save mode"
#define D_WEB_SS_SLOTS_HEADING      "Rotation slots"
#define D_WEB_SS_SLOT_LABEL         "Slot"
#define D_WEB_SS_SLOT_EMPTY         "empty"
#define D_WEB_SS_CONFIRM_DEL_SLOT   "Delete this slot?"
#define D_WEB_SS_DOWNLOAD_ARIA      "Download screensaver"
#define D_WEB_SS_DELETE_ARIA        "Delete screensaver"
#define D_WEB_SS_ROTATE             "Rotate 90\u00b0"
#define D_WEB_SS_SINGLE_HEADING     "Single screensaver"
#define D_WEB_SS_SINGLE_ALT         "Single screensaver"
#define D_WEB_SS_CONFIRM_DEL_SINGLE "Delete the single screensaver?"
#define D_WEB_SS_NO_SINGLE          "No single screensaver uploaded. Upload via Editor."
#define D_WEB_SS_EDITOR_HEADING     "Editor"
#define D_WEB_SS_EDITOR_INTRO       "Drop an image into the editor, then upload it to a rotation slot or as the single legacy screensaver. All images render as 250&times;122 1-bit (3904 bytes)."
#define D_WEB_SS_SOURCE_IMAGE       "Source image"
#define D_WEB_SS_TOLERANCE          "Black tolerance"
#define D_WEB_SS_INVERT             "Invert black/white"
#define D_WEB_SS_PRECISE_CONTROL    "Precise control"
#define D_WEB_SS_ZOOM               "Zoom"
#define D_WEB_SS_MOVE_X             "Move X"
#define D_WEB_SS_MOVE_Y             "Move Y"
#define D_WEB_SS_PREVIEW_LABEL      "Preview (drag to move, pinch or scroll to zoom)"
#define D_WEB_SS_RESET_FIT          "Reset fit"
#define D_WEB_SS_NO_IMAGE           "No image loaded"
#define D_WEB_SS_SAVE_TO            "Save to"
#define D_WEB_SS_DST_SINGLE         "Single screensaver (/sleep.bin)"
#define D_WEB_SS_DST_AUTO_PREFIX    "Next free rotation slot (slot "
#define D_WEB_SS_DST_AUTO_SUFFIX    ")"
#define D_WEB_SS_DST_FULL           "(All rotation slots full)"
#define D_WEB_SS_DST_SLOT_PREFIX    "Rotation slot "
#define D_WEB_SS_DST_OVERWRITE      " (overwrite)"
#define D_WEB_SS_UPLOAD_EDITED      "Upload edited image"

#endif  // PALA_LANG_EN_H
