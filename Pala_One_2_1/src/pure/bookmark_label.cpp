#include "bookmark_label.h"

static inline bool isBookmarkLabelWordChar(char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         ((uint8_t)c >= 128);
}

String readBookmarkLabelAtOffset(IReadStream& in, uint32_t off, int page) {
  if (!in.seek(off)) return String("p. ") + String(page + 1);

  String label;
  label.reserve(80);

  const int maxWords = 5;
  const int maxChars = 44;
  int words = 0;
  int scanned = 0;
  bool inWord = false;
  bool pendingSpace = false;

  while (in.available() && scanned < 240) {
    int rb = in.read();
    if (rb < 0) break;
    char c = (char)rb;
    scanned++;

    if (c == '\r') continue;
    if (c == '\n' || c == '\t') c = ' ';

    if (isBookmarkLabelWordChar(c)) {
      if (!inWord) {
        if (words >= maxWords) break;
        if (pendingSpace && label.length() > 0 && label.length() < (size_t)maxChars) label += ' ';
        pendingSpace = false;
        inWord = true;
      }
      if (label.length() < (size_t)maxChars) label += c;
      continue;
    }

    if (c == ' ') {
      if (inWord) {
        words++;
        inWord = false;
        pendingSpace = (words < maxWords);
        if (words >= maxWords) break;
      }
      continue;
    }

    if (inWord && label.length() < (size_t)maxChars) {
      label += c;
    }
  }

  label.trim();
  if (label.length() == 0) label = "Page";
  label += " - p. ";
  label += String(page + 1);
  return label;
}
