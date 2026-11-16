#ifndef PALA_STORAGE_BOOK_METADATA_H
#define PALA_STORAGE_BOOK_METADATA_H

#include "src/storage/kv_store.h"
#include "src/pure/bookmarks_codec.h"
#include "src/pure/hashing.h"

// All per-book NVS state — explicit bookmarks AND implicit saved-page
// progress — lives behind the same hash-keyed scheme (`<hash>_p`,
// `<hash>_bm`) and follows the same lifecycle (cleared on book delete,
// migrated on rename, invalidated on layout change), so it shares a file.

// ============================================================================
//  Testable KV-store-backed API (host-compatible)
// ============================================================================

// Load the bookmarks for `bookKey` from `kv`. Decodes both v1 (legacy,
// 2-byte/entry) and v2 (current, 6-byte/entry) blobs.
Bookmarks loadBookmarks(KeyValueStore &kv, const String &bookKey);

// Save `bm` for `bookKey` into `kv` using the v2 format.
void saveBookmarks(KeyValueStore &kv, const String &bookKey, const Bookmarks &bm);

// Per-book reading progress.
//
// The canonical position is the **byte offset** in the source file — that's
// invariant under font/layout changes. The page-number sibling is kept as a
// stale display hint (used by the web file list and as a legacy fallback for
// books saved by older firmware that didn't store offsets).
//
// `loadSavedOffset` returns `kOffsetUnset` if no offset has been saved yet.
int loadSavedPage(KeyValueStore &kv, const String &bookKey);
void saveSavedPage(KeyValueStore &kv, const String &bookKey, int pageIndex);
uint32_t loadSavedOffset(KeyValueStore &kv, const String &bookKey);
void saveSavedOffset(KeyValueStore &kv, const String &bookKey, uint32_t byteOffset);

// Remove all metadata for one book (progress + bookmarks). Returns true if
// anything was removed.
bool clearBookMetadata(KeyValueStore &kv, const String &bookKey);

// Move metadata (progress + bookmarks) from oldKey to newKey.
void renameBookMetadata(KeyValueStore &kv, const String &oldKey, const String &newKey);

#ifdef ARDUINO
// ============================================================================
//  Firmware glue — wraps the testable API with a PreferencesStore around
//  the global `prefs` so callers can keep their existing call sites.
// ============================================================================
#include "src/config.h"
#include "src/state.h"

uint8_t loadBookmarksForKey(const String &bookKey, uint32_t outPages[MAX_BOOKMARKS],
                            uint32_t outOffsets[MAX_BOOKMARKS]);
void saveBookmarksForKey(const String &bookKey, const uint32_t pages[MAX_BOOKMARKS],
                         const uint32_t offsets[MAX_BOOKMARKS], uint8_t count);
int savedPageForBookPath(const String &path);

// Per-book lifecycle. Compose the pure NVS operations above with the
// matching page-cache file work in storage/page_cache.h. They do NOT
// touch device wake state or `g_bookview` — those are higher-level
// concerns and callers run in contexts (web during upload) where the
// reader has already been fully cleared by `LibraryScreen::onEnter`.
void deleteBookMetadata(const String &path);
void migrateBookMetadata(const String &oldPath, const String &newPath);

// Throttled-save and bookmark-current-page helpers live in ui/reader.h —
// they manipulate g_bookview and belong on the ui side of the storage boundary.

#endif // ARDUINO

#endif // PALA_STORAGE_BOOK_METADATA_H
