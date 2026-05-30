#include "src/web/app.h"

#include "src/config.h"                       // FW_VERSION, BUILD_GIT_HASH
#include "src/state.h"                        // server
#include "src/web/generated/webui.gz.h"       // kWebUiGz, kWebUiGzLen

// Build-time language tag exposed to the SPA via /api/info. The on-device
// UI strings are still selected at compile time via LANG_* (see
// src/lang/lang.h); this constant just lets the browser know which locale
// the firmware was built for so the SPA can default to a matching one.
#if defined(LANG_ES_LA)
  static const char kBuildLang[] = "es";
#else
  static const char kBuildLang[] = "en";
#endif

// ----------------------------------------------------------------------------
//  GET /
//  Serves the gzipped SPA shell straight out of PROGMEM. Browsers all
//  speak gzip, so Accept-Encoding negotiation is a formality we skip.
// ----------------------------------------------------------------------------
static void handleApp() {
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", "public, max-age=600");
  server.send_P(200, "text/html; charset=utf-8",
                reinterpret_cast<const char*>(kWebUiGz), kWebUiGzLen);
}

// ----------------------------------------------------------------------------
//  GET /api/info
//  Tiny ~80-byte JSON: build-time language, firmware version, git hash.
//  Hand-written rather than via ArduinoJson because the payload is
//  fully-known at build time and snprintf is half the flash cost.
// ----------------------------------------------------------------------------
static void handleApiInfo() {
  char body[160];
  int n = snprintf(body, sizeof(body),
    "{\"lang\":\"%s\",\"fw\":\"%s\",\"build\":\"%s\"}",
    kBuildLang, FW_VERSION, BUILD_GIT_HASH);
  if (n < 0 || n >= (int)sizeof(body)) {
    server.send(500, "text/plain; charset=utf-8", "info payload too large");
    return;
  }
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", body);
}

void registerAppRoutes() {
  server.on("/",         HTTP_GET, handleApp);
  server.on("/api/info", HTTP_GET, handleApiInfo);
}
