#include "src/web/web_assets.h"
#include "src/lang/join.h"
#include "src/web/chrome.h"

#include "src/state.h"           // server, FS
#include "src/storage/fs_util.h" // fs*BytesSafe
#include "src/storage/library.h" // g_library

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
// Also exposes PalaTogglePw to allow password fields to toggle between hidden and shown.
static const char kThemeScript[] PROGMEM =
    "<script>(function(){"
    "var k='palaTheme',r=document.documentElement;"
    "function "
    "S(t){r.dataset.theme=(t==='dark')?'dark':'light';try{localStorage.setItem(k,r.dataset.theme)}"
    "catch(e){}}"
    "var v=null;try{v=localStorage.getItem(k)}catch(e){}"
    "S((v==='dark'||v==='light')?v:'" WEB_DEFAULT_THEME_JS "');"
    "window.palaToggleTheme=function(){S(r.dataset.theme==='dark'?'light':'dark')};"
    "window.palaTogglePw=function(b){var i=b.parentNode.querySelector('input');var "
    "s=(i.type==='password');i.type=s?'text':'password';b.classList.toggle('shown',s);};"
    // Positional formatter for the D_*_JS_*_FMT strings. %1..%9 are replaced by
    // the trailing arguments, so a translation can reorder them freely. Lives
    // here because this script is emitted on every page; the find and editor
    // script blocks both rely on it.
    "window.palaFmt=function(s){var a=arguments;"
    "return String(s).replace(/%([1-9])/g,function(m,i){"
    "var v=a[+i];return v===undefined?m:String(v);});};"
    "})();</script>";

static void handleStyleCss() { WebAssets::serve(WebAssets::Id::CoreCss, false); }

void registerChromeRoutes() {
  WebAssets::registerRoutes();
  server.on("/style.css", HTTP_GET, handleStyleCss);
}

// ============================================================================
//  Page chrome
// ============================================================================
String webPageStart(const String &title, const String &subtitle, const String &navHtml, bool wide) {
  String out;
  out.reserve(1400);
  out = "<!doctype html><html><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta name='color-scheme' content='light dark'>"
        "<link rel='stylesheet' href='/style.css'>";
  out += FPSTR(kThemeScript);
  out += "<title>";
  out += title;
  out += "</title></head><body><div class='wrap";
  if (wide)
    out += " wide";
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
  out +=
      Language::join({"<button type='button' class='btn secondary theme-toggle' ",
                      "onclick='palaToggleTheme()' title='", D_WEB_THEME_TOGGLE_TITLE, "'>",
                      "<span class='tgl'>", D_WEB_THEME_DARK_MODE, "</span>", "<span class='tgd'>",
                      D_WEB_THEME_LIGHT_MODE, "</span>", "</button></div></div>"})
          .c_str();
  return out;
}

String webPageEnd() { return String("</div></body></html>"); }

String successPage(const String &title, const String &subtitle, const String &banner,
                   const String &innerHtml) {
  String out = webPageStart(
      title, subtitle,
      Language::join({"<a href='/'>", D_WEB_NAV_HOME, "</a><a href='/files'>", D_WEB_NAV_FILES,
                      "</a><a href='/settings'>", D_WEB_NAV_SETTINGS, "</a>"})
          .c_str());
  out += "<div class='banner-ok'>" + banner + "</div>";
  out += innerHtml;
  out += webPageEnd();
  return out;
}

// ============================================================================
//  Small helpers
// ============================================================================
String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length() + 16);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c == '&')
      out += "&amp;";
    else if (c == '<')
      out += "&lt;";
    else if (c == '>')
      out += "&gt;";
    else if (c == '"')
      out += "&quot;";
    else if (c == '\'')
      out += "&#39;";
    else
      out += c;
  }
  return out;
}

String humanBytes(size_t bytes) {
  if (bytes < 1024)
    return String(bytes) + " B";
  if (bytes < (1024UL * 1024UL))
    return String(bytes / 1024.0f, 1) + " KB";
  return String(bytes / 1024.0f / 1024.0f, 2) + " MB";
}

int storageUsedPct() {
  size_t totalBytes = fsTotalBytesSafe();
  size_t usedBytes = fsUsedBytesSafe();
  if (totalBytes == 0)
    return 0;
  int pct = (int)((usedBytes * 100UL) / totalBytes);
  if (pct < 0)
    pct = 0;
  if (pct > 100)
    pct = 100;
  return pct;
}

String storageCardHtml(const char *title) {
  size_t totalBytes = fsTotalBytesSafe();
  size_t usedBytes = fsUsedBytesSafe();
  size_t freeBytes = fsFreeBytesSafe();
  int pct = storageUsedPct();

  String out;
  out.reserve(900);
  out += "<div class='card'><h2>";
  out += title;
  out += "</h2><div class='stats'>";
  out +=
      Language::join({"<div class='stat'><span class='muted'>", D_WEB_STORAGE_BOOKS, "</span><b>"})
          .c_str() +
      String(g_library.bookCount) + "</b></div>";
  out +=
      Language::join({"<div class='stat'><span class='muted'>", D_WEB_STORAGE_USED, "</span><b>"})
          .c_str() +
      humanBytes(usedBytes) + "</b></div>";
  out +=
      Language::join({"<div class='stat'><span class='muted'>", D_WEB_STORAGE_FREE, "</span><b>"})
          .c_str() +
      humanBytes(freeBytes) + "</b></div>";
  out +=
      Language::join({"<div class='stat'><span class='muted'>", D_WEB_STORAGE_TOTAL, "</span><b>"})
          .c_str() +
      humanBytes(totalBytes) + "</b></div>";
  out += "</div><div class='bar'><span style='width:" + String(pct) + "%'></span></div>";
  out += "<div class='muted' style='margin-top:8px'>" + String(pct) +
         Language::join({D_WEB_STORAGE_PCT_SUFFIX, "</div></div>"}).c_str();
  return out;
}
