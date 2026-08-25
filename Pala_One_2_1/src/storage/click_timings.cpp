#include "src/storage/click_timings.h"

#ifdef ARDUINO
#include "src/state.h"
#include "src/storage/preferences_store.h"
#endif

namespace ClickTimings {

static constexpr const char* kKeyGap = "cfg_cGap";
static constexpr const char* kKeySequence = "cfg_cSeq";
static constexpr const char* kKeyLong = "cfg_cLong";
static constexpr const char* kKeyVeryLong = "cfg_cVLng";
static constexpr const char* kKeyDebounce = "cfg_cDbnc";



static uint32_t s_gapMs = kDefaultGapMs;
static uint32_t s_sequenceMs = kDefaultSequenceMs;
static uint32_t s_longMs = kDefaultLongMs;
static uint32_t s_veryLongMs = kDefaultVeryLongMs;
static uint32_t s_debounceMs = kDefaultDebounceMs;

enum class MinKind : uint8_t {
  Fixed,
  Gap,
  LongPlusOne,
};

struct TimingSetting {
  const char* key;
  uint32_t defaultValue;
  uint32_t minValue;
  uint32_t maxValue;
  uint32_t* value;
  MinKind minKind;
};

static TimingSetting& gapSetting() {
  static TimingSetting setting = {kKeyGap, kDefaultGapMs, kGapMin, kGapMax, &s_gapMs, MinKind::Fixed};
  return setting;
}

static TimingSetting& sequenceSetting() {
  static TimingSetting setting = {kKeySequence, kDefaultSequenceMs, kGapMin, kSequenceMax, &s_sequenceMs, MinKind::Gap};
  return setting;
}

static TimingSetting& longSetting() {
  static TimingSetting setting = {kKeyLong, kDefaultLongMs, kLongMin, kLongMax, &s_longMs, MinKind::Fixed};
  return setting;
}

static TimingSetting& veryLongSetting() {
  static TimingSetting setting = {kKeyVeryLong, kDefaultVeryLongMs, kVeryLongMin, kVeryLongMax, &s_veryLongMs, MinKind::LongPlusOne};
  return setting;
}

static TimingSetting& debounceSetting() {
  static TimingSetting setting = {kKeyDebounce, kDefaultDebounceMs, kDebounceMin, kDebounceMax, &s_debounceMs, MinKind::Fixed};
  return setting;
}

static TimingSetting* const kAllSettings[] = {
  &gapSetting(),
  &sequenceSetting(),
  &longSetting(),
  &veryLongSetting(),
  &debounceSetting(),
};

static uint32_t clampU32(uint32_t value, uint32_t minValue, uint32_t maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

static uint32_t minValueFor(const TimingSetting& setting) {
  switch (setting.minKind) {
    case MinKind::Gap: return s_gapMs;
    case MinKind::LongPlusOne: return s_longMs + 1;
    case MinKind::Fixed:
    default: return setting.minValue;
  }
}

static void clampSetting(const TimingSetting* setting) {
  *setting->value = clampU32(*setting->value, minValueFor(*setting), setting->maxValue);
}

static void loadSetting(KeyValueStore& kv, const TimingSetting* setting) {
  *setting->value = (uint32_t)kv.getInt(setting->key, (int)setting->defaultValue);
}

static void saveSetting(KeyValueStore& kv, const TimingSetting* setting) {
  kv.putInt(setting->key, (int)*setting->value);
}

static void normalize() {
  for (const TimingSetting* setting : kAllSettings) {
    clampSetting(setting);
  }
}

static void loadFromStore(KeyValueStore& kv) {
  for (const TimingSetting* setting : kAllSettings) {
    loadSetting(kv, setting);
  }
  normalize();
}

static void saveToStore(KeyValueStore& kv) {
  normalize();
  for (const TimingSetting* setting : kAllSettings) {
    saveSetting(kv, setting);
  }
}

static void setSetting(const TimingSetting& setting, uint32_t value) {
  *setting.value = clampU32(value, minValueFor(setting), setting.maxValue);
  normalize();
}

static void resetSetting(const TimingSetting& setting) {
  *setting.value = setting.defaultValue;
  normalize();
}

static uint32_t gapMinMs() { return gapSetting().minValue; }
static uint32_t gapMaxMs() { return gapSetting().maxValue; }
static uint32_t sequenceMinMs() { return minValueFor(sequenceSetting()); }
static uint32_t sequenceMaxMs() { return sequenceSetting().maxValue; }
static uint32_t longMinMs() { return longSetting().minValue; }
static uint32_t longMaxMs() { return longSetting().maxValue; }
static uint32_t veryLongMinMs() { return minValueFor(veryLongSetting()); }
static uint32_t veryLongMaxMs() { return veryLongSetting().maxValue; }
static uint32_t debounceMinMs() { return debounceSetting().minValue; }
static uint32_t debounceMaxMs() { return debounceSetting().maxValue; }

void loadSettings(KeyValueStore& kv) {
  loadFromStore(kv);
}

void saveSettings(KeyValueStore& kv) {
  saveToStore(kv);
}

void loadSettings() {
#ifdef ARDUINO
  PreferencesStore kv(prefs);
  loadSettings(kv);
#endif
}

void saveSettings() {
#ifdef ARDUINO
  PreferencesStore kv(prefs);
  saveSettings(kv);
#endif
}

uint32_t maxClickGapMs() { return s_gapMs; }
uint32_t maxClickSequenceMs() { return s_sequenceMs; }
uint32_t longMs() { return s_longMs; }
uint32_t veryLongMs() { return s_veryLongMs; }
uint32_t debounceMs() { return s_debounceMs; }

void setMaxClickGapMs(uint32_t value) {
  setSetting(gapSetting(), value);
  saveSettings();
}

void setMaxClickSequenceMs(uint32_t value) {
  setSetting(sequenceSetting(), value);
  saveSettings();
}

void setLongMs(uint32_t value) {
  setSetting(longSetting(), value);
  saveSettings();
}

void setVeryLongMs(uint32_t value) {
  setSetting(veryLongSetting(), value);
  saveSettings();
}

void setDebounceMs(uint32_t value) {
  setSetting(debounceSetting(), value);
  saveSettings();
}

void resetMaxClickGapMs() {
  resetSetting(gapSetting());
  saveSettings();
}

void resetMaxClickSequenceMs() {
  resetSetting(sequenceSetting());
  saveSettings();
}

void resetLongMs() {
  resetSetting(longSetting());
  saveSettings();
}

void resetVeryLongMs() {
  resetSetting(veryLongSetting());
  saveSettings();
}

void resetDebounceMs() {
  resetSetting(debounceSetting());
  saveSettings();
}

void resetToDefaults() {
  for (const TimingSetting* setting : kAllSettings) {
    resetSetting(*setting);
  }
  normalize();
  saveSettings();
}

const TimingSettingSpec* timingSettings() {
  static const TimingSettingSpec kTimingSpecs[] = {
    {kKeyGap, kDefaultGapMs, maxClickGapMs, gapMinMs, gapMaxMs, resetMaxClickGapMs, setMaxClickGapMs},
    {kKeySequence, kDefaultSequenceMs, maxClickSequenceMs, sequenceMinMs, sequenceMaxMs, resetMaxClickSequenceMs, setMaxClickSequenceMs},
    {kKeyLong, kDefaultLongMs, longMs, longMinMs, longMaxMs, resetLongMs, setLongMs},
    {kKeyVeryLong, kDefaultVeryLongMs, veryLongMs, veryLongMinMs, veryLongMaxMs, resetVeryLongMs, setVeryLongMs},
    {kKeyDebounce, kDefaultDebounceMs, debounceMs, debounceMinMs, debounceMaxMs, resetDebounceMs, setDebounceMs},
  };
  return kTimingSpecs;
}

uint8_t timingSettingsCount() {
  return (uint8_t)(sizeof(kAllSettings) / sizeof(kAllSettings[0]));
}

}  // namespace ClickTimings
