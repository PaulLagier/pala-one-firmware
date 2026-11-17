#include "src/storage/book_metadata.h"
#include <cstring>

// ============================================================================
//  Testable KV-store-backed API
// ============================================================================
Bookmarks loadBookmarks(KeyValueStore &kv, const String &bookKey) {
  Bookmarks bm;
  uint8_t buf[BOOKMARKS_ENCODED_MAX_SIZE] = {0};
  size_t got = kv.getBytes(bmKeyFor(bookKey).c_str(), buf, sizeof(buf));
  decodeBookmarks(buf, got, bm);
  return bm;
}

void saveBookmarks(KeyValueStore &kv, const String &bookKey, const Bookmarks &bm) {
  uint8_t buf[BOOKMARKS_ENCODED_MAX_SIZE];
  size_t n = encodeBookmarks(bm, buf);
  kv.putBytes(bmKeyFor(bookKey).c_str(), buf, n);
}

bool loadBookmarksChecked(KeyValueStore &kv, const String &bookKey, Bookmarks &marks) {
  const String key = bmKeyFor(bookKey);
  uint8_t original[BOOKMARKS_ENCODED_MAX_SIZE];
  const size_t storedSize = kv.bytesSize(key.c_str());
  if (storedSize > sizeof(original) || (!storedSize && kv.has(key.c_str())))
    return false;
  const size_t length = kv.getBytes(key.c_str(), original, sizeof(original));
  if (length != storedSize)
    return false;
  if (length) {
    const size_t count = original[0] & 0x7f;
    const bool wide = (original[0] & 0x80) != 0;
    const bool validLength =
        wide ? length == 1 + count * 8 : length == 1 + count * 2 || length == 1 + count * 6;
    if (count > MAX_BOOKMARKS || !validLength)
      return false;
  }
  decodeBookmarks(original, length, marks);
  return true;
}

bool saveBookmarksChecked(KeyValueStore &kv, const String &bookKey, const Bookmarks &marks) {
  uint8_t expected[BOOKMARKS_ENCODED_MAX_SIZE], stored[BOOKMARKS_ENCODED_MAX_SIZE];
  const size_t size = encodeBookmarks(marks, expected);
  const String key = bmKeyFor(bookKey);
  kv.putBytes(key.c_str(), expected, size);
  const size_t got = kv.getBytes(key.c_str(), stored, sizeof(stored));
  // Even an empty list must read back its one-byte record, not a failed read.
  return got == size && kv.bytesSize(key.c_str()) == size &&
         std::memcmp(expected, stored, size) == 0;
}

const char *applyReaderBookmarkAction(KeyValueStore &kv, const String &bookKey, uint32_t page,
                                      uint32_t start, uint32_t end, bool toggle) {
  Bookmarks marks;
  if (!loadBookmarksChecked(kv, bookKey, marks))
    return D_WEB_ERR_WRITE_FAILED;
  BookmarkEditResult result;
  if (toggle)
    result = toggleBookmarkForPage(marks, page, start, end);
  else {
    const auto added = addBookmark(marks, page, start);
    result = {added.added, added.message};
  }
  if (!result.changed)
    return result.message;
  return saveBookmarksChecked(kv, bookKey, marks) ? result.message : D_WEB_ERR_WRITE_FAILED;
}

int loadSavedPage(KeyValueStore &kv, const String &bookKey) {
  int p = kv.getInt((bookKey + "_p").c_str(), 0);
  return (p < 0) ? 0 : p;
}

void saveSavedPage(KeyValueStore &kv, const String &bookKey, int pageIndex) {
  kv.putInt((bookKey + "_p").c_str(), pageIndex);
}

uint32_t loadSavedOffset(KeyValueStore &kv, const String &bookKey) {
  // Stored as int; -1 (default when key absent) sentinels "not set."
  int v = kv.getInt((bookKey + "_off").c_str(), -1);
  return (v < 0) ? kOffsetUnset : (uint32_t)v;
}

void saveSavedOffset(KeyValueStore &kv, const String &bookKey, uint32_t byteOffset) {
  kv.putInt((bookKey + "_off").c_str(), (int)byteOffset);
}

bool clearBookMetadata(KeyValueStore &kv, const String &bookKey) {
  kv.remove((bookKey + "_p").c_str());
  kv.remove((bookKey + "_off").c_str());
  kv.remove(bmKeyFor(bookKey).c_str());
  return true;
}

void renameBookMetadata(KeyValueStore &kv, const String &oldKey, const String &newKey) {
  int progress = kv.getInt((oldKey + "_p").c_str(), -1);
  if (progress >= 0) {
    kv.putInt((newKey + "_p").c_str(), progress);
    kv.remove((oldKey + "_p").c_str());
  }

  int off = kv.getInt((oldKey + "_off").c_str(), -1);
  if (off >= 0) {
    kv.putInt((newKey + "_off").c_str(), off);
    kv.remove((oldKey + "_off").c_str());
  }

  uint8_t buf[BOOKMARKS_ENCODED_MAX_SIZE] = {0};
  size_t got = kv.getBytes(bmKeyFor(oldKey).c_str(), buf, sizeof(buf));
  if (got > 0) {
    kv.putBytes(bmKeyFor(newKey).c_str(), buf, got);
    kv.remove(bmKeyFor(oldKey).c_str());
  }
}

#ifdef ARDUINO
// ============================================================================
//  Firmware glue
// ============================================================================
#include "src/storage/library.h"    // g_library (bulk invalidation iterates books)
#include "src/storage/page_cache.h" // deletePageCacheForBook / renamePageCacheForBook
#include "src/storage/preferences_store.h"

uint8_t loadBookmarksForKey(const String &bookKey, uint32_t outPages[MAX_BOOKMARKS],
                            uint32_t outOffsets[MAX_BOOKMARKS]) {
  PreferencesStore kv(prefs);
  Bookmarks bm = loadBookmarks(kv, bookKey);
  for (uint8_t i = 0; i < bm.count; i++) {
    outPages[i] = bm.pages[i];
    outOffsets[i] = bm.offsets[i];
  }
  return bm.count;
}

void saveBookmarksForKey(const String &bookKey, const uint32_t pages[MAX_BOOKMARKS],
                         const uint32_t offsets[MAX_BOOKMARKS], uint8_t count) {
  Bookmarks bm;
  bm.count = (count > MAX_BOOKMARKS) ? MAX_BOOKMARKS : count;
  for (uint8_t i = 0; i < bm.count; i++) {
    bm.pages[i] = pages[i];
    bm.offsets[i] = offsets[i];
  }
  PreferencesStore kv(prefs);
  saveBookmarks(kv, bookKey, bm);
}

int savedPageForBookPath(const String &path) {
  PreferencesStore kv(prefs);
  return loadSavedPage(kv, prefKeyForBook(path));
}

void deleteBookMetadata(const String &path) {
  PreferencesStore kv(prefs);
  clearBookMetadata(kv, prefKeyForBook(path)); // NVS: progress + bookmarks
  deletePageCacheForBook(path);                // disk: pc_<hash>.bin
}

void migrateBookMetadata(const String &oldPath, const String &newPath) {
  PreferencesStore kv(prefs);
  renameBookMetadata(kv, prefKeyForBook(oldPath), prefKeyForBook(newPath));
  renamePageCacheForBook(oldPath, newPath);
}
#endif // ARDUINO
