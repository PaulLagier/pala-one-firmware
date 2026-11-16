#include "text_util.h"

int utf8CharLenFromLead(uint8_t b) {
  if (b < 0x80) return 1;
  if ((b & 0xE0) == 0xC0) return 2;
  if ((b & 0xF0) == 0xE0) return 3;
  if ((b & 0xF8) == 0xF0) return 4;
  return 1;
}

int utf8SafeCharLenAt(const String& s, int index) {
  if (index < 0 || index >= (int)s.length()) return 0;
  uint8_t b0 = (uint8_t)s[index];
  int len = utf8CharLenFromLead(b0);
  if (index + len > (int)s.length()) return 1;
  for (int i = 1; i < len; i++) {
    if (!isUtf8ContinuationByte((uint8_t)s[index + i])) return 1;
  }
  return len;
}

int utf8SafeCharLenAt(const char* buf, size_t len, size_t index) {
  if (index >= len) return 0;
  uint8_t b0 = (uint8_t)buf[index];
  int clen = utf8CharLenFromLead(b0);
  if (index + (size_t)clen > len) return 1;
  for (int i = 1; i < clen; i++) {
    if (!isUtf8ContinuationByte((uint8_t)buf[index + i])) return 1;
  }
  return clen;
}

String utf8CharAt(const String& s, int index) {
  int len = utf8SafeCharLenAt(s, index);
  if (len <= 0) return String("");
  return s.substring(index, index + len);
}

static const char kEllipsis[] = "...";

static void rtrimSpaces(String& s) {
  while (s.length() > 0) {
    char last = s[s.length() - 1];
    if (last != ' ' && last != '\t') break;
    s.remove(s.length() - 1);
  }
}

String truncateWithEllipsis(const String& in, int maxWidth, int (*measure)(const char*)) {
  if (measure == nullptr) return in;
  if (measure(in.c_str()) <= maxWidth) return in;
  if (measure(kEllipsis) > maxWidth) return String("");

  // Grow one codepoint at a time, keeping the longest prefix whose
  // trimmed form still fits once the ellipsis is appended.
  int bestEnd = 0;
  int i = 0;
  while (i < (int)in.length()) {
    int charLen = utf8SafeCharLenAt(in, i);
    if (charLen <= 0) break;
    int end = i + charLen;

    String candidate = in.substring(0, end);
    rtrimSpaces(candidate);
    candidate += kEllipsis;
    if (measure(candidate.c_str()) > maxWidth) break;

    bestEnd = end;
    i = end;
  }

  String out = in.substring(0, bestEnd);
  rtrimSpaces(out);
  out += kEllipsis;
  return out;
}

bool isBreakableWhitespaceByte(char b) {
  return b == ' ' || b == '\n' || b == '\t';
}

bool isBreakablePunctuationByte(char b) {
  return b == '.' || b == ',' || b == ';' || b == ':' || b == '!' || b == '?' ||
         b == ')' || b == ']' || b == '}' || b == '-' || b == '/';
}

String normalizeTypography(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  size_t i = 0;

  while (i < in.length()) {
    uint8_t b0 = (uint8_t)in[i];

    // UTF-8 BOM
    if (i == 0 && b0 == 0xEF && i + 2 < in.length() &&
        (uint8_t)in[i + 1] == 0xBB && (uint8_t)in[i + 2] == 0xBF) {
      i += 3;
      continue;
    }

    // Non-breaking space -> normal space
    if (b0 == 0xC2 && i + 1 < in.length() && (uint8_t)in[i + 1] == 0xA0) {
      out += ' ';
      i += 2;
      continue;
    }

    // 0xC2 0xAB = U+00AB <<  (left guillemet)
    // 0xC2 0xBB = U+00BB >>  (right guillemet)
    if (b0 == 0xC2 && i + 1 < in.length()) {
      uint8_t b1 = (uint8_t)in[i + 1];
      if (b1 == 0xAB || b1 == 0xBB) { out += '"'; i += 2; continue; }
      // 0x91/0x92 are Windows-1252 smart quotes that sometimes leak through
      // despite being technically invalid UTF-8; matching them defensively
      // is harmless.
      if (b1 == 0x91 || b1 == 0x92) { out += '\''; i += 2; continue; }
    }

    if (b0 == 0xE2 && i + 2 < in.length()) {
      uint8_t b1 = (uint8_t)in[i + 1];
      uint8_t b2 = (uint8_t)in[i + 2];
      if (b1 == 0x80) {
        if (b2 == 0x98 || b2 == 0x99 || b2 == 0x9A || b2 == 0x9B) { out += '\''; i += 3; continue; }
        if (b2 == 0x9C || b2 == 0x9D || b2 == 0x9E || b2 == 0x9F || b2 == 0xB9 || b2 == 0xBA) { out += '"'; i += 3; continue; }
        if (b2 == 0x93 || b2 == 0x94 || b2 == 0x95) { out += '-'; i += 3; continue; }
        if (b2 == 0xA6) { out += "..."; i += 3; continue; }
      }
    }

    out += (char)b0;
    i++;
  }

  return out;
}

String compactText(const String& in,
                   bool* ioLastWasSpace,
                   int* ioNewlineCount,
                   bool trimTail) {
  String out;
  out.reserve(in.length());

  bool lastWasSpace = ioLastWasSpace ? *ioLastWasSpace : false;
  int newlineCount = ioNewlineCount ? *ioNewlineCount : 0;
  // Length of `out` up to (and including) its last non-space byte —
  // newlines count as non-space here. Lets newline emission strip
  // trailing spaces in O(1) without clobbering previously-emitted
  // newlines (which would break the "collapse runs to two" rule).
  size_t trimAnchor = 0;

  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];

    if (c == '\r') continue;
    if (c == '\t') c = ' ';

    if (c == '\n') {
      if (out.length() > trimAnchor) out.remove(trimAnchor);
      newlineCount++;
      if (newlineCount <= 2) {
        out += '\n';
        trimAnchor = out.length();
      }
      lastWasSpace = false;
      continue;
    }

    newlineCount = 0;

    if (c == ' ') {
      if (!lastWasSpace) {
        out += ' ';
        lastWasSpace = true;
      }
      continue;
    }

    lastWasSpace = false;
    out += c;
    trimAnchor = out.length();
  }

  if (ioLastWasSpace) *ioLastWasSpace = lastWasSpace;
  if (ioNewlineCount) *ioNewlineCount = newlineCount;

  if (trimTail) {
    while (out.length() > 0) {
      char last = out[out.length() - 1];
      if (last != ' ' && last != '\n') break;
      out.remove(out.length() - 1);
    }
  }

  return out;
}

String compactText(const String& in) {
  return compactText(in, nullptr, nullptr, /*trimTail=*/true);
}
