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
  // Height to be used for a paragraph gap. Either lineH or lineH/2.
  int paragraphGapH = 0;
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

// Pure pagination engine. Reads bytes from `in` starting at `startPos` and
// emits at most `metrics.maxLines` lines via `onLine`. Returns the absolute
// byte offset where the next page begins.
//
// `onLine` may be null (just compute the next offset). `measure` MUST be set.
uint32_t paginatePage(IReadStream& in,
                      uint32_t startPos,
                      const LayoutMetrics& metrics,
                      const MeasureFn& measure,
                      const LineCallback& onLine = nullptr);

#endif  // PALA_PURE_PAGINATOR_H
