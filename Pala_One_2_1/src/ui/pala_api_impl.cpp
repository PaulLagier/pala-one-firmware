#include "src/ui/pala_api_impl.h"

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

// `esp_rtc_get_time_us` lives in a chip-specific header (`soc/esp32s3/rtc.h`
// here, `soc/esp32/rtc.h` on classic ESP32, etc.). Forward-declaring it
// keeps this file building across chip variants without touching the
// include path. The symbol is always present at link time.
extern "C" uint64_t esp_rtc_get_time_us(void);

#include "src/hal/app_loader.h"       // loadAndRunApp, LoadResult
#include "src/hal/display.h"          // u8g2, display
#include "src/hal/input.h"            // g_btns, waitForNextEvent, ButtonEvent
#include "pala_api.h"             // PalaAPI
#include "pala_app.h"             // PALA_CLICK/DOUBLE/TRIPLE/LONG
#include "src/state.h"                // FS, prefs
#include "src/ui/font.h"
#include "src/ui/widgets.h"           // drawCenter, drawSectionHeader, prepareMenuFrame

// The one and only PalaAPI instance. File-private; the entry function
// pointer the loader hands to an app is `&s_palaAPI`.
static PalaAPI s_palaAPI;

// Map ButtonEvent::Kind → PALA_* event codes. Quad has no PALA equivalent
// (the apps API v3 didn't define one); treated as "no event".
static uint8_t toPalaCode(ButtonEvent::Kind k) {
  switch (k) {
    case ButtonEvent::Short:     return PALA_CLICK;
    case ButtonEvent::Double:    return PALA_DOUBLE;
    case ButtonEvent::Triple:    return PALA_TRIPLE;
    case ButtonEvent::Long:      return PALA_LONG;
    case ButtonEvent::Quad:      return 0;  // no PALA_QUAD in v3
    case ButtonEvent::VeryLong:  return 0;  // no PALA_VERYLONG in v3
    case ButtonEvent::ClickHold: return 0;  // no PALA_CLICKHOLD in v3
    case ButtonEvent::None:      return 0;
  }
  return 0;
}

// ----------------------------------------------------------------------------
//  Display wrappers
// ----------------------------------------------------------------------------

static void api_clearScreen() {
  prepareMenuFrame();
}

static void api_drawHeader(const char* title) {
  drawSectionHeader(title);
}

static void api_drawTextAt(int x, int y, const char* text, int bold) {
  if (bold) Font::useBold(); else Font::useBody();
  u8g2.setCursor(x, y);
  u8g2.print(text);
  Font::useBody();
}

static void api_drawCenteredLarge(const char* text) {
  Font::useAppLarge();
  int w   = u8g2.getUTF8Width(text);
  int asc = u8g2.getFontAscent();
  // Centre horizontally; vertically centred in the space below y≈20
  // (just under the header bar). Matches the old api_drawCenteredLarge
  // pixel-for-pixel.
  u8g2.setCursor((SCREEN_W - w) / 2, (SCREEN_H + 20 + asc) / 2);
  u8g2.print(text);
  Font::useBody();
}

static void api_refreshDisplay() {
  display.update();
}

// ----------------------------------------------------------------------------
//  Input wrappers
// ----------------------------------------------------------------------------

static uint8_t api_waitForEvent() {
  ButtonEvent e = waitForNextEvent();
  return toPalaCode(e.kind);
}

static uint8_t api_pollEvent() {
  g_btns.poll();
  ButtonEvent e = ButtonEvent::fromButtonState(g_btns);
  if (e.any()) {
    g_btns.resetClicks();
    markUserActivity();
  }
  return toPalaCode(e.kind);
}

static int api_buttonPressed() {
  return g_btns.isPressed() ? 1 : 0;
}

static uint32_t api_pendingPresses() {
  g_btns.poll();
  uint32_t n = g_btns.consumePressCount();
  if (n) markUserActivity();
  return n;
}

// ----------------------------------------------------------------------------
//  Time wrappers
// ----------------------------------------------------------------------------

static uint32_t api_millisNow() {
  return millis();
}

static void api_delayMs(uint32_t ms) {
  delay(ms);
}

static uint32_t api_rtcSeconds() {
  // Monotonic RTC clock — survives deep sleep. The Arduino `millis()`
  // resets on every wake; this doesn't. Apps use it for cross-session
  // timers (e.g. "next reminder in 3600 s").
  return (uint32_t)(esp_rtc_get_time_us() / 1000000ULL);
}

// ----------------------------------------------------------------------------
//  Formatting + storage wrappers
// ----------------------------------------------------------------------------

static int api_snprintf_wrap(char* buf, int len, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int r = vsnprintf(buf, (size_t)len, fmt, args);
  va_end(args);
  return r;
}

// Per-app key-value storage: each app gets its own `/apps/{key}.dat` file.
// Key namespacing across apps is the app author's responsibility (the v3
// contract didn't sandbox by binary name). Path separators in `key` are
// replaced with '_' to prevent directory traversal.

// Strip path-separator characters so app-supplied keys cannot escape /apps/.
static void sanitizeStorageKey(char* out, size_t outLen, const char* key) {
  size_t j = 0;
  for (size_t i = 0; key[i] && j < outLen - 1; i++) {
    char c = key[i];
    if (c == '/' || c == '\\') c = '_';
    out[j++] = c;
  }
  out[j] = '\0';
}

static int api_storageRead(const char* key, void* buf, int maxlen) {
  if (!key || !key[0]) return -1;
  char safeKey[54];
  sanitizeStorageKey(safeKey, sizeof(safeKey), key);
  char path[64];
  snprintf(path, sizeof(path), "/apps/%s.dat", safeKey);
  File f = FS.open(path, "r");
  if (!f) return -1;
  int n = f.read((uint8_t*)buf, maxlen);
  f.close();
  return n;
}

static int api_storageWrite(const char* key, const void* buf, int len) {
  if (!key || !key[0]) return -1;
  char safeKey[54];
  sanitizeStorageKey(safeKey, sizeof(safeKey), key);
  char path[64];
  snprintf(path, sizeof(path), "/apps/%s.dat", safeKey);
  File f = FS.open(path, "w");
  if (!f) return -1;
  int n = f.write((const uint8_t*)buf, len);
  f.close();
  return n;
}

// ----------------------------------------------------------------------------
//  Init + stub runner
// ----------------------------------------------------------------------------

void initPalaAPI() {
  // Field order is frozen — assignments below MUST match pala_api.h struct
  // order. Adding a field means appending here AND in pala_api.h AND
  // bumping PALA_API_VERSION in pala_app.h.
  s_palaAPI.clearScreen       = api_clearScreen;
  s_palaAPI.drawHeader        = api_drawHeader;
  s_palaAPI.drawTextAt        = api_drawTextAt;
  s_palaAPI.drawCenteredLarge = api_drawCenteredLarge;
  s_palaAPI.refreshDisplay    = api_refreshDisplay;
  s_palaAPI.waitForEvent      = api_waitForEvent;
  s_palaAPI.snprintf_wrap     = api_snprintf_wrap;
  s_palaAPI.pollEvent         = api_pollEvent;
  s_palaAPI.millisNow         = api_millisNow;
  s_palaAPI.buttonPressed     = api_buttonPressed;
  s_palaAPI.delayMs           = api_delayMs;
  s_palaAPI.pendingPresses    = api_pendingPresses;
  s_palaAPI.storageRead       = api_storageRead;
  s_palaAPI.storageWrite      = api_storageWrite;
  s_palaAPI.rtcSeconds        = api_rtcSeconds;
}

// Paint a two-line error toast for a failed load. Lives here (not in
// app_loader) so the loader stays free of ui/widgets — hal can't include
// ui. `delay(1500)` matches the dwell the old monolithic firmware used.
static void paintLoadError(const char* line1, const char* line2 = nullptr) {
  drawCenter(line1, line2);
  delay(1500);
}

void runApp(const char* path) {
  if (!path) {
    paintLoadError(D_APP_ERR_TITLE, D_APP_ERR_NULL_PATH);
    return;
  }
  uint32_t fileApiVer = 0;
  LoadResult r = loadAndRunApp(path, &s_palaAPI, &fileApiVer);
  switch (r) {
    case LoadResult::Ok:
      // App ran to completion. AppsScreen will repaint itself on return.
      return;
    case LoadResult::FileNotFound:
      paintLoadError(D_APP_ERR_NOT_FOUND, path);
      return;
    case LoadResult::FileTooSmall:
      paintLoadError(D_APP_ERR_TOO_SMALL, D_APP_ERR_INVALID_FILE);
      return;
    case LoadResult::FileTooLarge:
      paintLoadError(D_APP_ERR_TOO_LARGE, D_APP_ERR_SIZE_LIMIT);
      return;
    case LoadResult::ReadError:
      paintLoadError(D_APP_ERR_READ, D_APP_ERR_PARTIAL_READ);
      return;
    case LoadResult::OutOfMemory:
      paintLoadError(D_APP_ERR_NO_EXEC_MEM, nullptr);
      return;
    case LoadResult::BadMagic:
      paintLoadError(D_APP_ERR_BAD_FILE, D_APP_ERR_WRONG_MAGIC);
      return;
    case LoadResult::ApiMismatch: {
      // 64 to match the upload-side buffer in web/apps_upload.cpp. Worst-case
      // current expansion ("API v4294967295, requiere v4294967295") = 38 bytes;
      // keeps room for translation/format changes without re-validating here.
      char msg[64];
      snprintf(msg, sizeof(msg), D_APP_ERR_API_FMT,
               (unsigned)fileApiVer, (unsigned)PALA_API_VERSION);
      paintLoadError(D_APP_ERR_API_MISMATCH, msg);
      return;
    }
    case LoadResult::BadEntryOffset:
      paintLoadError(D_APP_ERR_BAD_ENTRY, nullptr);
      return;
    case LoadResult::BadRelocTable:
      paintLoadError(D_APP_ERR_BAD_RELOC, nullptr);
      return;
    case LoadResult::BadRelocEntry:
      paintLoadError(D_APP_ERR_RELOC_RANGE, nullptr);
      return;
  }
}
