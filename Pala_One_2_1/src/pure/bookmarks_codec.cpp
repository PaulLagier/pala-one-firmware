#include "bookmarks_codec.h"

#include <cstring>

namespace {
uint32_t readValue(const uint8_t *bytes, size_t n) {
  uint32_t value = 0;
  for (size_t i = 0; i < n; ++i)
    value |= uint32_t(bytes[i]) << (8 * i);
  return value;
}
void writeValue(uint8_t *bytes, uint32_t value, size_t n) {
  for (size_t i = 0; i < n; ++i)
    bytes[i] = uint8_t(value >> (8 * i));
}
} // namespace
uint8_t decodeBookmarks(const uint8_t *buf, size_t got, Bookmarks &out) {
  out.count = 0;
  if (!buf || got < 1)
    return 0;
  const bool wide = (buf[0] & 0x80) != 0;
  uint8_t count = wide ? buf[0] & 0x7f : buf[0];
  if (wide && count > MAX_BOOKMARKS)
    return 0;
  if (count > MAX_BOOKMARKS)
    count = MAX_BOOKMARKS;
  const bool offsets = wide || got >= 1u + size_t(count) * 6u;
  const size_t pageBytes = wide ? 4u : 2u, stride = offsets ? pageBytes + 4u : pageBytes;
  // A partial record must never be read as a complete bookmark.
  if (got < 1u + size_t(count) * stride)
    return 0;
  for (uint8_t i = 0; i < count; ++i) {
    const size_t base = 1u + size_t(i) * stride;
    out.pages[i] = readValue(buf + base, pageBytes);
    out.offsets[i] = offsets ? readValue(buf + base + pageBytes, 4) : kOffsetUnset;
  }
  out.count = count;
  return count;
}
size_t encodeBookmarks(const Bookmarks &bm, uint8_t *outBuf) {
  uint8_t count = bm.count;
  if (count > MAX_BOOKMARKS)
    count = MAX_BOOKMARKS;
  bool wide = false;
  for (uint8_t i = 0; i < count; ++i)
    wide = wide || bm.pages[i] > 0xffff;
  const size_t pageBytes = wide ? 4u : 2u, stride = pageBytes + 4u;
  std::memset(outBuf, 0, BOOKMARKS_ENCODED_MAX_SIZE);
  outBuf[0] = count | (wide ? 0x80 : 0);
  for (uint8_t i = 0; i < count; ++i) {
    const size_t base = 1u + size_t(i) * stride;
    writeValue(outBuf + base, bm.pages[i], pageBytes);
    writeValue(outBuf + base + pageBytes, bm.offsets[i], 4);
  }
  return 1u + size_t(count) * stride;
}

BookmarkAddResult addBookmark(Bookmarks &bm, uint32_t page, uint32_t offset) {
  for (uint8_t i = 0; i < bm.count; i++) {
    if (bm.pages[i] == page)
      return {false, D_TOAST_BOOKMARK_EXISTS};
  }

  if (bm.count < MAX_BOOKMARKS) {
    bm.pages[bm.count] = page;
    bm.offsets[bm.count] = offset;
    bm.count++;
  } else {
    // Evict the oldest (entry 0).
    for (uint8_t i = 1; i < MAX_BOOKMARKS; i++) {
      bm.pages[i - 1] = bm.pages[i];
      bm.offsets[i - 1] = bm.offsets[i];
    }
    bm.pages[MAX_BOOKMARKS - 1] = page;
    bm.offsets[MAX_BOOKMARKS - 1] = offset;
    bm.count = MAX_BOOKMARKS;
  }

  // Insertion sort by page (ascending). Keeps offsets[] aligned.
  for (uint8_t i = 0; i < bm.count; i++) {
    for (uint8_t j = (uint8_t)(i + 1); j < bm.count; j++) {
      if (bm.pages[j] < bm.pages[i]) {
        uint32_t tp = bm.pages[i];
        bm.pages[i] = bm.pages[j];
        bm.pages[j] = tp;
        uint32_t to = bm.offsets[i];
        bm.offsets[i] = bm.offsets[j];
        bm.offsets[j] = to;
      }
    }
  }

  return {true, D_TOAST_BOOKMARK_SAVED};
}
