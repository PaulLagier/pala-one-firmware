#include "find_text.h"

uint32_t find_text(const String& text, const String& pattern) {
  const unsigned text_len = text.length();
  const unsigned pattern_len = pattern.length();

  // Edge cases: empty pattern matches at the start; pattern longer than text can't match.
  if (pattern_len == 0) {
    return 0;
  }
  if (pattern_len > text_len) {
    return 0xFFFFFFFFu;
  }

  const char* text_data = text.c_str();
  const char* pattern_data = pattern.c_str();

  // Table of how far to skip ahead when a mismatch occurs on a given byte.
  unsigned shift[256];

  // Initialize all shifts to the pattern length (the max)
  for (unsigned i = 0; i < sizeof(shift) / sizeof(shift[0]); ++i) {
    shift[i] = pattern_len;
  }

  // For each pattern byte except the last, record how far we can skip
  // when that byte appears as the mismatch character.
  for (unsigned i = 0; i + 1 < pattern_len; ++i) {
    shift[static_cast<unsigned char>(pattern_data[i])] = pattern_len - 1 - i;
  }

  unsigned index = 0;
  while (index <= text_len - pattern_len) {
    // Compare from the end of the pattern backwards.
    unsigned pos = pattern_len - 1;
    while (pos < pattern_len && pattern_data[pos] == text_data[index + pos]) {
      if (pos == 0) {
        return static_cast<uint32_t>(index);
      }
      --pos;
    }
    index += shift[static_cast<unsigned char>(text_data[index + pattern_len - 1])];
  }

  return 0xFFFFFFFFu;
}

static String asciiLower(const String& s) {
  String out;
  const char* p = s.c_str();
  while (*p) {
    char c = *p++;
    if (c >= 'A' && c <= 'Z') c += 32;
    out += c;
  }
  return out;
}

uint32_t findByteOffset(IReadStream& in, const String& pattern, size_t chunkSize) {
  const unsigned patLen = pattern.length();
  if (patLen == 0) return 0;
  const unsigned overlap = patLen - 1;

  // Normalise once; search windows are lowercased per chunk below.
  const String lowerPat = asciiLower(pattern);

  in.seek(0);
  String tail;       // original bytes — preserves correct byte offsets
  String lowerTail;  // lowercased mirror of tail

  while (in.available()) {
    uint32_t chunkStart = in.position();
    String newBytes;
    for (size_t i = 0; i < chunkSize && in.available(); ++i) {
      int b = in.read();
      if (b < 0) break;
      newBytes += (char)(uint8_t)b;
    }
    if (newBytes.length() == 0) break;

    // searchStr[0] is at absolute byte searchBase.
    uint32_t searchBase = chunkStart - (uint32_t)tail.length();
    String searchStr   = tail;       searchStr   += newBytes;
    String lowerSearch = lowerTail;  lowerSearch += asciiLower(newBytes);

    uint32_t pos = find_text(lowerSearch, lowerPat);
    if (pos != 0xFFFFFFFFu) return searchBase + pos;

    if (overlap > 0 && searchStr.length() > overlap) {
      tail      = searchStr  .substring(searchStr  .length() - overlap);
      lowerTail = lowerSearch.substring(lowerSearch.length() - overlap);
    } else {
      tail      = searchStr;
      lowerTail = lowerSearch;
    }
  }

  return 0xFFFFFFFFu;
}
