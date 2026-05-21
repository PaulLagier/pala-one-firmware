#include "src/web/settings.h"

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h"             // prefKeyForBook
#include "src/storage/book_metadata.h"
#include "src/storage/page_cache.h"       // deletePageCacheForBook
#include "src/storage/preferences_store.h"
#include "src/ui/font.h"
#include "src/ui/reader.h"                // g_bookview, findPageForOffset, renderCurrentPage
#include "src/ui/screens/reader_screen.h" // g_readerScreen — active-reader check
#include "src/ui/sleep.h"
#include "src/web/chrome.h"

static void handleSettings() {
  int curFont = Font::currentBodySize();
  String sel8  = (curFont == 8)  ? " selected" : "";
  String sel10 = (curFont == 10) ? " selected" : "";
  String sel12 = (curFont == 12) ? " selected" : "";
  String sel14 = (curFont == 14) ? " selected" : "";

  int curSleep = Sleep::idleTimeoutSecs();
  String ss30   = (curSleep == 30)   ? " selected" : "";
  String ss60   = (curSleep == 60)   ? " selected" : "";
  String ss120  = (curSleep == 120)  ? " selected" : "";
  String ss300  = (curSleep == 300)  ? " selected" : "";
  String ss600  = (curSleep == 600)  ? " selected" : "";
  String ss1800 = (curSleep == 1800) ? " selected" : "";

  int curGap = Font::currentLineGap();
  String lg0 = (curGap == 0) ? " selected" : "";
  String lg1 = (curGap == 1) ? " selected" : "";
  String lg2 = (curGap == 2) ? " selected" : "";
  String lg3 = (curGap == 3) ? " selected" : "";

  Font::Family curFam = Font::currentFamily();
  String famH = (curFam == Font::Family::Helvetica)    ? " selected" : "";
  String famD = (curFam == Font::Family::OpenDyslexic) ? " selected" : "";

  bool curBionic   = Font::bionicEnabled();
  String bChecked  = curBionic ? " checked" : "";

  String out = webPageStart(
    "Pala One Settings",
    "Firmware " FW_BUILD " configuration page stored directly on the device.",
    "<a href='/'>&#8592; Home</a>"
  );
  out.reserve(out.length() + 4000);

  out +=
    "<div class='card'><h2>Reading</h2>"
    "<p class='muted'>Changing the font, family, line spacing, or bionic mode keeps your place in the current book — the device re-flows pages around the byte you're reading and lands on the page that contains it.</p>"
    "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>"
    "<div class='grid cols-2'>"
    "<div><label for='font'>Font size</label><select id='font' name='font'>"
    "<option value='8'";  out += sel8;  out += ">8px &mdash; tiny</option>"
    "<option value='10'"; out += sel10; out += ">10px &mdash; small</option>"
    "<option value='12'"; out += sel12; out += ">12px &mdash; medium</option>"
    "<option value='14'"; out += sel14; out += ">14px &mdash; large</option>"
    "</select><div class='hint'>Controls how many lines fit on each page.</div></div>"
    "<div><label for='family'>Font family</label><select id='family' name='family'>"
    "<option value='helv'"; out += famH; out += ">Helvetica</option>"
    "<option value='dys'";  out += famD; out += ">OpenDyslexic</option>"
    "</select><div class='hint'>OpenDyslexic uses heavier letter shapes designed for easier scanning.</div></div>"
    "<div><label for='sleep'>Sleep after</label><select id='sleep' name='sleep'>"
    "<option value='30'";   out += ss30;   out += ">30 seconds</option>"
    "<option value='60'";   out += ss60;   out += ">1 minute</option>"
    "<option value='120'";  out += ss120;  out += ">2 minutes</option>"
    "<option value='300'";  out += ss300;  out += ">5 minutes</option>"
    "<option value='600'";  out += ss600;  out += ">10 minutes</option>"
    "<option value='1800'"; out += ss1800; out += ">30 minutes</option>"
    "</select><div class='hint'>Auto-sleep keeps battery draw low while idle.</div></div>"
    "<div><label for='lgap'>Line spacing</label><select id='lgap' name='lgap'>"
    "<option value='0'"; out += lg0; out += ">0 px &mdash; compact</option>"
    "<option value='1'"; out += lg1; out += ">1 px &mdash; normal</option>"
    "<option value='2'"; out += lg2; out += ">2 px &mdash; relaxed</option>"
    "<option value='3'"; out += lg3; out += ">3 px &mdash; loose</option>"
    "</select><div class='hint'>A small change here can make text much easier to scan.</div></div>"
    "<div class='full' style='grid-column:1/-1'><label style='display:flex;gap:10px;align-items:center;font-weight:600'>"
    "<input type='checkbox' name='bionic' value='1'"; out += bChecked; out += "><span>Bionic reading</span></label>"
    "<div class='hint'>Bolds the leading characters of each word to help your eyes anchor.</div></div>"
    "</div>"
    "<div class='actions' style='margin-top:24px'><button type='submit'>Save settings</button>"
    "<span class='muted'>Changes apply to the next page render.</span></div>"
    "</form></div>"

    "<div class='card'><h2>Screensaver</h2>"
    "<p class='muted'>Manage the image (or multi-image rotation) shown on the e-ink when the device sleeps.</p>"
    "<div class='actions' style='margin-top:8px'>"
    "<a class='btn' href='/screensavers'>Open screensaver editor</a>"
    "<span class='muted'>Includes an in-browser bitmap editor and up to 8 rotation slots.</span>"
    "</div></div>";

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

// Apply pending form changes. Returns true if any layout-affecting setting
// (font size, family, line gap, bionic) was modified — caller uses this to
// decide whether to remap the reader's byte-offset cursor afterwards.
static bool applySettingsForm() {
  bool layoutChanged = false;

  // Font::setBodySize / setLineGap / setFamily / Sleep::setIdleTimeout all
  // clamp + validate internally — no inline guards needed here.
  if (server.hasArg("font")) {
    int fs = server.arg("font").toInt();
    if (fs != Font::currentBodySize()) { Font::setBodySize(fs); layoutChanged = true; }
  }
  if (server.hasArg("family")) {
    String f = server.arg("family");
    Font::Family want = (f == "dys") ? Font::Family::OpenDyslexic : Font::Family::Helvetica;
    if (want != Font::currentFamily()) { Font::setFamily(want); layoutChanged = true; }
  }
  if (server.hasArg("sleep")) {
    int ss = server.arg("sleep").toInt();
    if (ss != Sleep::idleTimeoutSecs()) Sleep::setIdleTimeout(ss);
  }
  if (server.hasArg("lgap")) {
    int lg = server.arg("lgap").toInt();
    if (lg != Font::currentLineGap()) { Font::setLineGap(lg); layoutChanged = true; }
  }
  // Checkbox is absent from the POST when unchecked.
  bool wantBionic = server.hasArg("bionic");
  if (wantBionic != Font::bionicEnabled()) {
    Font::setBionic(wantBionic);
    layoutChanged = true;
  }
  return layoutChanged;
}

static void handleSettingsPost() {
  // Snapshot the reader's current byte offset before applying changes, so
  // we can re-land on the same byte under the new layout. The on-disk page
  // cache self-invalidates via its layout stamp (see page_cache.cpp), so the
  // page table will rebuild lazily — no explicit cache wipe needed.
  bool readerActive =
      (g_currentScreen == &g_readerScreen) &&
      g_bookview.book.isOpen();
  uint32_t savedByte = 0;
  if (readerActive
      && g_bookview.cursor.pageIndex >= 0
      && g_bookview.cursor.pageIndex < g_bookview.pages.count) {
    savedByte = g_bookview.pages.offsets[g_bookview.cursor.pageIndex];
  }

  bool layoutChanged = applySettingsForm();

  // Layout shifted under our feet — the in-memory page table no longer
  // matches the active layout. Reset it (the on-disk cache, if any, will
  // be rejected on the next loadPageOffsetCacheForBook because of the
  // layout-stamp mismatch) and re-locate the cursor on the page that
  // contains the saved byte offset. Then render so the user sees the new
  // layout immediately when they return to the device.
  if (readerActive && layoutChanged) {
    g_bookview.pages.count = 1;
    g_bookview.pages.offsets[0] = 0;
    g_bookview.pages.eofReached = false;
    int newPage = findPageForOffset(savedByte);
    if (newPage < 0) newPage = 0;
    g_bookview.cursor.pageIndex = newPage;
    g_bookview.cursor.pageTurnsSinceFull = 0;

    PreferencesStore kv(prefs);
    saveSavedPage(kv, g_bookview.book.key(), newPage);
    // Offset is unchanged — no need to rewrite the canonical position.

    renderCurrentPage();
  }

  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

static void handleDeleteSleepImg() {
  if (FS.exists("/sleep.bin")) FS.remove("/sleep.bin");
  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

void registerSettingsRoutes() {
  server.on("/settings",  HTTP_GET,  handleSettings);
  server.on("/settings",  HTTP_POST, handleSettingsPost);
  server.on("/del-sleep", HTTP_POST, handleDeleteSleepImg);
}
