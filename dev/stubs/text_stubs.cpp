// Stubs for src/ui/text.h — the paginator/render functions that depend on the
// u8g2 display library. In the emulator these return simple approximations so
// the bookmark view and export pages can show *something*.

#include "src/ui/text.h"

uint32_t drawPageAt(File&, uint32_t startPos) {
  return startPos + 1000;
}

// Read up to ~800 bytes of raw text from the file at the given offset.
// Real firmware would word-wrap at pixel widths; emulator just reads bytes.
uint32_t extractPageText(File& f, uint32_t startPos, String& out) {
  if (!f) return startPos;
  f.seek(startPos);
  const int kMax = 800;
  uint8_t buf[kMax];
  size_t n = f.read(buf, kMax);
  for (size_t i = 0; i < n; i++) {
    char c = (char)buf[i];
    if (c != '\r') out += c;
  }
  return startPos + (uint32_t)n;
}

uint32_t nextPageOffset(File&, uint32_t startPos) {
  return startPos + 1000;
}

uint32_t pageOffsetForPage(File&, const String&, int page) {
  return (uint32_t)page * 1000u;
}

uint32_t resolveBookmarkOffset(const String&, uint16_t page, uint32_t storedOffset) {
  return (storedOffset != 0xFFFFFFFFu) ? storedOffset : (uint32_t)page * 1000u;
}
