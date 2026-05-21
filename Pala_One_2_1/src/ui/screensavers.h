#ifndef PALA_UI_SCREENSAVERS_H
#define PALA_UI_SCREENSAVERS_H

#include "src/pure/arduino_compat.h"  // uint8_t, size_t

// ============================================================================
//  Multi-screensaver module — owns the rotation of 1-bit XBM images shown
//  on the e-ink before deep sleep.
//
//  Storage:
//    /screensavers/0.bin … /screensavers/7.bin    rotation slots (3904 bytes each)
//    /sleep.bin                                   legacy single image (kept as fallback)
//
//  Each slot file is exactly SCREENSAVER_BYTES (250x122 px, 1-bit, LSB-first,
//  32 bytes per row — same XBitmap format the e-ink driver consumes).
//
//  Mode (NVS key `cfg_ss_mode`):
//    Single   skip the rotation, defer to Sleep.cpp's legacy path
//    Cycle    advance through populated slots in order, persisted across sleeps
//    Shuffle  pick a random populated slot, avoiding immediate repeats
//
//  Sleep.cpp's drawSleepScreen() calls drawNext() first. If that returns
//  false (no slots populated or mode is Single), Sleep.cpp falls back to
//  /sleep.bin or the built-in icon.
// ============================================================================
namespace Screensavers {

constexpr int MAX_SLOTS         = 8;
constexpr int SCREENSAVER_BYTES = 3904;   // 250 * 122 / 8, 1-bit packed

enum class Mode : uint8_t { Single = 0, Cycle = 1, Shuffle = 2 };

// Read persisted mode + rotation pointer + last-shown slot from NVS into
// the module's internal state. Call once from setup() after `prefs.begin`.
void loadSettings();

// Apply + persist the rotation mode.
void  setMode(Mode m);
Mode  currentMode();

// Render the next screensaver according to mode + populated slots. Returns
// true if pixels were emitted (caller should NOT also draw a fallback);
// false if there's nothing to do at this layer.
//
// Caller is expected to have already prepared the canvas / paged the e-ink;
// this function only draws bits into the framebuffer (via gfx).
bool drawNext();

// ---- Editor API: slot enumeration + R/W -------------------------------------

// True if /screensavers/<slot>.bin exists with the expected size.
bool slotExists(int slot);

// Number of slots currently populated (0..MAX_SLOTS).
int populatedCount();

// Index of the first unpopulated slot in [0, MAX_SLOTS), or -1 if all full.
int firstFreeSlot();

// Read slot bytes into `out` (must be SCREENSAVER_BYTES). Returns true iff
// the file existed and was the correct length.
bool readSlot(int slot, uint8_t out[SCREENSAVER_BYTES]);

// Promote a freshly-uploaded temp file to slot `slot`. The temp file is
// expected to be exactly SCREENSAVER_BYTES; on success the rotation
// pointer is reset so the new slot enters the cycle on the next sleep.
// Returns true on success (caller may report it to the user); on failure,
// leaves the existing slot untouched and removes the temp file.
bool installFromTemp(int slot, const String& tmpPath);

// Remove /screensavers/<slot>.bin if present + reset the rotation pointer.
// No-op if the slot isn't populated.
void deleteSlot(int slot);

// Slot's on-disk path. Exposed so the upload handler can stage to a sibling
// `.tmp` then call `installFromTemp`.
String slotPath(int slot);

}  // namespace Screensavers

#endif  // PALA_UI_SCREENSAVERS_H
