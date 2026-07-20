#include "src/ui/screens/library_screen.h"

#include "src/hal/display.h"
#include "src/ui/header_title.h"
#include "src/pure/library_nav.h"         // buildLibraryEntries
#include "src/pure/paths.h"               // folderLeafLabel, bookLeafLabel
#include "src/storage/library.h"
#include "src/storage/library_menu_order.h"
#include "src/storage/list_items.h"       // g_list
#include "src/ui/font.h"
#include "src/ui/reader.h"
#include "src/ui/screens/about_screen.h"
#include "src/ui/screens/apps_screen.h"
#include "src/ui/screens/update_screen.h"
#include "src/ui/screens/bookmarks/book_select_screen.h"
#include "src/ui/screens/bookmarks/session.h"
#include "src/ui/screens/list_screen.h"
#include "src/ui/screens/reader_screen.h"
#include "src/ui/screens/statistics_screen.h"
#include "src/ui/screens/upload_screen.h"
#include "src/ui/widgets.h"
#include "src/ui/reader_actions.h"
#include "src/ui/lock.h"
#include "src/ui/sleep.h"
// ============================================================================
//  Library screen nav state
//
//  Lives here, not in `storage/library.h`: only this screen consumes any of
//  it. The catalog (books + folders inventory) is what the rest of the
//  codebase shares; everything below is private screen state.
// ============================================================================

// LibraryScreen-specific row indenting. Other menu screens pass nothing
// extra to drawMenuRow; the library is the only one with folder nesting
// and a distinguishing nudge for "system" entries (Bookmarks/List/Device/
// Upload).
static const int LIBRARY_DEPTH_INDENT = 10;
static const int LIBRARY_SYSTEM_NUDGE = 2;

// Nav state — cursor + folder expansion. Persists across library visits
// (entering library doesn't collapse open folders).
//
// Folder expansion is keyed by *folder name*, not by index, because web
// handlers can call `loadBooks()` between our visits — that re-sorts
// `g_library.folders[]` and shifts indices arbitrarily. Storing names
// means we don't have to chase those reshuffles to stay in sync.
static int  s_cursor = 0;
static char s_expandedNames[MAX_FOLDERS][MAX_FOLDER_PATH + 1];
static int  s_expandedCount = 0;

// Cached entry list, rebuilt each draw, read by onButton until the next
// draw. Sized for the same upper bound the catalog can produce.
static LibEntry s_entries[MAX_LIBRARY_ENTRIES];
static int      s_entryCount = 0;

// ----------------------------------------------------------------------------
//  Folder expansion as a name-keyed set
// ----------------------------------------------------------------------------
static bool isExpanded(const char* name) {
  for (int i = 0; i < s_expandedCount; i++) {
    if (strcmp(s_expandedNames[i], name) == 0) return true;
  }
  return false;
}

// Toggle a folder's expansion. Prunes dead entries (folders that no longer
// exist in the catalog) before mutating — the user expanding/collapsing a
// folder is the natural moment to reconcile our set with the current
// catalog, rather than chasing every web handler that calls `loadBooks()`.
// Without this, the set would slowly fill with names of folders that have
// been deleted/renamed, until we'd refuse to expand a 33rd live one.
static void toggleExpanded(const char* name) {
  constexpr size_t kBuf = MAX_FOLDER_PATH + 1;

  int kept = 0;
  for (int i = 0; i < s_expandedCount; i++) {
    bool stillExists = false;
    for (int j = 0; j < g_library.folderCount; j++) {
      if (strcmp(s_expandedNames[i], g_library.folders[j]) == 0) {
        stillExists = true;
        break;
      }
    }
    if (stillExists) {
      if (kept != i) memcpy(s_expandedNames[kept], s_expandedNames[i], kBuf);
      kept++;
    }
  }
  s_expandedCount = kept;

  for (int i = 0; i < s_expandedCount; i++) {
    if (strcmp(s_expandedNames[i], name) == 0) {
      s_expandedCount--;
      if (i < s_expandedCount) {
        memcpy(s_expandedNames[i], s_expandedNames[s_expandedCount], kBuf);
      }
      return;
    }
  }
  if (s_expandedCount < MAX_FOLDERS) {
    strncpy(s_expandedNames[s_expandedCount], name, MAX_FOLDER_PATH);
    s_expandedNames[s_expandedCount][MAX_FOLDER_PATH] = '\0';
    s_expandedCount++;
  }
}

// ----------------------------------------------------------------------------
//  Per-entry helpers
// ----------------------------------------------------------------------------
static int rowIndent(const LibEntry& e) {
  int indent = e.depth * LIBRARY_DEPTH_INDENT;
  if (isValidLibEntry(e.type)) indent += LIBRARY_SYSTEM_NUDGE;
  return indent;
}

static String entryLabel(const LibEntry& e) {
  switch (e.type) {
    case LIB_ENTRY_FOLDER: {
      String prefix = isExpanded(g_library.folders[e.ref]) ? "- " : "+ ";
      return prefix + folderLeafLabel(String(g_library.folders[e.ref]));
    }
    case LIB_ENTRY_BOOK:      return bookLeafLabel(String(g_library.books[e.ref].path));
    case LIB_ENTRY_BOOKMARKS: return D_MENU_BOOKMARKS;
    case LIB_ENTRY_LIST:      return D_MENU_LIST;
    case LIB_ENTRY_APPS:       return D_MENU_APPS;
    case LIB_ENTRY_STATISTICS: return D_MENU_STATISTICS;
    case LIB_ENTRY_ABOUT:      return D_MENU_DEVICE;
    case LIB_ENTRY_UPDATE:     return D_MENU_UPDATE;
    case LIB_ENTRY_UPLOAD:     return D_MENU_UPLOAD;
  }
  return "";
}

// ----------------------------------------------------------------------------
//  Screen lifecycle
// ----------------------------------------------------------------------------
void LibraryScreen::onEnter() {
  // Full reset of reader/bookmark scratch state on every library entry —
  // downstream code never has to reason about stale reader state. Wake
  // state doesn't need clearing — it's consumed at boot, so it's already
  // empty during runtime unless the reader has explicitly armed it.
  resetBookView();
  resetBookmarkSession();

  // Reset the cursor but preserve folder expansion across visits — user
  // returning from a sub-screen expects their open folders still open.
  s_cursor = 0;

  draw();
}

void LibraryScreen::draw() {
  prepareMenuFrame();
  Font::useBody();

  // Pull the current order from persistent settings. The settings page can
  // later edit this list directly without changing the screen logic again.
  LibraryEntryType systemEntries[LibraryMenuOrder::kMaxSystemEntries];
  int systemCount = LibraryMenuOrder::copyEntries(
      systemEntries, LibraryMenuOrder::kMaxSystemEntries);

  // Build the bool[] view that the assembler wants from our name-keyed
  // expansion set, against the current folder ordering.
  bool expanded[MAX_FOLDERS] = {false};
  for (int i = 0; i < g_library.folderCount; i++) {
    expanded[i] = isExpanded(g_library.folders[i]);
  }

  s_entryCount = buildLibraryEntries(g_library, expanded,
                                     systemEntries, systemCount,
                                     s_entries, MAX_LIBRARY_ENTRIES);

  if (s_cursor < 0) s_cursor = 0;
  if (s_cursor >= s_entryCount) s_cursor = max(0, s_entryCount - 1);

  int y = drawSectionHeader(HeaderTitle::current());

  drawScrollableList(y, s_entryCount, s_cursor,
    [&](int idx, int rowY, bool selected, int /*budget*/) {
      drawMenuRow(rowY, entryLabel(s_entries[idx]), selected, rowIndent(s_entries[idx]));
      return 1;
    });

  display.update();
}

void LibraryScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;

  if (Gestures::resolveLegacyAction(e, ButtonEvent::Short, ACTION_NEXT)) {
    if (s_entryCount > 0) {
      s_cursor = (s_cursor + 1) % s_entryCount;
    }
    draw();
    return;
  }

  // Goes to the previous element (not supported in legacy)
  if (Gestures::isNonLegacyAction(e, ACTION_PREV)) {
    s_cursor--;

    if (s_cursor < 0)
    {
      s_cursor = s_entryCount - 1;
    }
    draw();
    return;
  }

  if (Gestures::isNonLegacyAction(e, ACTION_LOCK)) {
    Lock::engage();
    Sleep::enter();
    return;
  }
  if (Gestures::isNonLegacyAction(e, ACTION_ROTATE)) {
    ScreenSettings::toggleScreenRotation();
    return;
  }
  if (Gestures::isNonLegacyAction(e, ACTION_HOME)) {
    s_cursor = 0;
    draw();
    return;
  }

  if (Gestures::legacyControlsOn() && e.kind != ButtonEvent::Double)
    return;
  if (!Gestures::legacyControlsOn() && Gestures::actionFor(e.kind) != ACTION_MENU)
    return;

  if (s_cursor < 0 || s_cursor >= s_entryCount) {
    draw();
    return;
  }

  const LibEntry& sel = s_entries[s_cursor];

  if (sel.type == LIB_ENTRY_FOLDER) {
    toggleExpanded(g_library.folders[sel.ref]);
    draw();
    return;
  }

  if (sel.type == LIB_ENTRY_BOOK) {
    if (openBookByIndex(sel.ref)) {
      nextScreen = &g_readerScreen;
    } else {
      drawCenter(D_LIBRARY_OPEN_FAILED, D_LIBRARY_TRY_UPLOAD);
      draw();
    }
    return;
  }

  if (sel.type == LIB_ENTRY_BOOKMARKS) {
    nextScreen = &g_bmBookSelectScreen;
    return;
  }

  if (sel.type == LIB_ENTRY_LIST) {
    g_list.selectedIndex = 0;
    nextScreen = &g_listScreen;
    return;
  }

  if (sel.type == LIB_ENTRY_APPS) {
    nextScreen = &g_appsScreen;
    return;
  }

  if (sel.type == LIB_ENTRY_STATISTICS) {
    nextScreen = &g_statsScreen;
    return;
  }

  if (sel.type == LIB_ENTRY_ABOUT) {
    nextScreen = &g_aboutScreen;
    return;
  }

  if (sel.type == LIB_ENTRY_UPDATE) {
    nextScreen = &g_updateScreen;
    return;
  }

  if (sel.type == LIB_ENTRY_UPLOAD) {
    nextScreen = &g_uploadScreen;
    return;
  }

  Serial.println("LibraryScreen: unhandled entry type " + String(sel.type));
}

// ============================================================================
//  Public ops for callers outside the screen
// ============================================================================
void navigateToLibraryRoot() {
  if (g_currentScreen) g_currentScreen->nextScreen = &g_libraryScreen;
}

void resetLibraryNav() {
  s_cursor = 0;
  s_expandedCount = 0;
}
