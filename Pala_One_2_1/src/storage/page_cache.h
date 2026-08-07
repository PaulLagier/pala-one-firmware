#ifndef PALA_STORAGE_PAGE_CACHE_H
#define PALA_STORAGE_PAGE_CACHE_H

#include "src/config.h"
#include "src/state.h"
#include "src/pure/page_offset_table.h"

// ============================================================================
//  On-disk page-offset cache (pc_<hash>.bin files in LittleFS root)
//
//  Per-book binary file mapping page index -> byte offset, stamped with the
//  PageCacheLayout under which the offsets were computed.
//  Load rejects mismatched files; stale entries are silently overwritten on
//  the next save. No external "invalidate everything" pass needed on font or
//  layout change — layout-correctness is a property of the file format itself.
//
//  Layout values come from `Font::layoutForCache()` (see font.h). Anything
//  that changes how the body paginates (body size, line gap, font family,
//  bionic, statusbar reserve height) flows into that one struct, so this
//  module never has to know what specifically affects layout.
//
//  The active reader bulk-loads the whole table into `g_bookview.pages` via
//  `loadPageOffsetCacheForBook`. Cross-book lookups (web bookmark resolve,
//  page-text export) use the lighter `loadOffsetForPageFromDisk` to read
//  one entry without allocating a 40 KB scratch table.
// ============================================================================

// Anything that affects page layout — and therefore must invalidate the
// cache when changed. Body face is identified by the same (size, family,
// bionic) tuple the Font module exposes; line gap is the user's spacing
// preference; statusbar reserve is how many pixels the bottom statusbar
// steals from the text area (which changes the page's `maxLines`).
// Packed into the on-disk header at save time; the load path rejects files
// whose stamp doesn't match the *current* layout.
//
// New fields go at the end and bump the on-disk magic in page_cache.cpp so
// older caches are silently rejected and re-built on the next save.
struct PageCacheLayout {
  int     bodySize;          // 8/10/12/14
  int     lineGap;           // [0, 4]
  uint8_t family;            // matches Font::Family numeric value (0 = Helv, 1 = Dys)
  uint8_t bionic;            // 0 / 1
  uint8_t halfGaps;          // 0 / 1 - half-height paragraph gaps
  uint8_t statusbarReserve;  // pixels reserved at the bottom; from Statusbar::reserveH()
};

// Bulk-load the persisted offset table for `path` into `out`. Layout-stamped
// at save time; load rejects any file whose stamp doesn't match `layout`.
// Returns true on success; on false, `out` is left untouched (callers
// typically seed `offsets[0]=0, count=1` themselves).
bool loadPageOffsetCacheForBook(const String& path, size_t expectedSize,
                                const PageCacheLayout& layout,
                                PageOffsetTable& out);

// Persist `in` for `path`, stamped with `layout`. No-op if `in.count <= 1`
// (nothing useful to save).
void savePageOffsetCacheForBook(const String& path, size_t fileSize,
                                const PageCacheLayout& layout,
                                const PageOffsetTable& in);

// Single-entry on-disk lookup: read header, validate magic + layout +
// expected file size, and return the offset of the largest cached page
// `<= maxPage` along with that page's index. Constant-RAM (no PageOffsetTable
// scratch); two short reads (header + one offset). Returns -1 (and leaves
// `*out` untouched) if the cache file is absent, stamped for a different
// layout, sized for a different file, or has zero entries.
int loadOffsetForPageFromDisk(const String& path, size_t expectedSize,
                              const PageCacheLayout& layout,
                              int maxPage, uint32_t* out);

// Remove the on-disk page-cache file for `path` (no-op if absent).
void deletePageCacheForBook(const String& path);

// Move the on-disk page-cache file from `oldPath` to `newPath` (no-op if
// no source file). If a stale destination exists, it's removed first.
void renamePageCacheForBook(const String& oldPath, const String& newPath);

#endif  // PALA_STORAGE_PAGE_CACHE_H
