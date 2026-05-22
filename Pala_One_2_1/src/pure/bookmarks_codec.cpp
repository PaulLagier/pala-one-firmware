#include "bookmarks_codec.h"

#include <cstring>

uint8_t decodeBookmarks(const uint8_t* buf, size_t got, Bookmarks& out) {
  out.count = 0;
  if (got < 1) return 0;

  uint8_t count = buf[0];
  if (count > MAX_BOOKMARKS) count = MAX_BOOKMARKS;

  bool hasOffsets = (got >= (size_t)(1 + count * 6));
  if (hasOffsets) {
    for (uint8_t i = 0; i < count; i++) {
      size_t base = 1u + (size_t)i * 6u;
      out.pages[i] = (uint16_t)(buf[base + 0] | (buf[base + 1] << 8));
      out.offsets[i] = (uint32_t)buf[base + 2] |
                       ((uint32_t)buf[base + 3] << 8) |
                       ((uint32_t)buf[base + 4] << 16) |
                       ((uint32_t)buf[base + 5] << 24);
    }
  } else {
    for (uint8_t i = 0; i < count; i++) {
      size_t base = 1u + (size_t)i * 2u;
      out.pages[i] = (uint16_t)(buf[base + 0] | (buf[base + 1] << 8));
      out.offsets[i] = kOffsetUnset;
    }
  }
  out.count = count;
  return count;
}

size_t encodeBookmarks(const Bookmarks& bm, uint8_t* outBuf) {
  uint8_t count = bm.count;
  if (count > MAX_BOOKMARKS) count = MAX_BOOKMARKS;

  std::memset(outBuf, 0, BOOKMARKS_ENCODED_MAX_SIZE);
  outBuf[0] = count;
  for (uint8_t i = 0; i < count; i++) {
    size_t base = 1u + (size_t)i * 6u;
    uint16_t page = bm.pages[i];
    uint32_t off  = bm.offsets[i];
    outBuf[base + 0] = (uint8_t)(page & 0xFF);
    outBuf[base + 1] = (uint8_t)((page >> 8) & 0xFF);
    outBuf[base + 2] = (uint8_t)(off & 0xFF);
    outBuf[base + 3] = (uint8_t)((off >> 8) & 0xFF);
    outBuf[base + 4] = (uint8_t)((off >> 16) & 0xFF);
    outBuf[base + 5] = (uint8_t)((off >> 24) & 0xFF);
  }
  return 1u + (size_t)count * 6u;
}

BookmarkAddResult addBookmark(Bookmarks& bm, uint16_t page, uint32_t offset) {
  for (uint8_t i = 0; i < bm.count; i++) {
    if (bm.pages[i] == page) return {false, D_TOAST_BOOKMARK_EXISTS};
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
        uint16_t tp = bm.pages[i];
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
