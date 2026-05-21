#include "src/ui/screensavers.h"

#include <esp_random.h>

#include "src/config.h"
#include "src/hal/display.h"   // gfx
#include "src/state.h"         // FS, prefs

namespace Screensavers {

// File-private state. Cached at loadSettings(); mutators below keep both
// the in-memory copy and the NVS value in sync.
static Mode    s_mode      = Mode::Single;
static int     s_cycleIdx  = 0;     // next slot index to use in Cycle order
static int     s_lastShown = -1;    // last slot shown (Shuffle anti-repeat)

// NVS keys — file-private; nothing outside this file should reference them.
static constexpr const char* kKeyMode      = "cfg_ss_mode";
static constexpr const char* kKeyCycleIdx  = "cfg_ss_idx";
static constexpr const char* kKeyLastShown = "cfg_ss_last";

static constexpr const char* kDir = "/screensavers";

static void ensureDir() {
  if (!FS.exists(kDir)) FS.mkdir(kDir);
}

String slotPath(int slot) {
  char buf[28];
  snprintf(buf, sizeof(buf), "%s/%d.bin", kDir, slot);
  return String(buf);
}

bool slotExists(int slot) {
  if (slot < 0 || slot >= MAX_SLOTS) return false;
  File f = FS.open(slotPath(slot), "r");
  if (!f) return false;
  bool ok = (f.size() == (size_t)SCREENSAVER_BYTES);
  f.close();
  return ok;
}

int populatedCount() {
  int n = 0;
  for (int i = 0; i < MAX_SLOTS; i++) if (slotExists(i)) n++;
  return n;
}

int firstFreeSlot() {
  for (int i = 0; i < MAX_SLOTS; i++) if (!slotExists(i)) return i;
  return -1;
}

bool readSlot(int slot, uint8_t out[SCREENSAVER_BYTES]) {
  if (slot < 0 || slot >= MAX_SLOTS) return false;
  File f = FS.open(slotPath(slot), "r");
  if (!f) return false;
  if (f.size() != (size_t)SCREENSAVER_BYTES) { f.close(); return false; }
  size_t got = f.read(out, SCREENSAVER_BYTES);
  f.close();
  return got == (size_t)SCREENSAVER_BYTES;
}

// Build a compact array of populated slot indices in ascending order.
// Caller-allocated; returns the count.
static int collectSlots(int outSlots[MAX_SLOTS]) {
  int n = 0;
  for (int i = 0; i < MAX_SLOTS; i++) {
    if (slotExists(i)) outSlots[n++] = i;
  }
  return n;
}

void loadSettings() {
  ensureDir();
  int m = prefs.getInt(kKeyMode, (int)Mode::Single);
  if (m < 0 || m > 2) m = (int)Mode::Single;
  s_mode = (Mode)m;
  s_cycleIdx  = prefs.getInt(kKeyCycleIdx, 0);
  if (s_cycleIdx < 0) s_cycleIdx = 0;
  s_lastShown = prefs.getInt(kKeyLastShown, -1);
}

void setMode(Mode m) {
  s_mode = m;
  prefs.putInt(kKeyMode, (int)m);
  // Reset the cycle pointer so a fresh mode starts from slot 0.
  s_cycleIdx = 0;
  prefs.putInt(kKeyCycleIdx, 0);
}

Mode currentMode() { return s_mode; }

bool installFromTemp(int slot, const String& tmpPath) {
  if (slot < 0 || slot >= MAX_SLOTS) {
    if (FS.exists(tmpPath)) FS.remove(tmpPath);
    return false;
  }
  ensureDir();
  String dst = slotPath(slot);
  if (FS.exists(dst)) FS.remove(dst);
  if (!FS.rename(tmpPath, dst)) {
    if (FS.exists(tmpPath)) FS.remove(tmpPath);
    return false;
  }
  // New image enters the rotation on the next sleep. Resetting the cycle
  // pointer ensures the newly-populated slot is reached predictably, and
  // clearing `last` lets Shuffle pick it.
  s_cycleIdx  = 0;
  s_lastShown = -1;
  prefs.putInt(kKeyCycleIdx, 0);
  prefs.putInt(kKeyLastShown, -1);
  return true;
}

void deleteSlot(int slot) {
  if (slot < 0 || slot >= MAX_SLOTS) return;
  String p = slotPath(slot);
  if (FS.exists(p)) FS.remove(p);
  s_cycleIdx  = 0;
  s_lastShown = -1;
  prefs.putInt(kKeyCycleIdx, 0);
  prefs.putInt(kKeyLastShown, -1);
}

bool drawNext() {
  if (s_mode == Mode::Single) return false;

  int slots[MAX_SLOTS];
  int count = collectSlots(slots);
  if (count == 0) return false;

  int pick = slots[0];
  if (s_mode == Mode::Shuffle && count > 1) {
    // Bounded retry to avoid an immediate repeat; if the RNG keeps landing
    // on the same slot (very rare with count >= 2), the guard prevents an
    // infinite loop and accepts the repeat after enough tries.
    int guard = 12;
    do {
      pick = slots[(int)(esp_random() % (uint32_t)count)];
      guard--;
    } while (pick == s_lastShown && guard > 0);
  } else if (s_mode == Mode::Shuffle) {
    pick = slots[0];  // count == 1, no choice
  } else {
    // Cycle: walk the populated set in order, persisted across deep sleeps
    // via the cycle-idx pref so the next wake picks up where we left off.
    int idx = s_cycleIdx;
    if (idx < 0 || idx >= count) idx = 0;
    pick = slots[idx];
    int nextIdx = (idx + 1) % count;
    s_cycleIdx = nextIdx;
    prefs.putInt(kKeyCycleIdx, nextIdx);
  }

  static uint8_t buf[SCREENSAVER_BYTES];
  if (!readSlot(pick, buf)) return false;

  s_lastShown = pick;
  prefs.putInt(kKeyLastShown, pick);

  gfx.fillScreen(1);
  gfx.drawXBitmap(0, 0, buf, SCREEN_W, SCREEN_H, 0);
  return true;
}

}  // namespace Screensavers
