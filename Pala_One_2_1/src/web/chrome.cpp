#include "src/web/web_locale.h"
#include "src/web/web_assets.h"
#include "src/lang/join.h"
#include "src/web/chrome.h"

#include "src/state.h"            // server, FS
#include "src/storage/fs_util.h"  // fs*BytesSafe
#include "src/storage/library.h"  // g_library

// ============================================================================
//  Stylesheet — served as /style.css. Browsers cache for an hour, so visiting
//  multiple pages doesn't re-download. Bumping CSS requires a refresh on the
//  client (or a wait); acceptable for a personal-device UI.
//
//  All colors flow through CSS custom properties. `:root` carries the light
//  palette; `html[data-theme=dark]` overrides for dark. The theme attribute
//  is set client-side by the inline `kThemeScript` (see webPageStart), reading
//  `localStorage.palaTheme` first and falling back to `prefers-color-scheme`.
//  No server-side persistence — theme is a per-browser preference.
// ============================================================================
// Static payload served from losslessly compressed, content-hashed assets.

// Compile-time default for the *first* visit. Toggled in Pala_One_2_1.ino
// (Arduino IDE) or via -D WEB_THEME_DARK in platformio.ini (PlatformIO).
// Once the visitor uses the toggle button, localStorage.palaTheme wins and
// this default no longer matters for that browser.
#if defined(WEB_THEME_DARK)
  #define WEB_DEFAULT_THEME_JS "dark"
#else
  #define WEB_DEFAULT_THEME_JS "light"
#endif

// Inline pre-paint theme script. Runs in <head> before body renders so the
// data-theme attribute is set before first paint and the user doesn't see a
// flash of the wrong palette. Reads localStorage.palaTheme first; falls back
// to the build-time default above. Exposes window.palaToggleTheme() for the
// button.
static const char kThemeScript[] PROGMEM =
  "<script>(function(){"
  "var k='palaTheme',r=document.documentElement;"
  "function S(t){r.dataset.theme=(t==='dark')?'dark':'light';try{localStorage.setItem(k,r.dataset.theme)}catch(e){}}"
  "var v=null;try{v=localStorage.getItem(k)}catch(e){}"
  "S((v==='dark'||v==='light')?v:'" WEB_DEFAULT_THEME_JS "');"
  "window.palaToggleTheme=function(){S(r.dataset.theme==='dark'?'light':'dark')};"
  "})();</script>";

static void handleStyleCss() {
  WebAssets::serve(WebAssets::Id::CoreCss,false);
}

void registerChromeRoutes() {
  WebAssets::registerRoutes();
  server.on("/style.css", HTTP_GET, handleStyleCss);
}

// ============================================================================
//  Page chrome
// ============================================================================
String webPageStart(const String& title, const String& subtitle,
                    const String& navHtml, bool wide) {
  String out;
  out.reserve(1400);
  out = String("<!doctype html><html lang='") + Language::currentCode() + "'><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta name='color-scheme' content='light dark'>";
  WebAssets::appendStyle(out,WebAssets::Id::CoreCss);
  out += FPSTR(kThemeScript);
  out += "<title>";
  out += title;
  out += "</title></head><body><div class='wrap";
  if (wide) out += " wide";
  out += "'><div class='top'><div><h1>";
  out += title;
  out += "</h1><div class='muted'>";
  out += subtitle;
  out += "</div></div><div class='top-side'>";
  if (navHtml.length() > 0) {
    out += "<div class='nav'>";
    out += navHtml;
    out += "</div>";
  }
  out += "<button type='button' class='btn secondary theme-toggle' "
         "onclick='palaToggleTheme()' title='";
  out += htmlEscape(D_WEB_THEME_TITLE);
  out += "'><span class='tgl'>"; out += D_WEB_THEME_DARK;
  out += "</span><span class='tgd'>"; out += D_WEB_THEME_LIGHT;
  out += "</span></button></div></div>";
  appendWebLocale(out);
  return out;
}

String webPageEnd() {
  return String("</div></body></html>");
}

String successPage(const String& title, const String& subtitle,
                   const String& banner, const String& innerHtml) {
  String out = webPageStart(title, subtitle,
    Language::join({"<a href='/'>",  D_WEB_NAV_HOME,  "</a><a href='/files'>",  D_WEB_NAV_FILES,  "</a><a href='/settings'>",  D_WEB_NAV_SETTINGS,  "</a>"}));
  out += "<div class='banner-ok'>" + banner + "</div>";
  out += innerHtml;
  out += webPageEnd();
  return out;
}

// ============================================================================
//  Small helpers
// ============================================================================
String htmlEscape(const String& in) {
  String out;
  out.reserve(in.length() + 16);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if      (c == '&')  out += "&amp;";
    else if (c == '<')  out += "&lt;";
    else if (c == '>')  out += "&gt;";
    else if (c == '"')  out += "&quot;";
    else if (c == '\'') out += "&#39;";
    else                out += c;
  }
  return out;
}

String humanBytes(size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  if (bytes < (1024UL * 1024UL)) return String(bytes / 1024.0f, 1) + " KB";
  return String(bytes / 1024.0f / 1024.0f, 2) + " MB";
}

int storageUsedPct() {
  size_t totalBytes = fsTotalBytesSafe();
  size_t usedBytes  = fsUsedBytesSafe();
  if (totalBytes == 0) return 0;
  int pct = (int)((usedBytes * 100UL) / totalBytes);
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

String storageCardHtml(const char* title) {
  size_t totalBytes = fsTotalBytesSafe();
  size_t usedBytes  = fsUsedBytesSafe();
  size_t freeBytes  = fsFreeBytesSafe();
  int    pct        = storageUsedPct();

  String out;
  out.reserve(900);
  out += "<div class='card'><h2>";
  out += title;
  out += "</h2><div class='stats'>";
  out += Language::join({"<div class='stat'><span class='muted'>",  D_WEB_STORAGE_BOOKS,  "</span><b>"}) + String(g_library.bookCount) + "</b></div>";
  out += Language::join({"<div class='stat'><span class='muted'>",  D_WEB_STORAGE_USED,   "</span><b>"}) + humanBytes(usedBytes)         + "</b></div>";
  out += Language::join({"<div class='stat'><span class='muted'>",  D_WEB_STORAGE_FREE,   "</span><b>"}) + humanBytes(freeBytes)         + "</b></div>";
  out += Language::join({"<div class='stat'><span class='muted'>",  D_WEB_STORAGE_TOTAL,  "</span><b>"}) + humanBytes(totalBytes)        + "</b></div>";
  out += "</div><div class='bar'><span style='width:" + String(pct) + "%'></span></div>";
  out += "<div class='muted' style='margin-top:8px'>" + String(pct) + Language::join({D_WEB_STORAGE_PCT_SUFFIX,  "</div></div>"});
  return out;
}
