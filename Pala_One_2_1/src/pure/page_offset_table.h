#ifndef PALA_PURE_PAGE_OFFSET_TABLE_H
#define PALA_PURE_PAGE_OFFSET_TABLE_H
#include "arduino_compat.h"
#include "../config.h"
#include <assert.h>
#include <string.h>

// Bounded window of consecutive page starts. Page numbers stay absolute;
// offsets[] is local to firstPage. The oldest half rolls out when full.
// 10000 is a RAM capacity, never a limit on the length of a book.
struct PageOffsetTable {
  uint32_t offsets[MAX_PAGES];
  int count = 0;
  int firstPage = 0;
  bool eofReached = false;

  int endPage() const { return firstPage + count; } // exclusive
  bool contains(int page) const { return page >= firstPage && page < endPage(); }
  uint32_t offsetAt(int page) const {
    assert(contains(page));
    return offsets[page - firstPage];
  }
  void reset() {
    firstPage = 0;
    count = 1;
    offsets[0] = 0;
    eofReached = false;
  }
  bool append(uint32_t next, size_t fileSize) {
    if (count < 1 || eofReached)
      return false;
    if (next <= offsets[count - 1] || next >= fileSize) {
      eofReached = true;
      return false;
    }
    if (count == MAX_PAGES) {
      constexpr int drop = MAX_PAGES / 2;
      memmove(offsets, offsets + drop, (count - drop) * sizeof(uint32_t));
      firstPage += drop;
      count -= drop;
    }
    offsets[count++] = next;
    return true;
  }
};
#endif
