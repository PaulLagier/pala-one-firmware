#include "src/ui/screens/bookmarks/bookmark_list_screen.h"

#include "src/hal/display.h"
#include "src/pure/hashing.h"
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"         // g_library
#include "src/ui/font.h"
#include "src/ui/reader.h"
#include "src/ui/screens/bookmarks/book_select_screen.h"
#include "src/ui/screens/bookmarks/preview_screen.h"
#include "src/ui/screens/bookmarks/session.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/text.h"
#include "src/ui/widgets.h"

void BookmarkListScreen::onEnter() {
  draw();
}

void BookmarkListScreen::draw() {
  prepareMenuFrame();
  Font::useBody();
  int y = drawSectionHeader(D_BOOKMARKS_HEADER);

  String bookPath = String(g_library.books[g_bookmarkSession.bookIndex].path);
  String key = prefKeyForBook(bookPath);
  g_bookmarkSession.count = loadBookmarksForKey(key, g_bookmarkSession.pages, g_bookmarkSession.offsets);
  if (g_bookmarkSession.selectedIndex >= (int)g_bookmarkSession.count)
    g_bookmarkSession.selectedIndex = max(0, (int)g_bookmarkSession.count - 1);

  if (g_bookmarkSession.count == 0) {
    drawMenuRow(y, D_BOOKMARKS_NONE, false);
    display.update();
    return;
  }

  File f = FS.open(bookPath, "r");
  if (!f) {
    drawMenuRow(y, D_BOOKMARKS_OPEN_FAILED, false);
    display.update();
    return;
  }

  drawScrollableList(y, (int)g_bookmarkSession.count, g_bookmarkSession.selectedIndex,
    [&](int idx, int rowY, bool selected, int /*budget*/) {
      int targetPage = (int)g_bookmarkSession.pages[idx];
      if (targetPage < 0) targetPage = 0;
      uint32_t pageOff = resolveBookmarkOffset(bookPath, (uint16_t)targetPage,
                                               g_bookmarkSession.offsets[idx]);
      FileReadStream fs(f);
      String sn = readBookmarkLabelAtOffset(fs, pageOff, targetPage);
      drawMenuRow(rowY, sn, selected);
      return 1;
    });

  f.close();
  display.update();
}

void BookmarkListScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;

  if (g_bookmarkSession.selectedIndex >= (int)g_bookmarkSession.count)
    g_bookmarkSession.selectedIndex = max(0, (int)g_bookmarkSession.count - 1);

  if (e.kind == ButtonEvent::Short) {
    if (g_bookmarkSession.count > 0) {
      g_bookmarkSession.selectedIndex++;
      if (g_bookmarkSession.selectedIndex >= (int)g_bookmarkSession.count) g_bookmarkSession.selectedIndex = 0;
    }
    draw();
    return;
  }

  if (e.kind == ButtonEvent::Double) {
    if (g_bookmarkSession.count == 0) return;

    if (openBookByIndex(g_bookmarkSession.bookIndex)) {
      // Navigate by the bookmark's stored byte offset (invariant under
      // font/layout changes). The stored page number is just a stale
      // label; the offset is the source of truth for "where the user was."
      //
      // Legacy fallback: bookmarks saved by earlier firmware may have had
      // their offset invalidated to `kOffsetUnset` on font change. For those,
      // fall back to the stored page number — wrong-but-stable behavior
      // matching the old code. They'll heal next time the user re-saves.
      uint32_t off = g_bookmarkSession.offsets[g_bookmarkSession.selectedIndex];
      if (off == kOffsetUnset) {
        int storedPage = (int)g_bookmarkSession.pages[g_bookmarkSession.selectedIndex];
        if (storedPage < 0) storedPage = 0;
        g_bookview.cursor.pageIndex = storedPage;
      } else {
        g_bookview.cursor.pageIndex = findPageForOffset(off);
      }
      nextScreen = &g_bmPreviewScreen;
    } else {
      nextScreen = &g_libraryScreen;
    }
    return;
  }

  if (e.kind == ButtonEvent::Triple) {
    nextScreen = &g_libraryScreen;
    return;
  }

  if (e.kind == ButtonEvent::Long) {
    nextScreen = &g_bmBookSelectScreen;
    return;
  }
}
