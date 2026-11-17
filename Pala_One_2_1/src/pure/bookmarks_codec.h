#ifndef PALA_PURE_BOOKMARKS_CODEC_H
#define PALA_PURE_BOOKMARKS_CODEC_H

#include "arduino_compat.h"
#include "../config.h"

// Pure encoder/decoder for the bookmark byte blob stored in Preferences under
// "<bookKey>_bm". Three formats are supported:
//   v1 (legacy): 1 byte count + N * 2-byte page (offsets unknown)
//   v2 (current): 1 byte count + N * 6-byte (2-byte page + 4-byte offset)
//   v3: high bit on count + N * 8-byte (4-byte page + 4-byte offset)
// Ordinary bookmarks retain v2; v3 is emitted only beyond page 65535.

struct Bookmarks {
  uint32_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t count = 0;
};

// Size of the largest possible encoded blob (v3: 1 + MAX_BOOKMARKS * 8 bytes).
constexpr size_t BOOKMARKS_ENCODED_MAX_SIZE = 1u + (size_t)MAX_BOOKMARKS * 8u;

// Sentinel for "no byte offset known yet." Used for legacy v1 bookmark
// entries (no offset recorded) and for `loadSavedOffset` when no offset
// has been persisted. The firmware recomputes a real offset on demand.
constexpr uint32_t kOffsetUnset = 0xFFFFFFFFu;

// Decode `got` bytes from `buf` into `out`. Legacy v1 entries get an
// offset of `kOffsetUnset` (placeholder — firmware will recompute).
// Returns the decoded count (always == out.count).
uint8_t decodeBookmarks(const uint8_t *buf, size_t got, Bookmarks &out);

// Encode `bm` into `outBuf` (must be at least BOOKMARKS_ENCODED_MAX_SIZE bytes).
// Returns the byte length; existing small-page bookmarks keep their v2 encoding.
size_t encodeBookmarks(const Bookmarks &bm, uint8_t *outBuf);

// Result of inserting one bookmark. `added` is the success signal; `message`
// is a UI-facing string the caller can pass straight to the toast layer.
struct BookmarkAddResult {
  bool added;
  const char *message;
};

// Insert one bookmark at the current page/offset. If the bookmark list is
// full, the oldest bookmark is evicted. Returns `{true,  "Bookmark saved"}`
// on insertion or `{false, "Bookmark exists"}` if `page` is already bookmarked.
BookmarkAddResult addBookmark(Bookmarks &bm, uint32_t page, uint32_t offset);

// Optional reader action: remove a unique mark in [start, end), matching
// canonical offsets rather than stale page labels. Legacy marks use the page
// label only. Ambiguous matches are unchanged. Insertion retains the capacity
// and eviction rules, allowing repeated page hints at distinct canonical offsets.
struct BookmarkEditResult {
  bool changed;
  const char *message;
};
BookmarkEditResult toggleBookmarkForPage(Bookmarks &bm, uint32_t page, uint32_t start,
                                         uint32_t end);

#endif // PALA_PURE_BOOKMARKS_CODEC_H
