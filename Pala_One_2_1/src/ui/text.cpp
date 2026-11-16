#include <limits.h>
#include "src/ui/text.h"
#include "src/pure/page_text_buffer.h"
#include "src/hal/cpu_burst.h"

#include "src/hal/display.h"            // u8g2
#include "src/pure/bookmarks_codec.h"   // kOffsetUnset
#include "src/storage/page_cache.h"     // on-disk page-offset cache
#include "src/ui/font.h"                // Font::useBody / bodyLayout / measureBionicLine / layoutForCache

namespace {
struct PrefetchedPage {
  PageTextBuffer text;
  String path;
  size_t fileSize = 0;
  uint32_t start = 0, next = 0;
  PageCacheLayout layout{};
  LayoutMetrics metrics{};
  bool ready = false;
};
PrefetchedPage s_prefetch;
#if PALA_DIAGNOSTICS
bool s_lastDrawUsedPrefetch = false;
#endif
bool sameLayout(const PageCacheLayout& a, const PageCacheLayout& b) {
  return a.bodySize == b.bodySize && a.lineGap == b.lineGap
      && a.family == b.family && a.bionic == b.bionic
      && a.statusbarReserve == b.statusbarReserve && a.halfGaps == b.halfGaps;
}
bool matchesPrefetch(File& f, uint32_t start) {
  return s_prefetch.ready && s_prefetch.start == start
      && s_prefetch.fileSize == bookLogicalSize(f) && s_prefetch.path == String(f.path())
      && sameLayout(s_prefetch.layout, Font::layoutForCache());
}
} // namespace
void clearPagePrefetch() { s_prefetch.ready = false; }
#if PALA_DIAGNOSTICS
bool lastDrawUsedPrefetch() { return s_lastDrawUsedPrefetch; }
#endif

// Measure-width adapter for the paginator. Routes through Font::measureBionicLine
// so the bionic 1-px-per-split-word adjustment is folded into the same width
// budget the paginator wraps against — without bionic this collapses to a
// plain getUTF8Width under the Body face.
static int bodyMeasure(const char* s) {
  return Font::measureBionicLine(s);
}

namespace {
uint32_t layoutBodyPage(IReadStream& stream,uint32_t start,LayoutMetrics& m,PageTextBuffer* buffer) {
  m=Font::bodyLayout();
  // Bounds only grow, within the fixed font BBX. Usually one pass (ASCII),
  // two for accents. Retry until stable to cover half-paragraph rounding too.
  for(;;){
    const LayoutMetrics candidateMetrics=m;const int ascent=m.ascent,descent=m.descent;
    if(buffer)buffer->clear();
    const uint32_t next=paginatePage(stream,start,candidateMetrics,bodyMeasure,[&](const char* line,size_t len){
      Font::includeTextBounds(m,line,len);
      if(buffer)buffer->append(line,len);
    });
    if(m.ascent==ascent&&m.descent==descent)return next;
    Font::completePageLayout(m);
  }

}
void drawPreparedPage(const LayoutMetrics& m) {
  int y=TOP_PAD+m.ascent;
  for(size_t i=0;i<s_prefetch.text.count;++i){
    const char* line=s_prefetch.text.line(i);Font::drawBionicLine(MARGIN_X,y,line);
    y+=*line?m.lineH:m.paragraphGapH;
  }
}
}
uint32_t drawPageAt(File& f,uint32_t startPos) {
  const bool hit=matchesPrefetch(f,startPos);
#if PALA_DIAGNOSTICS
  s_lastDrawUsedPrefetch=hit;
#endif
  if(!hit)prefetchPage(f,startPos);
  if(s_prefetch.ready){drawPreparedPage(s_prefetch.metrics);return s_prefetch.next;}
  // A pathological page can overflow the bounded staging buffer. Re-render
  // through the same safe layout, never truncate the book to fit that buffer.
  FileReadStream stream(f);LayoutMetrics m;
  layoutBodyPage(stream,startPos,m,nullptr);int y=TOP_PAD+m.ascent;
  return paginatePage(stream,startPos,m,bodyMeasure,[&](const char* line,size_t len){
    Font::drawBionicLine(MARGIN_X,y,line);y+=len?m.lineH:m.paragraphGapH;
  });
}
void prefetchPage(File& f,uint32_t startPos) {
  if(matchesPrefetch(f,startPos))return;
  s_prefetch.ready=false;if(startPos>=bookLogicalSize(f))return;
  CpuBurst cpu;FileReadStream stream(f);
  s_prefetch.next=layoutBodyPage(stream,startPos,s_prefetch.metrics,&s_prefetch.text);
  s_prefetch.path=f.path();s_prefetch.fileSize=bookLogicalSize(f);s_prefetch.start=startPos;
  s_prefetch.layout=Font::layoutForCache();s_prefetch.ready=s_prefetch.text.valid;
}
uint32_t extractPageText(File& f,uint32_t startPos,String& out) {
  FileReadStream stream(f);LayoutMetrics m;layoutBodyPage(stream,startPos,m,nullptr);
  return paginatePage(stream,startPos,m,bodyMeasure,[&](const char* line,size_t len){
    while(len&&(*line==' '||*line=='\t')){++line;--len;}out.concat(line,len);out.concat('\n');
  });
}
uint32_t nextPageOffset(File& f,uint32_t startPos){FileReadStream stream(f);return nextPageOffset(stream,startPos);}
uint32_t nextPageOffset(IReadStream& stream,uint32_t startPos){LayoutMetrics m;return layoutBodyPage(stream,startPos,m,nullptr);}

// ============================================================================
//  Cross-book offset lookup
// ============================================================================
uint32_t pageOffsetForPage(File& f, const String& path, int page,
                           PageLookupProgress progress, void* context) {
  if (page < 0) page = 0;

  // On-disk fast path: O(1) seek to the highest cached page <= target.
  // The active reader keeps this file fresh for books it visits; cross-book
  // lookups (web bookmark resolve, page-text export) ride along.
  uint32_t off = 0;
  int startPage = loadOffsetForPageFromDisk(path, bookLogicalSize(f),
                                            Font::layoutForCache(),
                                            page, &off);
  if (startPage < 0) startPage = 0;
  if (startPage >= page) return off;

  CpuBurst cpu;
  FileReadStream stream(f);
  for (int p = startPage; p < page; p++) {
    if (progress && !progress(uint32_t(p),uint32_t(page),context)) return kOffsetUnset;
    uint32_t next = nextPageOffset(stream, off);
    if (next == off) break;
    off = next;
    if((p&31)==0)delay(1);
  }
  return off;
}

uint32_t resolveBookmarkOffset(const String& path, uint32_t page, uint32_t storedOffset,
                                 PageLookupProgress progress, void* context) {
  File f = FS.open(path, "r");
  if (!f) return 0;

  size_t size = bookLogicalSize(f);
  if (storedOffset != kOffsetUnset && storedOffset < size) {
    f.close();
    return storedOffset;
  }

  uint32_t off = pageOffsetForPage(f,path,page>uint32_t(INT_MAX)?INT_MAX:int(page),progress,context);
  f.close();
  return off;
}
