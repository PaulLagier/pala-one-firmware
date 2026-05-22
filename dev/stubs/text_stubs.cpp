// Stubs for src/ui/text.h. The emulator uses the real pure paginator
// (paginatePage) with a host-side character-width approximation for
// helvR12_te so page boundaries match the device closely.

#include "src/ui/text.h"
#include "src/ui/font.h"
#include "src/pure/find_text.h"
#include "src/pure/paginator.h"
#include "src/hal/file_stream.h"

// Approximate per-glyph pixel widths for u8g2_font_helvR12_te.
// Non-ASCII bytes (UTF-8 continuations) return 0; lead bytes return ~7.
static int hostMeasureWidth(const char* s) {
  static const uint8_t w[128] = {
  //  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 00
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 10
      3,  4,  5,  9,  7, 10,  9,  3,  4,  4,  6,  9,  4,  5,  4,  5, // 20  !"#$%&'()*+,-./
      7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  4,  4,  9,  9,  9,  6, // 30  0-9:;<=>?
     12,  8,  8,  8,  9,  7,  7,  9,  9,  3,  5,  8,  7, 10,  9,  9, // 40  @A-O
      7,  9,  8,  7,  7,  9,  8, 11,  8,  7,  8,  4,  5,  4,  9,  7, // 50  P-Z[\]^_
      5,  7,  7,  6,  7,  7,  4,  7,  7,  3,  3,  7,  3, 10,  7,  7, // 60  `a-o
      7,  7,  5,  6,  5,  7,  7, 10,  7,  7,  6,  5,  3,  5,  9,  0, // 70  p-z{|}~
  };
  int total = 0;
  while (*s) {
    unsigned char c = (unsigned char)*s++;
    if (c < 0x80)       total += w[c];       // ASCII
    else if (c >= 0xC0) total += 7;          // UTF-8 lead byte — non-ASCII char
    // continuation bytes (0x80–0xBF) contribute 0
  }
  return total;
}

uint32_t drawPageAt(File& f, uint32_t startPos) {
  FileReadStream stream(f);
  return paginatePage(stream, startPos, Font::bodyLayout(), hostMeasureWidth, nullptr);
}

uint32_t extractPageText(File& f, uint32_t startPos, String& out) {
  FileReadStream stream(f);
  auto onLine = [&](const char* buf, size_t len) {
    const char* p = buf;
    size_t rem = len;
    while (rem > 0 && (*p == ' ' || *p == '\t')) { p++; rem--; }
    out.concat(p, (unsigned int)rem);
    out.concat('\n');
  };
  return paginatePage(stream, startPos, Font::bodyLayout(), hostMeasureWidth, onLine);
}

uint32_t nextPageOffset(File& f, uint32_t startPos) {
  FileReadStream stream(f);
  return paginatePage(stream, startPos, Font::bodyLayout(), hostMeasureWidth, nullptr);
}

uint32_t pageOffsetForPage(File& f, const String& /*path*/, int page) {
  if (page <= 0) return 0;
  uint32_t off = 0;
  for (int p = 0; p < page; p++) {
    uint32_t next = nextPageOffset(f, off);
    if (next <= off) break;
    off = next;
  }
  return off;
}

uint32_t resolveBookmarkOffset(const String& path, uint16_t page, uint32_t storedOffset) {
  File f = FS.open(path, "r");
  if (!f) return 0;
  uint32_t off = (storedOffset != 0xFFFFFFFFu) ? storedOffset
                                                : pageOffsetForPage(f, path, page);
  f.close();
  return off;
}

uint32_t pageOffsetForText(File& f, const String& /*path*/, const String& query,
                           int* outPageIndex) {
  if (outPageIndex) *outPageIndex = -1;
  if (query.length() == 0 || !f) return 0;

  FileReadStream stream(f);
  uint32_t matchOffset = findByteOffset(stream, query);
  if (matchOffset == 0xFFFFFFFFu) return 0xFFFFFFFFu;

  uint32_t pageStart = 0;
  int pageIdx = 0;
  while (pageStart < matchOffset) {
    uint32_t next = nextPageOffset(f, pageStart);
    if (next <= pageStart) break;
    if (next > matchOffset) break;
    pageStart = next;
    pageIdx++;
  }
  if (outPageIndex) *outPageIndex = pageIdx;
  return pageStart;
}
