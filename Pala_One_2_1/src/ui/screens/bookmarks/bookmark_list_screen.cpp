#include "src/ui/bookmark_chrome.h"
#include "src/ui/pala_chrome.h"
#include "src/ui/reader_index_feedback.h"
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
  if (g_bookmarkSession.bookIndex < 0 || g_bookmarkSession.bookIndex >= g_library.bookCount) {
    nextScreen = &g_bmBookSelectScreen; return;
  }
  String filePath = String(g_library.books[g_bookmarkSession.bookIndex].path);
  String key = prefKeyForBook(filePath);
  g_bookmarkSession.count = loadBookmarksForKey(key, g_bookmarkSession.pages, g_bookmarkSession.offsets);
  if (g_bookmarkSession.selectedIndex < 0 || g_bookmarkSession.selectedIndex > (int)g_bookmarkSession.count)
    g_bookmarkSession.selectedIndex = 0;

  if (g_bookmarkSession.count == 0) {
    prepareMenuFrame();
    drawSectionHeader("");BookmarkChrome::header(u8g2,D_BOOKMARKS_HEADER);BookmarkChrome::message(gfx,u8g2,D_BOOKMARKS_NONE);
    BookmarkChrome::card(gfx,u8g2,89,D_WEB_NAV_BACK,true);
    display.update();
    return;
  }

  File f = FS.open(filePath, "r");
  if (!f) {
    g_bookmarkSession.count = 0; g_bookmarkSession.selectedIndex = 0;
    prepareMenuFrame();
    drawSectionHeader("");BookmarkChrome::header(u8g2,D_BOOKMARKS_HEADER);BookmarkChrome::message(gfx,u8g2,D_BOOKMARKS_OPEN_FAILED);
    BookmarkChrome::card(gfx,u8g2,89,D_WEB_NAV_BACK,true);
    display.update();
    return;
  }

  // Resolve only the visible rows before drawing. A slow legacy lookup may
  // paint progress; drawing the final list afterwards avoids mixing frames.
  const int first=PalaChrome::firstVisible(g_bookmarkSession.selectedIndex,int(g_bookmarkSession.count)+1);
  String labels[PalaChrome::visibleCards];
  for(int row=0;row<PalaChrome::visibleCards&&first+row<=int(g_bookmarkSession.count);++row){
    const int index=first+row;
    if(index==int(g_bookmarkSession.count)){labels[row]=D_WEB_NAV_BACK;continue;}
    uint32_t pageOff;
    {
      ReaderIndexFeedback feedback;
      pageOff=resolveBookmarkOffset(filePath,g_bookmarkSession.pages[index],g_bookmarkSession.offsets[index],
        [](uint32_t current,uint32_t target,void* owner)->bool{
          return static_cast<ReaderIndexFeedback*>(owner)->step(current,target);
        },&feedback);
    }
    if(pageOff==kOffsetUnset){f.close();nextScreen=&g_bmBookSelectScreen;return;}
    FileReadStream stream(f);
    labels[row]=readBookmarkLabelAtOffset(stream,pageOff,int(g_bookmarkSession.pages[index]));
  }
  prepareMenuFrame();
  drawSectionHeader("");
  BookmarkChrome::list(gfx,u8g2,D_BOOKMARKS_HEADER,int(g_bookmarkSession.count)+1,g_bookmarkSession.selectedIndex,
    [&](int index)->String{return labels[index-first];});

  f.close();
  display.update();
}

void BookmarkListScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;

  if (g_bookmarkSession.selectedIndex < 0 || g_bookmarkSession.selectedIndex > (int)g_bookmarkSession.count)
    g_bookmarkSession.selectedIndex = 0;

  if (e.kind == ButtonEvent::Short) {
    g_bookmarkSession.selectedIndex = (g_bookmarkSession.selectedIndex + 1)
      % ((int)g_bookmarkSession.count + 1);
    draw();
    return;
  }

  if (e.kind == ButtonEvent::Double || e.kind == ButtonEvent::Long) {
    if (g_bookmarkSession.selectedIndex == (int)g_bookmarkSession.count) {
      nextScreen = &g_bmBookSelectScreen; return;
    }
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
        g_bookview.cursor.pageIndex = storedPage;
      } else {
        g_bookview.cursor.pageIndex = findPageForOffset(off);
        if (g_bookview.cursor.pageIndex < 0) { nextScreen = &g_libraryScreen; return; }
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

}
