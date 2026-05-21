#include "paginator.h"

#include <climits>
#include <string.h>
#include <vector>

#include "text_util.h"
#include "token_reader.h"

// Fixed scratch buffers for the paginator's working state. Sized for
// "any line that could possibly fit on the display" + "any reasonable
// word/URL length" with margin. Stays on the call stack — no heap.
static constexpr size_t kLineMax  = 256;
static constexpr size_t kTokenMax = 512;

// DP paginator memory budget. Vectors are heap-allocated per call so we
// don't blow the 8 KB ESP32 loopTask stack — `kParagraphReserve` sizes
// the initial alloc to cover the common case without a grow, and
// `kParagraphHardCap` bounds the token window we DP at once: paragraphs
// longer than this are processed in cap-sized chunks across consecutive
// pages. Memory at the cap: ~24 KB tokens + 8 KB best + 4 KB breakPoint
// = ~36 KB peak (plenty of room on ESP32's ~200 KB free heap).
static constexpr size_t kParagraphReserve = 256;
static constexpr size_t kParagraphHardCap = 2048;

namespace {

// One-entry line-plan cache. Populated when the DP stops mid-paragraph
// — the only case where a future page within the same paragraph would
// benefit. Keyed by a layout fingerprint (font-size changes self-
// invalidate), by stream size (book switches self-invalidate), and by
// paragraph byte range (reading off the end self-invalidates). No
// external invalidation API needed.
struct LinePlanCache {
  bool valid = false;
  uint32_t fingerprint = 0;
  uint32_t streamSize = 0;
  uint32_t paragraphStart = 0;
  uint32_t paragraphEnd = 0;
  std::vector<ScannedToken> tokens;
  std::vector<uint16_t> breakPoint;
};

LinePlanCache g_planCache;

// Mix maxWidth with three measure() probes that shift when the active
// font face/size changes. We don't probe lineH because the line-break
// plan is independent of line gap — gap only affects vertical layout,
// not where breaks land. So a gap-only change correctly stays a hit.
uint32_t computeLayoutFingerprint(const MeasureFn& measure, int maxWidth) {
  uint32_t f = (uint32_t)maxWidth;
  f = f * 1000003u + (uint32_t)measure(" ");
  f = f * 1000003u + (uint32_t)measure("M");
  f = f * 1000003u + (uint32_t)measure("xgpy");
  return f;
}

// Binary-search tokens (monotonic startPos) for the entry whose
// startPos equals queryPos. The resume offset handed back by
// paginatePage is always one of tokens[i].startPos by construction,
// so a miss here only happens on a stale lookup (e.g. a startPos that
// landed inside a token after the cached paragraph was DP'd from a
// different anchor).
int findTokenAtPos(const std::vector<ScannedToken>& toks, uint32_t pos) {
  int lo = 0, hi = (int)toks.size();
  while (lo < hi) {
    int mid = (lo + hi) >> 1;
    if (toks[mid].startPos < pos) lo = mid + 1;
    else hi = mid;
  }
  if (lo < (int)toks.size() && toks[lo].startPos == pos) return lo;
  return -1;
}

// Split a single token whose rendered width exceeds maxWidth into UTF-8-
// safe chunks that each fit. Pushes one synthetic ScannedToken per chunk
// to `tokens` (plus corresponding entries to the three width vectors),
// so the rest of the DP can treat them as ordinary tokens.
//
// Each non-last chunk gets `end = Punctuation` — that means "no inter-
// word space follows" in the DP's cost calc, which is exactly the right
// semantic between hard-break chunks. The last chunk inherits the parent
// token's `end` so any real terminator (Space / Newline / Eof) survives.
//
// Largest-prefix-that-fits is greedy-optimal in chunk count: width grows
// monotonically with prefix length, so any tighter packing produces the
// same number of lines and only redistributes ragged-edge slack.
void splitOversizedToken(const ScannedToken& parent,
                         const char* tokenBuf,
                         int maxWidth,
                         int spaceLen,
                         const MeasureFn& measure,
                         std::vector<ScannedToken>& tokens,
                         std::vector<int>& lastWidth,
                         std::vector<int>& prefixNonLast) {
  char scratch[kTokenMax + 2];
  memcpy(scratch, tokenBuf, parent.len);
  scratch[parent.len] = 0;

  size_t consumed = 0;
  while (consumed < parent.len) {
    const size_t remaining = parent.len - consumed;

    // Find the largest UTF-8-safe prefix of scratch[consumed..] whose
    // rendered width is <= maxWidth. Same shape as greedy's hardBreakToken.
    size_t fitLen = 0;
    while (fitLen < remaining) {
      int clen = utf8SafeCharLenAt(scratch + consumed, remaining, fitLen);
      if (clen <= 0) break;
      if (fitLen + (size_t)clen > remaining) break;
      char saved = scratch[consumed + fitLen + clen];
      scratch[consumed + fitLen + clen] = 0;
      const bool fits = measure(scratch + consumed) <= maxWidth;
      scratch[consumed + fitLen + clen] = saved;
      if (!fits) break;
      fitLen += (size_t)clen;
    }
    // Degenerate: even a single character doesn't fit. Take one char
    // anyway so we make forward progress; the line will overflow, which
    // is the best we can do on a display narrower than one glyph.
    if (fitLen == 0) {
      int clen = utf8SafeCharLenAt(scratch + consumed, remaining, 0);
      if (clen <= 0) clen = 1;
      if ((size_t)clen > remaining) clen = (int)remaining;
      fitLen = (size_t)clen;
    }

    const bool isLastChunk = (consumed + fitLen) >= parent.len;

    ScannedToken chunk;
    chunk.startPos = parent.startPos + (uint32_t)consumed;
    chunk.len = fitLen;
    chunk.end = isLastChunk ? parent.end : TokenEnd::Punctuation;
    tokens.push_back(chunk);

    // lastWidth: chunk on its own.
    char saved = scratch[consumed + fitLen];
    scratch[consumed + fitLen] = 0;
    lastWidth.push_back(measure(scratch + consumed));

    // Append the chunk's "non-last" width (chunk + ' ' minus spaceLen,
    // same trick as the main scan loop) to the running prefix sum.
    // Matters only for the trailing chunk — the only one that might
    // combine with following text on the same line — but cheap for all.
    scratch[consumed + fitLen] = ' ';
    scratch[consumed + fitLen + 1] = 0;
    const int nonLast = measure(scratch + consumed) - spaceLen;
    prefixNonLast.push_back(prefixNonLast.back() + nonLast);

    scratch[consumed + fitLen] = saved;
    consumed += fitLen;
  }
}

// Emit lines from `tokens[fromIdx ..]` walking `breakPoint`. Mirrors
// the inter-word-space rule from the DP's cost calc: a space lands
// between A and B only when A was Space-terminated AND the line
// already has visible content. Writes the index of the first un-
// emitted token to *outNextIdx; returns linesEmitted.
int emitFromPlan(IReadStream& in,
                 const std::vector<ScannedToken>& tokens,
                 const std::vector<uint16_t>& breakPoint,
                 int fromIdx,
                 int maxLines,
                 const LineCallback& onLine,
                 int* outNextIdx) {
  char line[kLineMax];
  int linesEmitted = 0;
  int i = fromIdx;
  const int tokenCount = (int)tokens.size();
  while (i < tokenCount && linesEmitted < maxLines) {
    int lineEndToken = breakPoint[i];
    size_t lineLen = 0;
    bool hasContent = false;
    for (int j = i; j < lineEndToken; j++) {
      if (hasContent && tokens[j - 1].end == TokenEnd::Space) {
        line[lineLen++] = ' ';
      }
      in.seek(tokens[j].startPos);
      for (size_t k = 0; k < tokens[j].len; k++) {
        int rb = in.read();
        if (rb < 0) break;
        line[lineLen + k] = (char)rb;
      }
      lineLen += tokens[j].len;
      if (tokens[j].len > 0) hasContent = true;
    }
    // Trim trailing inter-word spaces. They appear when the last token on a
    // line is Space-terminated and empty — e.g. "hello   \n" yields ('hello',
    // '', '', '', '\n') and the emit loop drops a space before each empty
    // tail token. Greedy's flushLine had a matching trim.
    while (lineLen > 0 && line[lineLen - 1] == ' ') lineLen--;
    // NUL-terminate so callbacks that rely on strlen (eg. u8g2.print) don't
    // bleed into stale bytes from a previous, longer line.
    if (lineLen < kLineMax) line[lineLen] = 0;
    if (onLine) onLine(line, lineLen);
    linesEmitted++;
    i = lineEndToken;
  }
  if (outNextIdx) *outNextIdx = i;
  return linesEmitted;
}

// Shared tail for both the cache-hit and the freshly-DP'd paths: emit
// lines from `tokens[fromIdx ..]` walking `breakPoint`, and either
// recurse into the next paragraph (if we exhausted this one with line
// slots to spare) or compute the resume offset for the next page.
//
// Writes the index of the first un-emitted token to *outNextIdx so the
// caller can decide whether to cache: `*outNextIdx < tokens.size()`
// holds iff we stopped mid-paragraph without recursing.
uint32_t emitAndAdvance(IReadStream& in,
                        uint32_t startPos,
                        uint32_t streamSize,
                        uint32_t paragraphEnd,
                        const std::vector<ScannedToken>& tokens,
                        const std::vector<uint16_t>& breakPoint,
                        int fromIdx,
                        const LayoutMetrics& m,
                        const MeasureFn& measure,
                        const LineCallback& onLine,
                        int* outNextIdx) {
  int nextIdx = fromIdx;
  const int linesEmitted = emitFromPlan(in, tokens, breakPoint, fromIdx,
                                        m.maxLines, onLine, &nextIdx);
  if (outNextIdx) *outNextIdx = nextIdx;

  // Filled the paragraph with line slots to spare → recurse into the
  // next paragraph at paragraphEnd. The recursion populates the cache
  // with that paragraph if it stops mid-way.
  if (linesEmitted < m.maxLines && paragraphEnd < streamSize) {
    LayoutMetrics nm = m;
    nm.maxLines -= linesEmitted;
    return paginatePage(in, paragraphEnd, nm, measure, onLine);
  }

  // Stopped mid-paragraph (or at the natural end). Resume on the next
  // page at the first un-emitted token's byte position; fall back to
  // paragraphEnd if everything was consumed.
  uint32_t nextPos = (nextIdx < (int)tokens.size())
      ? tokens[nextIdx].startPos
      : paragraphEnd;
  if (nextPos <= startPos) nextPos = startPos + 1;
  // Only clamp to streamSize when there's a stream to clamp into — an
  // empty stream still needs to return >= 1 for forward progress.
  if (streamSize > 0 && nextPos > streamSize) nextPos = streamSize;
  return nextPos;
}

}  // namespace

// Lay out one page of lines starting at `startPos` and return the byte
// offset where the next page begins. Four phases:
//
//   1. Cache check. If the previous page was inside the same paragraph
//      under the same layout, reuse its precomputed line plan.
//   2. Scan. Read tokens forward from startPos to the next paragraph
//      terminator (or kParagraphHardCap tokens, whichever comes first),
//      recording each token's byte range and rendered width. Oversized
//      tokens are split into width-fitting chunks during this pass.
//   3. DP. Backward sweep computing the line-break plan that minimises
//      sum-of-squared-slack across the paragraph.
//   4. Emit + cache. Walk the plan emitting up to maxLines lines via
//      emitAndAdvance. If tokens remain afterwards, cache the plan so
//      the next page in this paragraph short-circuits to step 1.
uint32_t paginatePage(IReadStream& in,
                      uint32_t startPos,
                      const LayoutMetrics& m,
                      const MeasureFn& measure,
                      const LineCallback& onLine) {
  const uint32_t streamSize = (uint32_t)in.size();
  const uint32_t fp = computeLayoutFingerprint(measure, m.maxWidth);

  // -------- 1. Cache check ------------------------------------------------
  if (g_planCache.valid
      && g_planCache.fingerprint == fp
      && g_planCache.streamSize == streamSize
      && startPos >= g_planCache.paragraphStart
      && startPos <  g_planCache.paragraphEnd) {
    const int idx = findTokenAtPos(g_planCache.tokens, startPos);
    if (idx >= 0) {
      return emitAndAdvance(in, startPos, streamSize,
                            g_planCache.paragraphEnd,
                            g_planCache.tokens, g_planCache.breakPoint,
                            idx, m, measure, onLine, nullptr);
    }
    // findTokenAtPos missed — startPos isn't at a token boundary in the
    // cached plan. Fall through to a fresh DP from startPos.
  }

  // -------- 2. Scan + per-token width precompute (fused) ------------------
  // For each token we record two width metrics that let the DP compute
  // any candidate line's pixel width in O(1):
  //
  //   lastWidth[i]    = measure(token_i)
  //       Width contribution when token_i is the LAST token on a line.
  //       u8g2 applies a last-glyph bbox adjustment at the right edge.
  //
  //   nonLastWidth[i] = measure(token_i + ' ') - measure(' ')
  //       Width contribution when token_i is NOT the last on its line.
  //       Trick: ' ' has glyph_width=0 so u8g2's last-glyph adjustment
  //       subtracts the space's advance and adds 0, giving
  //       measure(token + ' ') = sum_of_advances(token) + advance(' ').
  //       Subtracting measure(' ') leaves the pure sum-of-advances.
  //
  // The DP only reads sums of nonLastWidth over ranges, so we skip the
  // per-token vector and roll directly into a prefix sum (prefixNonLast).
  //
  // Both widths are computed DURING the scan while tokenBuf still holds
  // the bytes we just read — an earlier version re-seek'd and re-read
  // each token in a second pass, a measurable cost on ESP32 since every
  // byte read goes through LittleFS.
  in.seek(startPos);
  std::vector<ScannedToken> tokens;
  std::vector<int>          lastWidth;
  std::vector<int>          prefixNonLast;
  tokens.reserve(kParagraphReserve);
  lastWidth.reserve(kParagraphReserve);
  prefixNonLast.reserve(kParagraphReserve + 1);
  prefixNonLast.push_back(0);

  const int spaceLen = measure(" ");

  // kParagraphHardCap bounds the token window we DP at once. Paragraphs
  // longer than this are processed in cap-sized chunks across consecutive
  // pages — the scan exits early, the DP lays out what it has, and the
  // next page resumes at the first un-emitted token.
  uint32_t pos = startPos;
  while (pos < in.size()) {
    if (tokens.size() >= kParagraphHardCap) break;
    ScannedToken t;
    char tokenBuf[kTokenMax + 2];   // +2 for trailing ' ' + NUL during precompute
    if (!readNextToken(in, tokenBuf, kTokenMax, t)) break;
    // tokenBuf is already NUL-terminated at t.len by readNextToken.

    const int tokenWidth = measure(tokenBuf);
    if (tokenWidth > m.maxWidth) {
      // Oversized token (URL / hash / long compound word). Split into
      // UTF-8-safe chunks that each fit; the DP and emit then handle
      // them transparently because each chunk is just another token.
      splitOversizedToken(t, tokenBuf, m.maxWidth, spaceLen, measure,
                          tokens, lastWidth, prefixNonLast);
    } else {
      lastWidth.push_back(tokenWidth);
      // Append the trailing space, measure, subtract spaceLen for the
      // pure sum-of-advances (the token's "non-last" width); roll into
      // the running prefix sum.
      tokenBuf[t.len]     = ' ';
      tokenBuf[t.len + 1] = 0;
      const int nonLast = measure(tokenBuf) - spaceLen;
      prefixNonLast.push_back(prefixNonLast.back() + nonLast);
      tokens.push_back(t);
    }
    pos = in.position();

    // A blank line (two consecutive newlines) signals the paragraph end.
    if (t.end == TokenEnd::Newline && tokens.size() >= 2 &&
        tokens[tokens.size() - 2].end == TokenEnd::Newline) {
      break;
    }
  }

  // -------- 3. DP cost calc -----------------------------------------------
  // best[i] = min total badness for laying out tokens[i..end].
  // breakPoint[i] = index of the first token on the line *after* the one
  // that starts at i, in the optimal solution. uint16_t is enough — j+1
  // is bounded by kParagraphHardCap (2048).
  const size_t tokenCount = tokens.size();
  std::vector<int>      best(tokenCount + 1, INT_MAX);
  std::vector<uint16_t> breakPoint(tokenCount + 1, 0);
  best[0] = 0;
  best[tokenCount] = 0;

  for (int i = (int)tokenCount - 1; i >= 0; i--) {
    best[i] = INT_MAX;
    int spaces = 0;          // inter-word ' ' inserted so far on this line
    bool hasContent = false; // any non-empty token already on this line?
    for (int j = i; j < (int)tokenCount; j++) {
      // Inter-word space lands between A and B only if A was Space-
      // terminated AND the line already has visible content — same rule
      // the emit loop uses to keep the two consistent.
      if (hasContent && tokens[j - 1].end == TokenEnd::Space) {
        spaces++;
      }

      const int lineWidth =
          (prefixNonLast[j] - prefixNonLast[i])
          + lastWidth[j]
          + spaces * spaceLen;
      if (lineWidth > m.maxWidth) break;
      if (tokens[j].len > 0) hasContent = true;

      const int remainingSpace = m.maxWidth - lineWidth;
      const bool isLastLine =
          (j == (int)tokenCount - 1) || (tokens[j].end == TokenEnd::Newline);
      const int badness = squaredSlackBadness(remainingSpace, isLastLine);
      if (best[i] > badness + best[j + 1]) {
        best[i] = badness + best[j + 1];
        breakPoint[i] = (uint16_t)(j + 1);
      }
    }
  }

  // Defensive: every token has been pre-chunked to fit within maxWidth
  // (splitOversizedToken during the scan), so the DP should always find
  // a valid first line. The only way breakPoint[0] stays 0 is if a single
  // glyph somehow exceeds maxWidth — impossible on any sane display. If
  // that does happen, just nudge forward by one byte so the caller
  // doesn't spin.
  if (tokenCount > 0 && breakPoint[0] == 0) {
    return startPos + 1;
  }

  // -------- 4. Emit + cache -----------------------------------------------
  int nextIdx = 0;
  const uint32_t nextPos = emitAndAdvance(in, startPos, streamSize, pos,
                                          tokens, breakPoint, 0,
                                          m, measure, onLine, &nextIdx);

  // Cache the plan only when tokens are left over — that's the case
  // where the next page will resume within this same paragraph. When
  // emitAndAdvance recursed into the next paragraph, that recursion
  // already populated the cache with its own paragraph if it stopped
  // mid-way; nothing to do here.
  if (nextIdx < (int)tokenCount) {
    g_planCache.valid          = true;
    g_planCache.fingerprint    = fp;
    g_planCache.streamSize     = streamSize;
    g_planCache.paragraphStart = startPos;
    g_planCache.paragraphEnd   = pos;
    g_planCache.tokens         = std::move(tokens);
    g_planCache.breakPoint     = std::move(breakPoint);
  }

  return nextPos;
}

