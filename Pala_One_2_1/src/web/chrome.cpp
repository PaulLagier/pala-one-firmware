#include "src/web/chrome.h"

#include "src/state.h"            // server, FS
#include "src/storage/fs_util.h"  // fs*BytesSafe
#include "src/storage/library.h"  // g_library

// ============================================================================
//  Stylesheet — served as /style.css. Browsers cache for an hour, so visiting
//  multiple pages doesn't re-download. Bumping CSS requires a refresh on the
//  client (or a wait); acceptable for a personal-device UI.
// ============================================================================
static const char kStyleCss[] PROGMEM =
  ":root{--bg:#f3efe7;--card:#fff;--line:#ddd4c7;--line-soft:#ece5d9;--text:#1f2328;--muted:#667085;--link:#3c5a7a;--ok:#216e39;--okbg:#e7f6ec;--warn:#8a5a00;--warnbg:#fff4d6;--danger:#6e2a2a}"
  "*{box-sizing:border-box}"
  "body{margin:0;background:var(--bg);color:var(--text);font:15px/1.45 system-ui,sans-serif}"
  ".wrap{max-width:820px;margin:0 auto;padding:18px}"
  ".wide{max-width:1020px}"
  ".top{display:flex;justify-content:space-between;align-items:flex-start;gap:12px;margin-bottom:14px}"
  ".top a,.link{color:var(--link);text-decoration:none}"
  ".top a:hover,.link:hover{text-decoration:underline}"
  ".muted{color:var(--muted);font-size:13px}"
  ".card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px 15px;margin:0 0 14px;box-shadow:0 1px 0 rgba(0,0,0,.03)}"
  ".grid{display:grid;gap:12px}"
  ".actions{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-top:14px}"
  ".nav{display:flex;flex-wrap:wrap;gap:10px 14px;font-size:14px}"
  ".nav a{color:var(--link);text-decoration:none}"
  ".list{list-style:none;padding:0;margin:0}"
  ".list li{padding:11px 0;border-top:1px solid var(--line-soft)}"
  ".list li:first-child{border-top:0;padding-top:0}"
  ".row{display:flex;justify-content:space-between;gap:12px;align-items:flex-start}"
  ".meta{color:var(--muted);font-size:13px}"
  ".pill{display:inline-block;background:#f6f2ea;color:#6b6358;border-radius:999px;padding:3px 8px;font-size:12px}"
  ".pre{white-space:pre-wrap;line-height:1.45;padding:12px;border:1px solid var(--line);border-radius:10px;background:#fcfaf7}"
  ".danger{background:var(--danger)}"
  ".banner-ok{background:var(--okbg);color:var(--ok);border:1px solid #cfe9d7;border-radius:12px;padding:12px 13px;margin-bottom:14px}"
  ".banner-warn{background:var(--warnbg);color:var(--warn);border:1px solid #ecd9a3;border-radius:12px;padding:12px 13px;margin-bottom:14px}"
  ".stats{display:grid;gap:10px;grid-template-columns:repeat(2,minmax(0,1fr));margin-top:12px}"
  ".stat{padding:11px 12px;border:1px solid var(--line-soft);border-radius:12px;background:#fcfaf7}"
  ".stat b{display:block;font-size:17px;line-height:1.2;margin-top:2px}"
  ".bar{height:10px;border-radius:999px;background:#ece5d9;overflow:hidden;border:1px solid #e0d7ca;margin-top:12px}"
  ".bar > span{display:block;height:100%;background:#3c5a7a}"
  ".stack{display:grid;gap:8px}"
  ".small{font-size:13px}"
  "label{display:block;font-weight:600;margin:0 0 6px}"
  ".hint{margin:6px 0 0;color:var(--muted);font-size:12px}"
  ".status{padding:10px 12px;border-radius:10px;font-size:14px;margin:10px 0 0}"
  ".status.ok{background:var(--okbg);color:var(--ok)}"
  ".status.idle{background:#f6f2ea;color:#6b6358}"
  "button,.btn{display:inline-flex;align-items:center;justify-content:center;border:0;border-radius:10px;background:#1f2328;color:#fff;padding:10px 14px;font:600 14px system-ui,sans-serif;text-decoration:none;cursor:pointer}"
  ".btn.secondary{background:#eef2f6;color:#334e68;border:1px solid #d8e0e8}"
  "input[type=text],input[type=file],select{width:100%;box-sizing:border-box;border:1px solid #c9c2b8;border-radius:10px;background:#fff;padding:10px;font:inherit}"
  "h1,h2,h3,p{margin:0}"
  "h1,h2,h3{margin-bottom:6px}"
  "p + p{margin-top:10px}"
  "@media(min-width:760px){.stats{grid-template-columns:repeat(4,minmax(0,1fr))}}"
  "@media(min-width:620px){.grid.cols-2{grid-template-columns:1fr 1fr}}"
  "@media(max-width:640px){.row,.top{flex-direction:column}.wrap{padding:14px}}"
  ;

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
  out.reserve(900);
  out = "<!doctype html><html><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<link rel='stylesheet' href='/style.css'><title>";
  out += title;
  out += "</title></head><body><div class='wrap";
  if (wide) out += " wide";
  out += "'><div class='top'><div><h1>";
  out += title;
  out += "</h1><div class='muted'>";
  out += subtitle;
  out += "</div></div>";
  if (navHtml.length() > 0) {
    out += "<div class='nav'>";
    out += navHtml;
    out += "</div>";
  }
  out += "</div>";
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
