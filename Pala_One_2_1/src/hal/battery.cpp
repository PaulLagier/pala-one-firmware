#include "src/hal/battery.h"

#include "src/hal/display.h" // u8g2 + gfx for the battery icon
#include "src/ui/font.h"     // role-based font switch (UiSmall for the % text)

#if HAS_BATTERY

// Cached battery reading. The type lives here (file-private) because nothing
// outside this file needs to see the internals — callers use the public
// drawBatteryTopRight() / updateBatteryCached() functions.
struct BatteryState
{
  float rawV = 0.0f;
  float filteredV = 0.0f;
  int pctRaw = 0;
  int pctShown = 0;
  bool valid = false;
  bool low = false;
  bool charging = false;
  bool chargingChanged = false;
  uint32_t lastMs = 0;
  uint32_t lastChargingCheckMs = 0;
  float calibrationFactor = 1.00f;
};
static BatteryState s_battery;

void adcSetupOnce()
{
  pinMode(BAT_ADC_IN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC_IN, ADC_11db);
}

static int cmpUint16(const void *a, const void *b)
{
  uint16_t aa = *reinterpret_cast<const uint16_t *>(a);
  uint16_t bb = *reinterpret_cast<const uint16_t *>(b);
  if (aa < bb)
    return -1;
  if (aa > bb)
    return 1;
  return 0;
}

static inline float clampf(float x, float lo, float hi)
{
  if (x < lo)
    return lo;
  if (x > hi)
    return hi;
  return x;
}

static uint32_t readAdcMilliVoltsStable()
{
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
  for (int i = 0; i < N; i++)
  {
    vals[i] = (uint16_t)analogReadMilliVolts(BAT_ADC_IN);
    delay(2);
  }

  pinMode(BAT_ADC_CTRL, INPUT);
  qsort(vals, N, sizeof(vals[0]), cmpUint16);

  uint32_t sum = 0;
  for (int i = 2; i < (N - 2); i++)
    sum += vals[i];
  return sum / (uint32_t)(N - 4);
}

static float readBatteryVoltageRaw()
{
  uint32_t mv = readAdcMilliVoltsStable();
  float v = ((float)mv / 1000.0f) * 2.0f;
  v *= s_battery.calibrationFactor;
  return v;
}

static int batteryPercentFromOCV(float v)
{
  struct BatPoint
  {
    float v;
    int pct;
  };
  static const BatPoint lut[] = {
      {4.20f, 100}, {4.15f, 95}, {4.11f, 90}, {4.08f, 85}, {4.05f, 80}, {4.02f, 75}, {3.99f, 70}, {3.96f, 62}, {3.93f, 55}, {3.90f, 48}, {3.87f, 40}, {3.84f, 32}, {3.81f, 24}, {3.78f, 18}, {3.75f, 13}, {3.72f, 9}, {3.69f, 6}, {3.65f, 4}, {3.55f, 2}, {3.40f, 0}};

  if (v >= lut[0].v)
    return 100;
  const int n = (int)(sizeof(lut) / sizeof(lut[0]));
  if (v <= lut[n - 1].v)
    return 0;

  for (int i = 0; i < n - 1; i++)
  {
    float vHi = lut[i].v;
    float vLo = lut[i + 1].v;
    int pHi = lut[i].pct;
    int pLo = lut[i + 1].pct;
    if (v <= vHi && v >= vLo)
    {
      float t = (v - vLo) / (vHi - vLo);
      int pct = (int)(pLo + t * (float)(pHi - pLo) + 0.5f);
      if (pct < 0)
        pct = 0;
      if (pct > 100)
        pct = 100;
      return pct;
    }
  }
  return 0;
}

void updateBatteryBackground()
{
  uint32_t now = millis();
  bool needFull = (now - s_battery.lastMs) >= BAT_CACHE_MS;
  if (needFull) {
    updateBatteryCached(/*force=*/true);
  }
}

void updateBatteryCached(bool force)
{
  uint32_t now = millis();
  bool needFull = force || (now - s_battery.lastMs) >= BAT_CACHE_MS;
  bool chargingCheckDue = force || (now - s_battery.lastChargingCheckMs) >= BAT_CHARGING_CHECK_MS;
  if (!needFull && !chargingCheckDue)
    return;

  float raw = readBatteryVoltageRaw();
  if (chargingCheckDue)
  {
    bool chargingNow = (raw >= BAT_CHARGING_VOLTAGE);
    s_battery.lastChargingCheckMs = now;
    s_battery.chargingChanged = !force && (chargingNow != s_battery.charging);
    s_battery.charging = chargingNow;
  }

  if (!needFull)
    return;

  s_battery.lastMs = now;
  bool valid = (raw > 2.8f && raw < 4.5f);
  s_battery.valid = valid;
  if (!valid)
    return;

  s_battery.rawV = raw;
  if (s_battery.filteredV <= 0.0f)
  {
    s_battery.filteredV = raw;
  }
  else
  {
    const float alpha = 0.22f;
    s_battery.filteredV = (alpha * raw) + ((1.0f - alpha) * s_battery.filteredV);
  }
  s_battery.filteredV = clampf(s_battery.filteredV, 3.0f, 4.25f);
  s_battery.pctRaw = batteryPercentFromOCV(s_battery.filteredV);

  if (force)
  {
    s_battery.pctShown = s_battery.pctRaw;
  }
  else
  {
    if (s_battery.pctRaw < s_battery.pctShown)
    {
      s_battery.pctShown--;
    }
    else if (s_battery.pctRaw > s_battery.pctShown + 2)
    {
      s_battery.pctShown++;
    }
  }

  if (s_battery.pctShown < 0)
    s_battery.pctShown = 0;
  if (s_battery.pctShown > 100)
    s_battery.pctShown = 100;

  if (!s_battery.low && s_battery.pctShown <= 8)
    s_battery.low = true;
  else if (s_battery.low && s_battery.pctShown >= 12)
    s_battery.low = false;
}

bool batteryChargingChanged()
{
  bool changed = s_battery.chargingChanged;
  s_battery.chargingChanged = false;
  return changed;
}

// Draws the outline of a battery with top-left corner at (x, y), width `w` and height `h`
void drawBatteryOutline(int x, int y, int w, int h)
{
  gfx.drawRect(x, y, w, h, 1);             // main body
  gfx.fillRect(x + w, y + 2, 2, h - 4, 1); // positive terminal on the right
}

// Draws a solid bar indicating `perc` percentage of the battery charge. 0<= perc <= 100.
// The coordinates refer to the top-left corner of the battery
// `w` and `h` are the width and height of the battery
void drawBatteryCharge(int battX, int battY, int battW, int battH, int perc)
{
  if (perc == 0)
  {
    return;
  }

  int fillW = perc * (battW - 2) / 100;
  gfx.fillRect(battX + 1, battY + 1, fillW, battH - 2, 1);
}

// Draws an exclamation mark leaving `spacing` pixels between it and the LEFT of the battery.
void drawExclamation(int battX, int battY, int battH, int spacing)
{
  int exMarkX1 = battX - spacing - 2;
  gfx.fillRect(exMarkX1, battY, 2, battH - 4, 1);     // "pipe" part of the exclamation mark
  gfx.fillRect(exMarkX1, battY + battH - 2, 2, 2, 1); // "dot"
}

void drawChargingFill(int battX, int battY, int battH, int battW)
{
  gfx.fillTriangle(battX, battY, battX, battY + battH - 1, battX + battH, battY, 1);                                             // left triangle
  gfx.fillTriangle(battX + battW - 1, battY, battX + battW - 1, battY + battH - 1, battX + battW - battH, battY + battH - 1, 1); // right triangle
}

// Draws a bolt symbol leaving `spacing` pixels between it and the LEFT of the battery.
//    a
//   /
// b -- c
//    /
//   d

void drawBolt(int battX, int battY, int battH, int spacing)
{
  int a_x = battX - spacing;
  int a_y = battY;
  int b_x = battX - spacing - battH / 2;
  int b_y = battY + battH / 2;
  int c_x = a_x - 1;
  int c_y = b_y;
  int d_x = b_x - 1;
  int d_y = battY + battH - 1;

  int thickness = 2;

  for (int i = 0; i < thickness; i++)
  {
    gfx.drawLine(a_x - i, a_y, b_x - i, b_y, 1);
    gfx.drawLine(b_x - i, b_y, c_x - i, c_y, 1);
    gfx.drawLine(c_x - i, c_y, d_x - i, d_y, 1);
  }
}

const int iconW = 18;
const int iconH = 9;

void drawBattery(int xIcon, int yIcon)
{
  updateBatteryCached(false);
  Font::useUiSmall();
  int pct = s_battery.valid ? s_battery.pctShown : 0;
  if (pct < 0)
    pct = 0;
  if (pct > 100)
    pct = 100;

  drawBatteryOutline(xIcon, yIcon, iconW, iconH);
  int displayedCharge = 0;

  if (pct > 75)
  {
    displayedCharge = 100;
  }
  else if (pct > 50)
  {
    displayedCharge = 50;
  }
  else if (pct > 25)
  {
    displayedCharge = 25;
  }
  else
  {
    displayedCharge = 0;
    drawExclamation(xIcon, yIcon, iconH, 2);
  }

  if (!s_battery.charging) {
    drawBatteryCharge(xIcon, yIcon, iconW, iconH, displayedCharge);
  }
  else {
    drawBolt(xIcon, yIcon, iconH, 2);
    drawChargingFill(xIcon, yIcon, iconH, iconW);
  }
}

void drawBatteryTopRight(bool extended)
{
  updateBatteryCached(false);
  int pct = s_battery.valid ? s_battery.pctShown : 0;
  if (pct < 0)
    pct = 0;
  if (pct > 100)
    pct = 100;

  char extendedInfo[11];
  int icon_extendedInfo_spacing = 5;
  if (extended) {
    snprintf(extendedInfo, sizeof(extendedInfo), "%d%%/%.2fV", pct, readBatteryVoltageRaw());
  }
  int xIcon = SCREEN_W - MARGIN_X - iconW - 2 - (extended ? u8g2.getUTF8Width(extendedInfo) + icon_extendedInfo_spacing : 0);
  int yIcon = 2;

  // Clear previous icons
  gfx.fillRect(xIcon, yIcon, iconW + u8g2.getUTF8Width(extendedInfo) + icon_extendedInfo_spacing, iconH, 0);

  drawBattery(xIcon, yIcon);

  if (extended) {
    Font::useUiSmall();
    u8g2.setCursor(xIcon + iconW + icon_extendedInfo_spacing, yIcon + iconH - 1);
    u8g2.print(extendedInfo);
  }
}

void drawBatteryBottomLeft()
{
	drawBattery(/*xIcon=*/MARGIN_X + 2, /*yIcon=*/SCREEN_H - iconH);
}

bool batteryLow()
{
  return s_battery.valid && s_battery.low;
}

// void drawBatteryTopRight()
// {
//   updateBatteryCached(false);

//   int pct = s_battery.valid ? s_battery.pctShown : 0;
//   if (pct < 0)
//     pct = 0;
//   if (pct > 100)
//     pct = 100;

//   const int iconW = 18;
//   const int iconH = 9;
//   int xIcon = SCREEN_W - MARGIN_X - iconW - 2;
//   int yIcon = 2;

//   gfx.drawRect(xIcon, yIcon, iconW, iconH, 1);
//   gfx.fillRect(xIcon + iconW, yIcon + 2, 2, iconH - 4, 1);

//   int innerW = iconW - 2;
//   int fillW = (innerW * pct) / 100;
//   if (fillW > 0)
//     gfx.fillRect(xIcon + 1, yIcon + 1, fillW, iconH - 2, 1);
//   if (s_battery.low && pct > 0)
//     gfx.drawLine(xIcon + 3, yIcon + 2, xIcon + 3, yIcon + iconH - 3, 0);

//   Font::useUiSmall();
//   char buf[8];
//   if (s_battery.valid) {
//     if (s_battery.charging) {
//       snprintf(buf, sizeof(buf), "USB");
//     } else {
//       snprintf(buf, sizeof(buf), "%d%%", pct);
//     }
//   } else {
//     snprintf(buf, sizeof(buf), "--");
//   }
//   int wTxt = u8g2.getUTF8Width(buf);
//   u8g2.setCursor(xIcon - 4 - wTxt, yIcon + 8);
//   u8g2.print(buf);
//   Font::useBody();
// }

#endif
