#pragma once
#include <stddef.h>
#include <stdint.h>

// Small, allocation-free cluster helpers for the reader's supported scripts.
// Accents remain attached when choosing a Bionic prefix or breaking long words.
namespace PalaUtf8 {
inline size_t decode(const char *text, size_t length, size_t index, uint32_t &cp) {
  if (!text || index >= length) {
    cp = 0;
    return 0;
  }
  const auto first = static_cast<uint8_t>(text[index]);
  size_t bytes = first < 0x80                     ? 1
                 : first >= 0xC2 && first <= 0xDF ? 2
                 : first >= 0xE0 && first <= 0xEF ? 3
                 : first >= 0xF0 && first <= 0xF4 ? 4
                                                  : 1;
  cp = bytes == 1 ? first : first & ((1u << (7 - bytes)) - 1);
  if (index + bytes > length) {
    cp = first;
    return 1;
  }
  for (size_t n = 1; n < bytes; ++n) {
    const auto next = static_cast<uint8_t>(text[index + n]);
    if ((next & 0xC0) != 0x80) {
      cp = first;
      return 1;
    }
    cp = (cp << 6) | (next & 0x3F);
  }
  if ((bytes == 2 && cp < 0x80) || (bytes == 3 && cp < 0x800) || (bytes == 4 && cp < 0x10000) ||
      (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
    cp = first;
    return 1;
  }
  return bytes;
}
inline bool combining(uint32_t cp) {
  return (cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) ||
         (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) ||
         (cp >= 0xFE20 && cp <= 0xFE2F);
}
inline size_t clusterBytes(const char *text, size_t length, size_t index) {
  uint32_t cp;
  size_t end = index + decode(text, length, index, cp);
  if (end == index)
    return 0;
  while (end < length) {
    const auto bytes = decode(text, length, end, cp);
    if (!bytes || !combining(cp))
      break;
    end += bytes;
  }
  return end - index;
}
inline bool wordStart(uint32_t cp) {
  return (cp >= '0' && cp <= '9') || (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') ||
         (cp >= 0xC0 && cp <= 0x2AF && cp != 0xD7 && cp != 0xF7) || (cp >= 0x400 && cp <= 0x52F);
}
inline int bionicPrefixBytes(const char *text, int length) {
  if (!text || length <= 0)
    return 0;
  uint32_t cp;
  decode(text, size_t(length), 0, cp);
  if (!wordStart(cp))
    return 0;
  size_t index = 0;
  int count = 0;
  while (index < size_t(length)) {
    index += clusterBytes(text, size_t(length), index);
    ++count;
  }
  int strong = count <= 3 ? 1 : count / 2;
  if (strong > 4)
    strong = 4;
  index = 0;
  for (int n = 0; n < strong && index < size_t(length); ++n)
    index += clusterBytes(text, size_t(length), index);
  return index < size_t(length) ? int(index) : 0;
}
} // namespace PalaUtf8
