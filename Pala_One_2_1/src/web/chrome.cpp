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
static const char kStyleCss[] PROGMEM =
  ":root{--bg:#f3efe7;--card:#fff;--line:#ddd4c7;--line-soft:#ece5d9;--text:#1f2328;--muted:#667085;--link:#3c5a7a;"
  "--ok:#216e39;--okbg:#e7f6ec;--warn:#8a5a00;--warnbg:#fff4d6;--danger:#6e2a2a;"
  "--pill-bg:#f6f2ea;--pill-fg:#6b6358;--pre-bg:#fcfaf7;--stat-bg:#fcfaf7;"
  "--bar-bg:#ece5d9;--bar-bd:#e0d7ca;--bar-fill:#3c5a7a;"
  "--ban-ok-bd:#cfe9d7;--ban-warn-bd:#ecd9a3;"
  "--btn-bg:#1f2328;--btn-fg:#fff;--btn-sec-bg:#eef2f6;--btn-sec-fg:#334e68;--btn-sec-bd:#d8e0e8;"
  "--inp-bd:#c9c2b8;--inp-bg:#fff}"
  "html[data-theme=dark]{--bg:#14161c;--card:#1c2028;--line:#343a46;--line-soft:#262b34;--text:#e8eaef;--muted:#9aa3b2;--link:#8ab4f8;"
  "--ok:#7dd89a;--okbg:#163526;--warn:#e6c84e;--warnbg:#3a3220;--danger:#f56565;"
  "--pill-bg:#262b34;--pill-fg:#b4bcc8;--pre-bg:#12151c;--stat-bg:#12151c;"
  "--bar-bg:#262b34;--bar-bd:#343a46;--bar-fill:#8ab4f8;"
  "--ban-ok-bd:#2d5a40;--ban-warn-bd:#5c4f24;"
  "--btn-bg:#e8eaef;--btn-fg:#14161c;--btn-sec-bg:#2a3140;--btn-sec-fg:#e8eaef;--btn-sec-bd:#3d4656;"
  "--inp-bd:#454d5c;--inp-bg:#0f1218}"
  "*{box-sizing:border-box}"
  "body{margin:0;background:var(--bg);color:var(--text);font:15px/1.45 system-ui,sans-serif}"
  ".wrap{max-width:820px;margin:0 auto;padding:18px}"
  ".wide{max-width:1020px}"
  ".top{display:flex;justify-content:space-between;align-items:flex-start;gap:12px;margin-bottom:14px;flex-wrap:wrap}"
  ".top a,.link{color:var(--link);text-decoration:none}"
  ".top a:hover,.link:hover{text-decoration:underline}"
  ".top-side{display:flex;flex-wrap:wrap;gap:10px 14px;align-items:center;justify-content:flex-end}"
  ".muted{color:var(--muted);font-size:13px}"
  ".card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px 15px;margin:0 0 14px;box-shadow:0 1px 0 rgba(0,0,0,.03)}"
  "html[data-theme=dark] .card{box-shadow:0 1px 0 rgba(255,255,255,.04)}"
  ".grid{display:grid;gap:12px}"
  ".actions{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-top:14px}"
  ".nav{display:flex;flex-wrap:wrap;gap:10px 14px;font-size:14px;align-items:center}"
  ".nav a{color:var(--link);text-decoration:none}"
  ".list{list-style:none;padding:0;margin:0}"
  ".list li{padding:11px 0;border-top:1px solid var(--line-soft)}"
  ".list li:first-child{border-top:0;padding-top:0}"
  ".row{display:flex;justify-content:space-between;gap:12px;align-items:flex-start}"
  ".meta{color:var(--muted);font-size:13px}"
  ".pill{display:inline-block;background:var(--pill-bg);color:var(--pill-fg);border-radius:999px;padding:3px 8px;font-size:12px}"
  ".pre{white-space:pre-wrap;line-height:1.45;padding:12px;border:1px solid var(--line);border-radius:10px;background:var(--pre-bg)}"
  ".danger{background:var(--danger)}"
  ".banner-ok{background:var(--okbg);color:var(--ok);border:1px solid var(--ban-ok-bd);border-radius:12px;padding:12px 13px;margin-bottom:14px}"
  ".banner-warn{background:var(--warnbg);color:var(--warn);border:1px solid var(--ban-warn-bd);border-radius:12px;padding:12px 13px;margin-bottom:14px}"
  ".stats{display:grid;gap:10px;grid-template-columns:repeat(2,minmax(0,1fr));margin-top:12px}"
  ".stat{padding:11px 12px;border:1px solid var(--line-soft);border-radius:12px;background:var(--stat-bg)}"
  ".stat b{display:block;font-size:17px;line-height:1.2;margin-top:2px}"
  ".bar{height:10px;border-radius:999px;background:var(--bar-bg);overflow:hidden;border:1px solid var(--bar-bd);margin-top:12px}"
  ".bar > span{display:block;height:100%;background:var(--bar-fill)}"
  ".stack{display:grid;gap:8px}"
  ".small{font-size:13px}"
  "label{display:block;font-weight:600;margin:0 0 6px}"
  ".hint{margin:6px 0 0;color:var(--muted);font-size:12px}"
  ".status{padding:10px 12px;border-radius:10px;font-size:14px;margin:10px 0 0}"
  ".status.ok{background:var(--okbg);color:var(--ok)}"
  ".status.idle{background:var(--pill-bg);color:var(--pill-fg)}"
  "button,.btn{display:inline-flex;align-items:center;justify-content:center;border:0;border-radius:10px;background:var(--btn-bg);color:var(--btn-fg);padding:10px 14px;font:600 14px system-ui,sans-serif;text-decoration:none;cursor:pointer}"
  ".btn.secondary{background:var(--btn-sec-bg);color:var(--btn-sec-fg);border:1px solid var(--btn-sec-bd)}"
  "input[type=text],input[type=number],input[type=file],input[type=search],select,textarea{width:100%;box-sizing:border-box;border:1px solid var(--inp-bd);border-radius:10px;background:var(--inp-bg);color:var(--text);padding:10px;font:inherit}"
  "input[type=checkbox],input[type=radio]{accent-color:var(--link)}"
  ".theme-toggle{padding:8px 12px;font-size:13px;white-space:nowrap}"
  ".theme-toggle .tgd{display:none}"
  "html[data-theme=dark] .theme-toggle .tgl{display:none}"
  "html[data-theme=dark] .theme-toggle .tgd{display:inline}"
  "h1,h2,h3,p{margin:0}"
  "h1,h2,h3{margin-bottom:6px}"
  "p + p{margin-top:10px}"
  "@media(min-width:760px){.stats{grid-template-columns:repeat(4,minmax(0,1fr))}}"
  "@media(min-width:620px){.grid.cols-2{grid-template-columns:1fr 1fr}}"
  "@media(max-width:640px){.row,.top{flex-direction:column}.top-side{justify-content:flex-start}.wrap{padding:14px}}"
  ;

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
  server.sendHeader("Cache-Control", "public, max-age=3600");
  server.send_P(200, "text/css; charset=utf-8", kStyleCss);
}

void registerChromeRoutes() {
  server.on("/style.css", HTTP_GET, handleStyleCss);
}

// ============================================================================
//  Page chrome
// ============================================================================
String webPageStart(const String& title, const String& subtitle,
                    const String& navHtml, bool wide) {
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
         "onclick='palaToggleTheme()' title='Light or dark appearance'>"
         "<span class='tgl'>Dark mode</span><span class='tgd'>Light mode</span>"
         "</button></div></div>";
  return out;
}

String webPageEnd() {
  return String("</div></body></html>");
}

String successPage(const String& title, const String& subtitle,
                   const String& banner, const String& innerHtml) {
  String out = webPageStart(title, subtitle,
    "<a href='/'>" D_WEB_NAV_HOME "</a><a href='/files'>" D_WEB_NAV_FILES "</a><a href='/settings'>" D_WEB_NAV_SETTINGS "</a>");
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
  out += "<div class='stat'><span class='muted'>" D_WEB_STORAGE_BOOKS "</span><b>" + String(g_library.bookCount) + "</b></div>";
  out += "<div class='stat'><span class='muted'>" D_WEB_STORAGE_USED  "</span><b>" + humanBytes(usedBytes)         + "</b></div>";
  out += "<div class='stat'><span class='muted'>" D_WEB_STORAGE_FREE  "</span><b>" + humanBytes(freeBytes)         + "</b></div>";
  out += "<div class='stat'><span class='muted'>" D_WEB_STORAGE_TOTAL "</span><b>" + humanBytes(totalBytes)        + "</b></div>";
  out += "</div><div class='bar'><span style='width:" + String(pct) + "%'></span></div>";
  out += "<div class='muted' style='margin-top:8px'>" + String(pct) + D_WEB_STORAGE_PCT_SUFFIX "</div></div>";
  return out;
}
