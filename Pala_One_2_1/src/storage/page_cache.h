#ifndef PALA_STORAGE_PAGE_CACHE_H
#define PALA_STORAGE_PAGE_CACHE_H

#include "src/config.h"
#include "src/state.h"
#include "src/pure/page_offset_table.h"

// ============================================================================
//  On-disk page-offset cache (pc_<hash>.bin files in LittleFS root)
//
//  Per-book binary file mapping page index -> byte offset, stamped with the
//  layout (font size + line gap) under which the offsets were computed.
//  Load rejects mismatched files; stale entries are silently overwritten on
//  the next save. No external "invalidate everything" pass needed on font or
//  file change — layout-correctness is a property of the file format itself.
//
//  The active reader bulk-loads the whole table into `g_bookview.pages` via
//  `loadPageOffsetCacheForBook`. Cross-book lookups (web bookmark resolve,
//  page-text export) use the lighter `loadOffsetForPageFromDisk` to read
//  one entry without allocating a 40 KB scratch table.
// ============================================================================

// Bulk-load the persisted offset table for `path` into `out`. Layout-stamped
// at save time; load rejects any file whose stamp doesn't match
// `(bodySize, lineGap)`. Returns true on success; on false, `out` is left
// untouched (callers typically seed `offsets[0]=0, count=1` themselves).
bool loadPageOffsetCacheForBook(const String& path, size_t expectedSize,
                                int bodySize, int lineGap,
                                PageOffsetTable& out);

// Persist `in` for `path`, stamped with `(bodySize, lineGap)`. No-op if
// `in.count <= 1` (nothing useful to save).
void savePageOffsetCacheForBook(const String& path, size_t fileSize,
                                int bodySize, int lineGap,
                                const PageOffsetTable& in);

// Single-entry on-disk lookup: read header, validate magic + layout +
// expected file size, and return the offset of the largest cached page
// `<= maxPage` along with that page's index. Constant-RAM (no PageOffsetTable
// scratch); two short reads (header + one offset). Returns -1 (and leaves
// `*out` untouched) if the cache file is absent, stamped for a different
// layout, sized for a different file, or has zero entries.
int loadOffsetForPageFromDisk(const String& path, size_t expectedSize,
                              int bodySize, int lineGap,
                              int maxPage, uint32_t* out);

// Remove the on-disk page-cache file for `path` (no-op if absent).
void deletePageCacheForBook(const String& path);

// Move the on-disk page-cache file from `oldPath` to `newPath` (no-op if
// no source file). If a stale destination exists, it's removed first.
void renamePageCacheForBook(const String& oldPath, const String& newPath);

#endif  // PALA_STORAGE_PAGE_CACHE_H
