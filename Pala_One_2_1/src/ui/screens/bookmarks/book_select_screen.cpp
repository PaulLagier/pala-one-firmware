#include "src/ui/bookmark_chrome.h"
#include "src/ui/pala_chrome.h"
#include "src/ui/screens/bookmarks/book_select_screen.h"

#include "src/hal/display.h"
#include "src/storage/library.h"         // g_library
#include "src/ui/font.h"
#include "src/ui/screens/bookmarks/bookmark_list_screen.h"
#include "src/ui/screens/bookmarks/session.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"

void BookmarkBookSelectScreen::onEnter() {
  draw();
}

void BookmarkBookSelectScreen::draw() {
  prepareMenuFrame();
  if (g_bookmarkSession.bookIndex < 0 || g_bookmarkSession.bookIndex > g_library.bookCount)
    g_bookmarkSession.bookIndex = 0;
  if(g_library.bookCount==0){drawSectionHeader("");BookmarkChrome::header(u8g2,D_BOOKMARKS_HEADER);BookmarkChrome::message(gfx,u8g2,D_BOOKMARKS_NO_BOOKS);BookmarkChrome::card(gfx,u8g2,89,D_READER_BACK_LIBRARY,true);display.update();return;}
  drawSectionHeader("");
  BookmarkChrome::list(gfx,u8g2,D_BOOKMARKS_HEADER, g_library.bookCount + 1, g_bookmarkSession.bookIndex,
    [](int idx) { return idx == g_library.bookCount ? String(D_READER_BACK_LIBRARY)
      : String(g_library.books[idx].name); });

  display.update();
}

void BookmarkBookSelectScreen::onButton(const ButtonEvent& e) {
  if (e.kind == ButtonEvent::Triple) {
    nextScreen = &g_libraryScreen;
    return;
  }

  if (e.kind == ButtonEvent::Short) {
    g_bookmarkSession.bookIndex = (g_bookmarkSession.bookIndex + 1) % (g_library.bookCount + 1);
    draw(); return;
  }
  if (e.kind == ButtonEvent::Double || e.kind == ButtonEvent::Long) {
    if (g_bookmarkSession.bookIndex == g_library.bookCount) {
      nextScreen = &g_libraryScreen; return;
    }
    if (g_bookmarkSession.bookIndex < 0 || g_bookmarkSession.bookIndex >= g_library.bookCount) return;
    g_bookmarkSession.selectedIndex = 0;
    nextScreen = &g_bmListScreen;
  }
}
