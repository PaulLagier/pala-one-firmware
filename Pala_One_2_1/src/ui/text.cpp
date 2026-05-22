#include "src/ui/text.h"

#include "src/hal/display.h"            // u8g2
#include "src/pure/bookmarks_codec.h"   // kOffsetUnset
#include "src/storage/page_cache.h"     // on-disk page-offset cache
#include "src/ui/font.h"                // Font::useBody / bodyLayout / current{BodySize,LineGap}

// Measure-width adapter so `paginatePage` can ask the u8g2 instance about
// rendered widths under the current font.
static int u8g2Measure(const char* s) {
  return u8g2.getUTF8Width(s);
}

// ============================================================================
//  Page primitives — one page at `startPos`, three flavors.
//
//  All set the body font before calling the paginator so the measure
//  function and the metrics describe the same face. Each returns the byte
//  offset where the next page begins.
// ============================================================================
uint32_t drawPageAt(File& f, uint32_t startPos) {
  FileReadStream stream(f);
  const LayoutMetrics& m = Font::bodyLayout();
  Font::useBody();

  int cursorY = TOP_PAD + m.ascent;
  auto onLine = [&](const char* buf, size_t /*len*/) {
    u8g2.setCursor(MARGIN_X, cursorY);
    u8g2.print(buf);
    cursorY += m.lineH;
  };

  return paginatePage(stream, startPos, m, u8g2Measure, onLine);
}

uint32_t extractPageText(File& f, uint32_t startPos, String& out) {
  FileReadStream stream(f);
  const LayoutMetrics& m = Font::bodyLayout();
  Font::useBody();

  auto onLine = [&](const char* buf, size_t len) {
    // Trim leading whitespace (paginator already trims trailing).
    const char* start = buf;
    size_t remaining = len;
    while (remaining > 0 && (*start == ' ' || *start == '\t')) { start++; remaining--; }
    out.concat(start, remaining);
    out.concat('\n');
  };

  return paginatePage(stream, startPos, m, u8g2Measure, onLine);
}

uint32_t nextPageOffset(File& f, uint32_t startPos) {
  FileReadStream stream(f);
  const LayoutMetrics& m = Font::bodyLayout();
  Font::useBody();
  return paginatePage(stream, startPos, m, u8g2Measure, nullptr);
}

// ============================================================================
//  Cross-book offset lookup
// ============================================================================
uint32_t pageOffsetForPage(File& f, const String& path, int page) {
  if (page < 0) page = 0;

  // On-disk fast path: O(1) seek to the highest cached page <= target.
  // The active reader keeps this file fresh for books it visits; cross-book
  // lookups (web bookmark resolve, page-text export) ride along.
  uint32_t off = 0;
  int startPage = loadOffsetForPageFromDisk(path, f.size(),
                                            Font::currentBodySize(), Font::currentLineGap(),
                                            page, &off);
  if (startPage < 0) startPage = 0;

  for (int p = startPage; p < page; p++) {
    uint32_t next = nextPageOffset(f, off);
    if (next == off) break;
    off = next;
  }
  return off;
}

uint32_t resolveBookmarkOffset(const String& path, uint16_t page, uint32_t storedOffset) {
  File f = FS.open(path, "r");
  if (!f) return 0;

  size_t size = f.size();
  if (storedOffset != kOffsetUnset && storedOffset < size) {
    f.close();
    return storedOffset;
  }

  uint32_t off = pageOffsetForPage(f, path, page);
  f.close();
  return off;
}
