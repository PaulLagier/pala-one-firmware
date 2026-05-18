#pragma once
// Emulator override: exposes firmware-glue functions without #ifdef ARDUINO.
// Definitions live in dev/stubs/book_metadata_glue.cpp.

#include "src/storage/kv_store.h"
#include "src/pure/bookmarks_codec.h"
#include "src/pure/hashing.h"

// ---------------------------------------------------------------------------
//  Testable KV-store-backed API (identical to real header)
// ---------------------------------------------------------------------------
Bookmarks loadBookmarks(KeyValueStore& kv, const String& bookKey);
void      saveBookmarks(KeyValueStore& kv, const String& bookKey, const Bookmarks& bm);

int      loadSavedPage(KeyValueStore& kv, const String& bookKey);
void     saveSavedPage(KeyValueStore& kv, const String& bookKey, int pageIndex);
uint32_t loadSavedOffset(KeyValueStore& kv, const String& bookKey);
void     saveSavedOffset(KeyValueStore& kv, const String& bookKey, uint32_t byteOffset);

bool clearBookMetadata(KeyValueStore& kv, const String& bookKey);
void renameBookMetadata(KeyValueStore& kv, const String& oldKey, const String& newKey);

// ---------------------------------------------------------------------------
//  Firmware-glue API — exposed unconditionally in the emulator
// ---------------------------------------------------------------------------
#include "src/config.h"

uint8_t loadBookmarksForKey(const String& bookKey,
                            uint16_t outPages[MAX_BOOKMARKS],
                            uint32_t outOffsets[MAX_BOOKMARKS]);
void saveBookmarksForKey(const String& bookKey,
                         const uint16_t pages[MAX_BOOKMARKS],
                         const uint32_t offsets[MAX_BOOKMARKS],
                         uint8_t count);
int  savedPageForBookPath(const String& path);
void deleteBookMetadata(const String& path);
void migrateBookMetadata(const String& oldPath, const String& newPath);
