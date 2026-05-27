#include "src/ui/statusbar.h"

#include "src/config.h"     // STATUS_H
#include "src/state.h"      // prefs
#include "src/ui/font.h"    // Font::invalidateLayoutCache
#include "src/ui/reader.h"  // repaginateForLayoutChange

namespace Statusbar {

static constexpr const char* kKey = "cfg_statusbar";

static Mode s_mode = Full;

static Mode clamp(int v) {
  if (v < Full || v > Hidden) return Full;
  return (Mode)v;
}

void loadSettings() {
  s_mode = clamp(prefs.getInt(kKey, Full));
}

Mode mode() { return s_mode; }

int reserveH() {
  switch (s_mode) {
    case Hidden:  return 0;
    case Minimal: return 1;
    case Full:
    default:      return STATUS_H;
  }
}

void setMode(Mode m) {
  Mode nm = clamp(m);
  if (nm == s_mode) return;
  s_mode = nm;
  prefs.putInt(kKey, (int)s_mode);
  // Layout's `maxLines` is computed from `SCREEN_H - reserveH()`, so a mode
  // change shifts pagination. Invalidate the font's layout-metrics cache
  // and the reader's in-memory page-offset table together — they would
  // otherwise hold offsets computed under the old reserve.
  Font::invalidateLayoutCache();
  repaginateForLayoutChange();
}

void cycleMode() {
  Mode next = (Mode)((int)s_mode + 1);
  if (next > Hidden) next = Full;
  setMode(next);
}

}  // namespace Statusbar
