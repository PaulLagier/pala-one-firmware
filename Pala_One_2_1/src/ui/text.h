#ifndef PALA_UI_TEXT_H
#define PALA_UI_TEXT_H

#include "src/config.h"
#include "src/state.h"

// Re-export pure modules so existing callers that include "ui/text.h" keep
// working unchanged.
#include "src/pure/text_util.h"
#include "src/pure/paginator.h"
#include "src/pure/bookmark_label.h"
#include "src/hal/packed_file_stream.h"
#include "src/ui/print_state.h"

// ============================================================================
//  Page-level operations on a book file.
//
//  Three primitives that take an open `File&` + a starting byte offset and
//  do exactly one thing each, all returning the next page's start offset:
//
//    drawPageAt        — render one page to the e-ink at the body font
//    extractPageText   — append one page's text (newline-separated) to `out`
//    nextPageOffset    — compute the next page boundary, no draw, no capture
//
//  All three use the pure paginator with a u8g2 width-measure adapter under
//  the current Font::useBody(). None touch `g_bookview` — they're pure on
//  the file argument.
//
//  Above those, two cross-book helpers:
//
//    pageOffsetForPage     — locate page N in an arbitrary file (uses disk cache)
//    resolveBookmarkOffset — return a stored bookmark offset, or look it up
// ============================================================================
void clearPagePrefetch();
void prefetchPage(IReadStream &stream, uint32_t startPos);
uint32_t drawPageAt(File &f, uint32_t startPos, PrintState::State &state);
uint32_t drawPageAt(IReadStream &stream, uint32_t startPos, PrintState::State &state);
uint32_t extractPageText(File &f, uint32_t startPos, String &out);
uint32_t nextPageOffset(File &f, uint32_t startPos);
uint32_t nextPageOffset(IReadStream &stream, uint32_t startPos);
// Find page N's byte offset in `f` (which must be opened to `path`). Reads
// the on-disk page cache for an O(1) start, walks forward via
// `nextPageOffset` if the requested page is past the cached portion.
using PageLookupProgress = bool (*)(uint32_t current, uint32_t target, void *context);
uint32_t pageOffsetForPage(File &f, const String &path, int page,
                           PageLookupProgress progress = nullptr, void *context = nullptr);

// Returns `storedOffset` if it's a usable cached value for this book; falls
// back to `pageOffsetForPage` otherwise. Owns the file handle internally.
uint32_t resolveBookmarkOffset(const String &path, uint32_t page, uint32_t storedOffset,
                               PageLookupProgress progress = nullptr, void *context = nullptr);

#endif // PALA_UI_TEXT_H
