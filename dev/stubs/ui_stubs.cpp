// Stubs for the Font:: and Sleep:: namespaces from src/ui/font.h and src/ui/sleep.h.
// The emulator returns sensible defaults and persists changes in memory only.

#include "src/ui/font.h"
#include "src/ui/sleep.h"
#include "src/pure/paginator.h"

// ---------------------------------------------------------------------------
//  Font
// ---------------------------------------------------------------------------
namespace Font {

static int  s_bodySize = 12;
static int  s_lineGap  = 0;

void useBody()     {}
void useBold()     {}
void useUiSmall()  {}
void useUiTiny()   {}
void useAppLarge() {}
void loadSettings() {}

const LayoutMetrics& bodyLayout() {
  static LayoutMetrics m;
  m.lineH    = s_bodySize + 2 + s_lineGap;
  m.maxWidth = 250;
  m.maxLines = 122 / m.lineH;
  m.ascent   = s_bodySize;
  m.descent  = 2;
  return m;
}

void setBodySize(int sz) {
  if (sz == 8 || sz == 10 || sz == 12 || sz == 14) s_bodySize = sz;
}
void setLineGap(int gap) {
  if (gap >= 0 && gap <= 4) s_lineGap = gap;
}
int currentBodySize() { return s_bodySize; }
int currentLineGap()  { return s_lineGap; }

}  // namespace Font

// ---------------------------------------------------------------------------
//  Sleep
// ---------------------------------------------------------------------------
namespace Sleep {

static int s_timeout = 120;

void loadSettings()       {}
void setIdleTimeout(int s) { if (s >= 10 && s <= 3600) s_timeout = s; }
int  idleTimeoutSecs()    { return s_timeout; }
uint32_t idleTimeoutMs()  { return (uint32_t)s_timeout * 1000u; }
void enter()              {}
void idleLightSleep(bool) {}

}  // namespace Sleep
