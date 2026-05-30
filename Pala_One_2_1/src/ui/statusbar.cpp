#include "src/ui/statusbar.h"

#include "src/config.h"     // STATUS_H
#include "src/state.h"      // prefs
#include "src/ui/font.h"    // Font::invalidateLayoutCache
#include "src/ui/reader.h"  // markPagesDirtyForLayoutChange

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
  // (cheap — recomputed on the next bodyLayout()) and mark the reader's
  // in-memory page-offset table as stale so the next reader render rebases
  // it. Deferring (rather than rebasing here) keeps `setMode` a plain setting
  // write and collapses rapid cycles into one rebase on the next render — see
  // markPagesDirtyForLayoutChange / repaginateForLayoutChange in reader.cpp.
  Font::invalidateLayoutCache();
  markPagesDirtyForLayoutChange();
}

void cycleMode() {
  Mode next = (Mode)((int)s_mode + 1);
  if (next > Hidden) next = Full;
  setMode(next);
}

}  // namespace Statusbar
