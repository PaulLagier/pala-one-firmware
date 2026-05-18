// Host implementations of the book_metadata.h firmware-glue functions
// (normally compiled only under #ifdef ARDUINO). Delegates to the same
// prefs store that handleJumpPageWeb writes to, so page saves round-trip.

#include "src/storage/book_metadata.h"
#include "src/storage/preferences_store.h"
#include "src/pure/hashing.h"
#include "src/state.h"

static PreferencesStore kv() { return PreferencesStore(prefs); }

uint8_t loadBookmarksForKey(const String& bookKey,
                            uint16_t outPages[MAX_BOOKMARKS],
                            uint32_t outOffsets[MAX_BOOKMARKS]) {
  auto store = kv();
  Bookmarks bm = loadBookmarks(store, bookKey);
  for (int i = 0; i < bm.count; i++) {
    outPages[i]   = bm.pages[i];
    outOffsets[i] = bm.offsets[i];
  }
  return bm.count;
}

void saveBookmarksForKey(const String& bookKey,
                         const uint16_t pages[MAX_BOOKMARKS],
                         const uint32_t offsets[MAX_BOOKMARKS],
                         uint8_t count) {
  Bookmarks bm;
  bm.count = count;
  for (int i = 0; i < count; i++) {
    bm.pages[i]   = pages[i];
    bm.offsets[i] = offsets[i];
  }
  auto store = kv();
  saveBookmarks(store, bookKey, bm);
}

int savedPageForBookPath(const String& path) {
  String key = prefKeyForBook(path);
  auto store = kv();
  return loadSavedPage(store, key);
}

void deleteBookMetadata(const String& path) {
  String key = prefKeyForBook(path);
  auto store = kv();
  clearBookMetadata(store, key);
}

void migrateBookMetadata(const String& oldPath, const String& newPath) {
  String oldKey = prefKeyForBook(oldPath);
  String newKey = prefKeyForBook(newPath);
  auto store = kv();
  renameBookMetadata(store, oldKey, newKey);
}
