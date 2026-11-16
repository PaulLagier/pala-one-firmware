#ifndef PALA_UI_SCREENS_BOOKMARKS_SESSION_H
#define PALA_UI_SCREENS_BOOKMARKS_SESSION_H

#include <Arduino.h>

#include "src/config.h"

// Shared state for the three-screen bookmark flow:
// BookmarkBookSelectScreen -> BookmarkListScreen -> BookmarkPreviewScreen.
// Reset on entering LibraryScreen.
struct BookmarkSession {
  int bookIndex = 0;
  int selectedIndex = 0;

  uint32_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t  count = 0;
};

extern BookmarkSession g_bookmarkSession;

void resetBookmarkSession();

#endif  // PALA_UI_SCREENS_BOOKMARKS_SESSION_H
