#ifndef PALA_PURE_PAGINATOR_H
#define PALA_PURE_PAGINATOR_H

#include <functional>

#include "arduino_compat.h"
#include "stream.h"

// Layout dimensions for the paginator. All values are in pixels.
struct LayoutMetrics {
  int ascent = 0;
  int descent = 0;
  int lineH = 0;
  int maxWidth = 0;
  int maxLines = 1;
};

// Measure the rendered width (in pixels) of a UTF-8 string under the layout's
// current font. The paginator never sets fonts itself — callers must ensure
// the measurement function is consistent with the metrics they pass in.
using MeasureFn = std::function<int(const char*)>;

// Called once per emitted line, in order. `buf` is NUL-terminated and
// trailing-spaces-trimmed; `len` is its byte length (excluding NUL). The
// buffer is owned by the paginator and only valid for the duration of the
// call — copy if you need to keep it.
using LineCallback = std::function<void(const char* buf, size_t len)>;

// Score for how "bad" a laid-out line is. Smaller is better; 0 = perfect.
// `slack` is `maxWidth - actualLineWidth` in pixels (non-negative; an over-
// flowing line is illegal upstream of this fn). `isLastLine` is true for the
// final line of a paragraph — the optimizer should usually give it a free
// pass, since it's normal for the last line to be short.
//
// Not consumed by the current (greedy) strategy. Reserved for the future
// DP strategy and for harness/diagnostics that want a single comparable
// number across line-breaking algorithms.
using BadnessFn = std::function<int(int slack, bool isLastLine)>;

// Default badness: squared slack, free pass on the last line.
// Penalises one really short line much more than several mildly short ones.
inline int squaredSlackBadness(int slack, bool isLastLine) {
  if (isLastLine) return 0;
  if (slack < 0) return 0;          // shouldn't happen, but defend
  return slack * slack;
}

// Pure pagination engine. Reads bytes from `in` starting at `startPos` and
// emits at most `metrics.maxLines` lines via `onLine`. Returns the absolute
// byte offset where the next page begins.
//
// Implementation: paragraph-level dynamic programming that minimises the
// sum of squared slack across each paragraph's lines (Knuth–Plass-flavoured
// total-fit). Oversized tokens are pre-split into width-fitting chunks
// during the scan; overlong paragraphs are processed in cap-sized token
// windows across consecutive pages. A one-entry line-plan cache keyed on a
// layout fingerprint lets subsequent pages within the same paragraph skip
// the scan and DP entirely.
//
// `onLine` may be null (just compute the next offset). `measure` MUST be set.
uint32_t paginatePage(IReadStream& in,
                      uint32_t startPos,
                      const LayoutMetrics& metrics,
                      const MeasureFn& measure,
                      const LineCallback& onLine = nullptr);

#endif  // PALA_PURE_PAGINATOR_H
