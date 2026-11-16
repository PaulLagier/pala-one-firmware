#include "src/hal/cpu_burst.h"
#include "src/ui/text.h"
#include "src/ui/body_page.h"

#include "src/hal/display.h"            // u8g2
#include "src/pure/bookmarks_codec.h"   // kOffsetUnset
#include "src/storage/page_cache.h"     // on-disk page-offset cache
#include "src/ui/font.h"                // Font::useBody / bodyLayout / measureLine / layoutForCache / PrintState

// Measure-width adapter for the paginator. Routes through Font::measureBionicLine
// so the bionic 1-px-per-split-word adjustment is folded into the same width
// budget the paginator wraps against — without bionic this collapses to a
// plain getUTF8Width under the Body face.
// Main-thread staging avoids repeating an already validated page layout.
// Static storage keeps the bounded 4 KiB buffer off the Arduino task stack.
static PageTextBuffer s_pageText;
static IReadStream *s_preparedStream = nullptr;
static uint32_t s_preparedStart = 0, s_preparedNext = 0;
static size_t s_preparedSize = 0;
static LayoutMetrics s_preparedMetrics{};
static PageCacheLayout s_preparedLayout{};
static bool sameLayout(const PageCacheLayout &a, const PageCacheLayout &b) {
  return a.bodySize == b.bodySize && a.lineGap == b.lineGap && a.family == b.family &&
         a.bionic == b.bionic && a.halfGaps == b.halfGaps &&
         a.statusbarReserve == b.statusbarReserve && a.markdown == b.markdown;
}
void clearPagePrefetch() { s_preparedStream = nullptr; }
void prefetchPage(IReadStream &stream, uint32_t start) {
  const auto layout = Font::layoutForCache();
  if (s_preparedStream == &stream && s_preparedStart == start && s_preparedSize == stream.size() &&
      s_pageText.valid && sameLayout(layout, s_preparedLayout))
    return;
  clearPagePrefetch();
  if (start >= stream.size()) {
    s_pageText.clear();
    s_pageText.valid = false;
    s_preparedMetrics = Font::bodyLayout();
    s_preparedNext = start;
    return;
  }
  CpuBurst cpu;
  s_preparedNext = prepareBodyPage(stream, start, s_preparedMetrics, &s_pageText);
  s_preparedStart = start;
  s_preparedSize = stream.size();
  s_preparedLayout = layout;
  if (s_pageText.valid)
    s_preparedStream = &stream;
}

static int bodyMeasure(const char *s) { return Font::measureLine(s); }

// ============================================================================
//  Page primitives — one page at `startPos`, three flavors.
//
//  All set the body font before calling the paginator so the measure
//  function and the metrics describe the same face. Each returns the byte
//  offset where the next page begins.
// ============================================================================
uint32_t drawPageAt(File &f, uint32_t startPos) {
  BookReadStream stream(f);
  clearPagePrefetch();
  const auto next = drawPageAt(stream, startPos);
  clearPagePrefetch();
  return next;
}

uint32_t drawPageAt(IReadStream &stream, uint32_t startPos) {
  prefetchPage(stream, startPos);
  LayoutMetrics m = s_preparedMetrics;
  const uint32_t next = s_preparedNext;
  Font::useBody();

  Font::PrintState state = Font::PrintState();
  state.isBold = false;
  int cursorY = TOP_PAD + m.ascent;
  auto onLine = [&](const char *buf, size_t len) {
    // len 0 indicates a paragraph gap. Advance by gap height.
    if (len == 0) {
      cursorY += m.paragraphGapH;
      return;
    }
    // drawLine sets the active u8g2 font back to Body before returning,
    // so the next line measurement (via bodyMeasure) is consistent.
    Font::drawLine(MARGIN_X, cursorY, buf, state);
    cursorY += m.lineH;
  };

  if (s_pageText.valid) {
    for (size_t i = 0; i < s_pageText.count; ++i) {
      const char *line = s_pageText.line(i);
      onLine(line, strlen(line));
    }
    return next;
  }
  // A pathological page must fall back to direct pagination, never truncation.
  return paginatePage(stream, startPos, m, bodyMeasure, onLine);
}

uint32_t extractPageText(File &f, uint32_t startPos, String &out) {
  BookReadStream stream(f);
  LayoutMetrics m;
  prepareBodyPage(stream, startPos, m);
  Font::useBody();

  auto onLine = [&](const char *buf, size_t len) {
    // len 0 indicates paragraph break.
    if (len == 0) {
      out.concat("\n");
      return;
    }
    // Trim leading whitespace (paginator already trims trailing).
    const char *start = buf;
    size_t remaining = len;
    while (remaining > 0 && (*start == ' ' || *start == '\t')) {
      start++;
      remaining--;
    }
    out.concat(start, remaining);
    out.concat("\n");
  };

  return paginatePage(stream, startPos, m, bodyMeasure, onLine);
}

uint32_t nextPageOffset(File &f, uint32_t startPos) {
  BookReadStream stream(f);
  return nextPageOffset(stream, startPos);
}

uint32_t nextPageOffset(IReadStream &stream, uint32_t startPos) {
  LayoutMetrics m;
  return prepareBodyPage(stream, startPos, m);
}

// ============================================================================
//  Cross-book offset lookup
// ============================================================================
uint32_t pageOffsetForPage(File &f, const String &path, int page, PageLookupProgress progress,
                           void *context) {
  if (page < 0)
    page = 0;

  // On-disk fast path: O(1) seek to the highest cached page <= target.
  // The active reader keeps this file fresh for books it visits; cross-book
  // lookups (web bookmark resolve, page-text export) ride along.
  uint32_t off = 0;
  BookReadStream stream(f);
  int startPage =
      loadOffsetForPageFromDisk(path, stream.size(), Font::layoutForCache(), page, &off);
  if (startPage < 0)
    startPage = 0;

  CpuBurst cpu;
  for (int p = startPage; p < page; p++) {
    if (progress && !progress(uint32_t(p), uint32_t(page), context))
      return kOffsetUnset;
    uint32_t next = nextPageOffset(stream, off);
    if (next == off)
      break;
    off = next;
    if ((p & 31) == 0)
      delay(1);
  }
  return off;
}

uint32_t resolveBookmarkOffset(const String &path, uint32_t page, uint32_t storedOffset,
                               PageLookupProgress progress, void *context) {
  File f = FS.open(path, "r");
  if (!f)
    return 0;

  size_t size = bookLogicalSize(f);
  if (storedOffset != kOffsetUnset && storedOffset < size) {
    f.close();
    return storedOffset;
  }

  uint32_t off = pageOffsetForPage(f, path, page, progress, context);
  f.close();
  return off;
}
