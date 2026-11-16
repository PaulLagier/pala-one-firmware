#pragma once
#include "src/ui/font.h"
#include "src/pure/page_text_buffer.h"

// Use compact ordinary-line bounds; grow only for glyphs on this page. Bounds
// grow monotonically within the bundled font BBX, so the fixed point is finite.
// Drawing, extraction and offset scans must all use this same geometry.
inline uint32_t prepareBodyPage(IReadStream &stream, uint32_t start, LayoutMetrics &metrics,
                                PageTextBuffer *buffer = nullptr) {
  metrics = Font::bodyLayout();
  for (;;) {
    const LayoutMetrics candidate = metrics;
    if (buffer)
      buffer->clear();
    const uint32_t next = paginatePage(stream, start, candidate, Font::measureBionicLine,
                                       [&](const char *line, size_t length) {
                                         Font::includeTextBounds(metrics, line, length);
                                         if (buffer)
                                           buffer->append(line, length);
                                       });
    if (metrics.ascent == candidate.ascent && metrics.descent == candidate.descent)
      return next;
    Font::completePageLayout(metrics);
  }
}
