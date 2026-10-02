#ifndef PALA_STORAGE_CLICK_TIMINGS_H
#define PALA_STORAGE_CLICK_TIMINGS_H

#include <stdint.h>

#include "src/config.h"
#include "src/storage/kv_store.h"

namespace ClickTimings {

// ---------------------------------------------------------------------------
//  Defaults and bounds.
//
//  These are what the UI resets to and the range it will accept. They are no
//  longer the values the classifier reads — input.cpp and hold_gesture.cpp go
//  through the accessors below, which return the live, user-tuned values.
// ---------------------------------------------------------------------------

// Max silence after the most recent release before we commit a click sequence.
// The press-in-progress gate in input.cpp's trailing-silence check means this
// effectively bounds "dead time between release and the next press" — once a
// press starts, the commit pauses until that release lands. So the user can
// take this long to *start* their next click; the press itself can take as
// long as it wants (up to the long-press threshold).
static constexpr uint32_t kDefaultGapMs = 175;

// Max total duration of a multi-click sequence, measured from the first release.
// Conceptually: "the whole multi-click input has to complete within this window."
// Combined with kDefaultGapMs this caps both per-gap and overall sloppiness.
static constexpr uint32_t kDefaultSequenceMs = 550;

static constexpr uint32_t kDefaultLongMs = 850;

// Hold this long (without a preceding click) and the classifier emits
// VeryLong instead of Long. Long and VeryLong — plus the click-then-hold
// chord — are independently bindable to reader actions (bookmark / lock /
// menu / none) via the web settings UI.
static constexpr uint32_t kDefaultVeryLongMs = 2000;

static constexpr uint32_t kDefaultDebounceMs = 14;

static constexpr uint32_t kGapMin = 75;
static constexpr uint32_t kGapMax = 5000;
static constexpr uint32_t kSequenceMax = 10000;
static constexpr uint32_t kLongMin = 50;
static constexpr uint32_t kLongMax = 10000;
static constexpr uint32_t kVeryLongMin = kLongMin + 1;
static constexpr uint32_t kVeryLongMax = 20000;
static constexpr uint32_t kDebounceMin = 0;
static constexpr uint32_t kDebounceMax = 100;

struct TimingSettingSpec {
  const char* key;
  uint32_t defaultValue;
  uint32_t (*current)();
  uint32_t (*minValue)();
  uint32_t (*maxValue)();
  void (*reset)();
  void (*set)(uint32_t value);
};

void loadSettings(KeyValueStore& kv);
void saveSettings(KeyValueStore& kv);
void loadSettings();
void saveSettings();

const TimingSettingSpec* timingSettings();
uint8_t timingSettingsCount();

uint32_t maxClickGapMs();
uint32_t maxClickSequenceMs();
uint32_t longMs();
uint32_t veryLongMs();
uint32_t debounceMs();

void setMaxClickGapMs(uint32_t value);
void setMaxClickSequenceMs(uint32_t value);
void setLongMs(uint32_t value);
void setVeryLongMs(uint32_t value);
void setDebounceMs(uint32_t value);

void resetMaxClickGapMs();
void resetMaxClickSequenceMs();
void resetLongMs();
void resetVeryLongMs();
void resetDebounceMs();
void resetToDefaults();

}  // namespace ClickTimings

#endif  // PALA_STORAGE_CLICK_TIMINGS_H
