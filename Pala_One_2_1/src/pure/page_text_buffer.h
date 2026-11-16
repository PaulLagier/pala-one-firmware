#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Bounded prefetch storage. Overflow invalidates the candidate; rendering
// always falls back to pagination rather than showing truncated text.
struct PageTextBuffer {
  static constexpr size_t capacity = 4096;
  static constexpr size_t maxLines = 32;
  char bytes[capacity];
  uint16_t starts[maxLines];
  size_t used = 0, count = 0;
  bool valid = true;
  void clear() { used = count = 0; valid = true; }
  void append(const char* text, size_t len) {
    if (!valid) return;
    if (count == maxLines || len >= capacity - used) { valid = false; return; }
    starts[count++] = static_cast<uint16_t>(used);
    memcpy(bytes + used, text, len);
    used += len; bytes[used++] = 0;
  }
  const char* line(size_t index) const { return bytes + starts[index]; }
};
