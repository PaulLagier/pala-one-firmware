#include "src/hal/battery.h"

#include "src/hal/display.h" // u8g2 + gfx for the battery icon
#include "src/ui/font.h"     // role-based font switch (UiSmall for the % text)

#if HAS_BATTERY

// Cached battery reading. The type lives here (file-private) because nothing
// outside this file needs to see the internals — callers use the public
// drawBatteryTopRight() / updateBatteryCached() functions.
struct BatteryState {
  float rawV = 0.0f;
  float filteredV = 0.0f;
  int pctRaw = 0;
  int pctShown = 0;
  bool valid = false;
  bool low = false;
  uint32_t lastMs = 0;
  float calibrationFactor = 1.00f;
};
static BatteryState s_battery;

void adcSetupOnce() {
  pinMode(BAT_ADC_IN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC_IN, ADC_11db);
}

static int cmpUint16(const void* a, const void* b) {
  uint16_t aa = *reinterpret_cast<const uint16_t*>(a);
  uint16_t bb = *reinterpret_cast<const uint16_t*>(b);
  if (aa < bb) return -1;
  if (aa > bb) return 1;
  return 0;
}

static inline float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

static uint32_t readAdcMilliVoltsStable() {
  pinMode(BAT_ADC_CTRL, OUTPUT);
  digitalWrite(BAT_ADC_CTRL, LOW);
  delay(12);

  (void)analogReadMilliVolts(BAT_ADC_IN);
  delay(3);
  (void)analogReadMilliVolts(BAT_ADC_IN);
  delay(3);

  // 11 samples, drop 2 low + 2 high, average 7 — accurate enough, ~20ms faster
  const int N = 11;
  uint16_t vals[N];
  for (int i = 0; i < N; i++) {
    vals[i] = (uint16_t)analogReadMilliVolts(BAT_ADC_IN);
    delay(2);
  }

  pinMode(BAT_ADC_CTRL, INPUT);
  qsort(vals, N, sizeof(vals[0]), cmpUint16);

  uint32_t sum = 0;
  for (int i = 2; i < (N - 2); i++) sum += vals[i];
  return sum / (uint32_t)(N - 4);
}

static float readBatteryVoltageRaw() {
  uint32_t mv = readAdcMilliVoltsStable();
  float v = ((float)mv / 1000.0f) * 2.0f;
  v *= s_battery.calibrationFactor;
  return v;
}

static int batteryPercentFromOCV(float v) {
  struct BatPoint { float v; int pct; };
  static const BatPoint lut[] = {
    {4.20f, 100}, {4.15f, 95}, {4.11f, 90}, {4.08f, 85},
    {4.05f, 80},  {4.02f, 75}, {3.99f, 70}, {3.96f, 62},
    {3.93f, 55},  {3.90f, 48}, {3.87f, 40}, {3.84f, 32},
    {3.81f, 24},  {3.78f, 18}, {3.75f, 13}, {3.72f, 9},
    {3.69f, 6},   {3.65f, 4},  {3.55f, 2},  {3.40f, 0}
  };

  if (v >= lut[0].v) return 100;
  const int n = (int)(sizeof(lut) / sizeof(lut[0]));
  if (v <= lut[n - 1].v) return 0;

  for (int i = 0; i < n - 1; i++) {
    float vHi = lut[i].v;
    float vLo = lut[i + 1].v;
    int pHi = lut[i].pct;
    int pLo = lut[i + 1].pct;
    if (v <= vHi && v >= vLo) {
      float t = (v - vLo) / (vHi - vLo);
      int pct = (int)(pLo + t * (float)(pHi - pLo) + 0.5f);
      if (pct < 0) pct = 0;
      if (pct > 100) pct = 100;
      return pct;
    }
  }
  return 0;
}

void updateBatteryCached(bool force) {
  uint32_t now = millis();
  if (!force && (now - s_battery.lastMs) < BAT_CACHE_MS) return;
  s_battery.lastMs = now;

  float raw = readBatteryVoltageRaw();
  bool valid = (raw > 2.8f && raw < 4.5f);
  s_battery.valid = valid;
  if (!valid) return;

  s_battery.rawV = raw;
  if (s_battery.filteredV <= 0.0f) {
    s_battery.filteredV = raw;
  } else {
    const float alpha = 0.22f;
    s_battery.filteredV = (alpha * raw) + ((1.0f - alpha) * s_battery.filteredV);
  }
  s_battery.filteredV = clampf(s_battery.filteredV, 3.0f, 4.25f);
  s_battery.pctRaw = batteryPercentFromOCV(s_battery.filteredV);

  if (force) {
    s_battery.pctShown = s_battery.pctRaw;
  } else {
    if (s_battery.pctRaw < s_battery.pctShown) {
      s_battery.pctShown--;
    } else if (s_battery.pctRaw > s_battery.pctShown + 2) {
      s_battery.pctShown++;
    }
  }

  if (s_battery.pctShown < 0) s_battery.pctShown = 0;
  if (s_battery.pctShown > 100) s_battery.pctShown = 100;

  if (!s_battery.low && s_battery.pctShown <= 8) s_battery.low = true;
  else if (s_battery.low && s_battery.pctShown >= 12) s_battery.low = false;
}

void drawBatteryTopRight() {
  updateBatteryCached(false);

  int pct = s_battery.valid ? s_battery.pctShown : 0;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  const int iconW = 18;
  const int iconH = 9;
  int xIcon = SCREEN_W - MARGIN_X - iconW - 2;
  int yIcon = 2;

  gfx.drawRect(xIcon, yIcon, iconW, iconH, 1);
  gfx.fillRect(xIcon + iconW, yIcon + 2, 2, iconH - 4, 1);

  int innerW = iconW - 2;
  int fillW = (innerW * pct) / 100;
  if (fillW > 0) gfx.fillRect(xIcon + 1, yIcon + 1, fillW, iconH - 2, 1);
  if (s_battery.low && pct > 0) gfx.drawLine(xIcon + 3, yIcon + 2, xIcon + 3, yIcon + iconH - 3, 0);

  Font::useUiSmall();
  char buf[8];
  if (s_battery.valid) snprintf(buf, sizeof(buf), "%d%%", pct);
  else snprintf(buf, sizeof(buf), "--");
  int wTxt = u8g2.getUTF8Width(buf);
  u8g2.setCursor(xIcon - 4 - wTxt, yIcon + 8);
  u8g2.print(buf);
  Font::useBody();
}

#endif
