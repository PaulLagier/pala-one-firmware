#include "paginator.h"

#include <string.h>

#include "text_util.h"

// Fixed scratch buffers for the paginator's working state. Sized for
// "any line that could possibly fit on the display" + "any reasonable
// word/URL length" with margin. Stays on the call stack — no heap.
static constexpr size_t kLineMax   = 256;
static constexpr size_t kTokenMax  = 512;
static constexpr size_t kScratchMax = kLineMax + kTokenMax + 1;

uint32_t paginatePage(IReadStream& in,
                      uint32_t startPos,
                      const LayoutMetrics& m,
                      const MeasureFn& measure,
                      const LineCallback& onLine) {
  in.seek(startPos);

  int linesUsed = 0;
  char line[kLineMax];      size_t lineLen = 0;
  char token[kTokenMax] = {}; size_t tokLen  = 0;
  char scratch[kScratchMax];

  uint32_t lineStartPos  = startPos;
  uint32_t tokenStartPos = startPos;

  auto trimTrailing = [](const char* buf, size_t& len) {
    while (len > 0 && buf[len - 1] == ' ') len--;
  };

  auto trimLeading = [](char* buf, size_t& len) {
    size_t i = 0;
    while (i < len && buf[i] == ' ') i++;
    if (i > 0) {
      memmove(buf, buf + i, len - i);
      len -= i;
    }
  };

  auto emit = [&](const char* buf, size_t len) {
    linesUsed++;
    if (onLine) onLine(buf, len);
  };

  auto flushLine = [&]() {
    trimTrailing(line, lineLen);
    line[lineLen] = 0;
    emit(line, lineLen);
    lineLen = 0;
  };

  auto safeReturn = [&](uint32_t off) -> uint32_t {
    if (off <= startPos) off = startPos + 1;
    size_t sz = in.size();
    if (sz > 0 && off > sz) off = (uint32_t)sz;
    return off;
  };

  auto lineEndsWithSpace = [&]() -> bool {
    return lineLen > 0 && line[lineLen - 1] == ' ';
  };

  // Hard-break the token buffer: emit prefix-by-prefix lines, each as wide
  // as fits at the current font, until the token is consumed. Updates
  // `tokenStartPos` as bytes leave so safeReturn can compute the correct
  // resume offset on early-out.
  auto hardBreakToken = [&]() -> uint32_t {
    while (tokLen > 0) {
      size_t fitLen = 0;
      while (fitLen < tokLen) {
        int clen = utf8SafeCharLenAt(token, tokLen, fitLen);
        if (clen <= 0) break;
        if (fitLen + (size_t)clen > tokLen) break;
        char saved = token[fitLen + clen];
        token[fitLen + clen] = 0;
        bool fits = measure(token) <= m.maxWidth;
        token[fitLen + clen] = saved;
        if (!fits) break;
        fitLen += (size_t)clen;
      }
      if (fitLen == 0) {
        int clen = utf8SafeCharLenAt(token, tokLen, 0);
        if (clen <= 0) clen = 1;
        if ((size_t)clen > tokLen) clen = (int)tokLen;
        fitLen = (size_t)clen;
      }

      char saved = token[fitLen];
      token[fitLen] = 0;
      emit(token, fitLen);
      token[fitLen] = saved;

      if (linesUsed >= m.maxLines)
        return safeReturn(tokenStartPos + (uint32_t)fitLen);

      memmove(token, token + fitLen, tokLen - fitLen);
      tokLen -= fitLen;
      tokenStartPos += (uint32_t)fitLen;
    }
    return 0;
  };

  // Try to append the current token to the line. Flushes the line first if
  // the combined width would overflow; falls into hardBreakToken if even a
  // standalone token won't fit. Clears `tokLen` on success.
  auto appendTokenToLine = [&]() -> uint32_t {
    if (tokLen == 0) return 0;

    if (lineLen == 0) {
      trimLeading(token, tokLen);
      if (tokLen == 0) return 0;
      token[tokLen] = 0;
      if (measure(token) > m.maxWidth) {
        return hardBreakToken();
      }
      memcpy(line, token, tokLen);
      lineLen = tokLen;
      lineStartPos = tokenStartPos;
      tokLen = 0;
      return 0;
    }

    trimLeading(token, tokLen);
    if (tokLen == 0) return 0;

    memcpy(scratch, line, lineLen);
    memcpy(scratch + lineLen, token, tokLen);
    scratch[lineLen + tokLen] = 0;

    if (measure(scratch) > m.maxWidth) {
      flushLine();
      if (linesUsed >= m.maxLines) return safeReturn(tokenStartPos);

      token[tokLen] = 0;
      if (measure(token) > m.maxWidth) {
        return hardBreakToken();
      }
      memcpy(line, token, tokLen);
      lineLen = tokLen;
      lineStartPos = tokenStartPos;
    } else {
      memcpy(line + lineLen, token, tokLen);
      lineLen += tokLen;
    }
    tokLen = 0;
    return 0;
  };

  while (in.available() && linesUsed < m.maxLines) {
    uint32_t charPos = in.position();
    int rb = in.read();
    if (rb < 0) break;
    char c = (char)rb;
    if (c == '\r') continue;

    if (c == '\n') {
      uint32_t forcedNext = appendTokenToLine();
      if (forcedNext != 0) return forcedNext;
      flushLine();
      if (linesUsed >= m.maxLines) return safeReturn(in.position());
      lineStartPos = in.position();
      continue;
    }

    if (isBreakableWhitespaceByte(c)) {
      uint32_t forcedNext = appendTokenToLine();
      if (forcedNext != 0) return forcedNext;
      if (lineLen > 0 && !lineEndsWithSpace() && lineLen < kLineMax - 1) {
        line[lineLen++] = ' ';
      }
      continue;
    }

    // Defensive: if the token buffer is full (degenerate input — a long
    // run of non-breakable bytes), force a flush before appending. Normal
    // text never reaches this branch.
    if (tokLen >= kTokenMax - 1) {
      uint32_t forcedNext = appendTokenToLine();
      if (forcedNext != 0) return forcedNext;
    }
    if (tokLen == 0) tokenStartPos = charPos;
    token[tokLen++] = c;

    if (isBreakablePunctuationByte(c)) {
      uint32_t forcedNext = appendTokenToLine();
      if (forcedNext != 0) return forcedNext;
    }
  }

  uint32_t forcedNext = appendTokenToLine();
  if (forcedNext != 0) return forcedNext;

  if (linesUsed < m.maxLines && lineLen > 0) {
    flushLine();
  }

  return safeReturn(in.position());
}
