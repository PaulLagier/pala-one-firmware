#include "src/web/settings.h"

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h"             // prefKeyForBook
#include "src/storage/book_metadata.h"
#include "src/storage/page_cache.h"       // deletePageCacheForBook
#include "src/storage/preferences_store.h"
#include "src/ui/font.h"
#include "src/ui/header_title.h"
#include "src/ui/reader.h"                // g_bookview, findPageForOffset, renderCurrentPage
#include "src/ui/reader_actions.h"        // ButtonAction + Gestures
#include "src/ui/screens/reader_screen.h" // g_readerScreen — active-reader check
#include "src/ui/lock.h"
#include "src/ui/sleep.h"
#include "src/web/chrome.h"

// ----------------------------------------------------------------------------
//  HTML escaping for user-supplied text rendered in attributes
// ----------------------------------------------------------------------------
static String htmlAttrEscape(const char* raw) {
  String out;
  out.reserve(strlen(raw) + 8);
  while (*raw) {
    switch (*raw) {
      case '&':  out += "&amp;";  break;
      case '"':  out += "&quot;"; break;
      case '\'': out += "&#39;";  break;
      case '<':  out += "&lt;";   break;
      case '>':  out += "&gt;";   break;
      default:   out += *raw;     break;
    }
    raw++;
  }
  return out;
}

// ----------------------------------------------------------------------------
//  Helpers for the remappable-button section
// ----------------------------------------------------------------------------
static void appendActionOption(String& out, int val, const char* label, int current) {
  out += "<option value='";
  out += val;
  out += "'";
  if (val == current) out += " selected";
  out += ">";
  out += label;
  out += "</option>";
}

static void appendActionSelect(String& out, const char* nameId, const char* label, int current) {
  out += "<div><label for='";
  out += nameId;
  out += "'>";
  out += label;
  out += "</label><select id='";
  out += nameId;
  out += "' name='";
  out += nameId;
  out += "'>";
  appendActionOption(out, ACTION_NONE,     D_WEB_BUTTONS_ACTION_NONE,     current);
  appendActionOption(out, ACTION_BOOKMARK, D_WEB_BUTTONS_ACTION_BOOKMARK, current);
  appendActionOption(out, ACTION_LOCK,     D_WEB_BUTTONS_ACTION_LOCK,     current);
  appendActionOption(out, ACTION_MENU,     D_WEB_BUTTONS_ACTION_MENU,     current);
  out += "</select></div>";
}

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
    D_WEB_SETTINGS_TITLE,
    D_WEB_SETTINGS_SUBTITLE_PREFIX FW_VERSION D_WEB_SETTINGS_SUBTITLE_SUFFIX,
    "<a href='/'>" D_WEB_NAV_HOME "</a><a href='/screensavers'>" D_WEB_NAV_SCREENSAVER "</a>"
  );
  out.reserve(out.length() + 4500);

  // Device personalization card — separate form, no layout-remap interaction.
  out += "<div class='card'><h2>" D_WEB_DEVICE_HEADING "</h2>";
  out += "<p class='muted'>" D_WEB_DEVICE_INTRO "</p>";
  out += "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>";
  out += "<div><label for='hdr'>" D_WEB_HEADER_TITLE_LABEL "</label>";
  out += "<input type='text' id='hdr' name='hdr' maxlength='31' value='";
  out += htmlAttrEscape(HeaderTitle::current());
  out += "' placeholder='" LIB_HEADER_TITLE "'>";
  out += "<div class='hint'>" D_WEB_HEADER_TITLE_HINT "</div></div>";
  out += "<label style='display:flex;gap:8px;align-items:center;margin-top:10px;cursor:pointer'>";
  out += "<input type='checkbox' name='hdr_rst' value='1' style='width:auto'>";
  out += "<span>" D_WEB_HEADER_TITLE_RESET "</span></label>";
  out += "<div class='actions' style='margin-top:14px'><button type='submit'>" D_WEB_SAVE_SETTINGS_BUTTON "</button></div>";
  out += "</form></div>";

  out +=
    "<div class='card'><h2>" D_WEB_READING_HEADING "</h2>"
    "<p class='muted'>" D_WEB_READING_INTRO "</p>"
    "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>"
    "<div class='grid cols-2'>"
    "<div><label for='font'>" D_WEB_FONT_SIZE_LABEL "</label><select id='font' name='font'>"
    "<option value='8'";  out += sel8;  out += ">" D_WEB_FONT_SIZE_8  "</option>"
    "<option value='10'"; out += sel10; out += ">" D_WEB_FONT_SIZE_10 "</option>"
    "<option value='12'"; out += sel12; out += ">" D_WEB_FONT_SIZE_12 "</option>"
    "<option value='14'"; out += sel14; out += ">" D_WEB_FONT_SIZE_14 "</option>"
    "</select><div class='hint'>" D_WEB_FONT_SIZE_HINT "</div></div>"
    "<div><label for='family'>" D_WEB_FONT_FAMILY_LABEL "</label><select id='family' name='family'>"
    "<option value='helv'"; out += famH; out += ">" D_WEB_FONT_FAMILY_HELVETICA "</option>"
    "<option value='dys'";  out += famD; out += ">" D_WEB_FONT_FAMILY_DYSLEXIC  "</option>"
    "</select><div class='hint'>" D_WEB_FONT_FAMILY_HINT "</div></div>"
    "<div><label for='sleep'>" D_WEB_SLEEP_AFTER_LABEL "</label><select id='sleep' name='sleep'>"
    "<option value='30'";   out += ss30;   out += ">" D_WEB_SLEEP_30S "</option>"
    "<option value='60'";   out += ss60;   out += ">" D_WEB_SLEEP_1M  "</option>"
    "<option value='120'";  out += ss120;  out += ">" D_WEB_SLEEP_2M  "</option>"
    "<option value='300'";  out += ss300;  out += ">" D_WEB_SLEEP_5M  "</option>"
    "<option value='600'";  out += ss600;  out += ">" D_WEB_SLEEP_10M "</option>"
    "<option value='1800'"; out += ss1800; out += ">" D_WEB_SLEEP_30M "</option>"
    "</select><div class='hint'>" D_WEB_SLEEP_HINT "</div></div>"
    "<div><label for='lgap'>" D_WEB_LINE_SPACING_LABEL "</label><select id='lgap' name='lgap'>"
    "<option value='0'"; out += lg0; out += ">" D_WEB_LINE_SPACING_0 "</option>"
    "<option value='1'"; out += lg1; out += ">" D_WEB_LINE_SPACING_1 "</option>"
    "<option value='2'"; out += lg2; out += ">" D_WEB_LINE_SPACING_2 "</option>"
    "<option value='3'"; out += lg3; out += ">" D_WEB_LINE_SPACING_3 "</option>"
    "</select><div class='hint'>" D_WEB_LINE_SPACING_HINT "</div></div>"
    "<div class='full' style='grid-column:1/-1'><label style='display:flex;gap:10px;align-items:center;font-weight:600'>"
    "<input type='checkbox' name='bionic' value='1'"; out += bChecked; out += "><span>" D_WEB_BIONIC_LABEL "</span></label>"
    "<div class='hint'>" D_WEB_BIONIC_HINT "</div></div>"
    "</div>"
    "<div style='margin-top:14px'>"
    "<label style='display:flex;align-items:center;gap:8px;font-weight:600;cursor:pointer'>"
    "<input type='checkbox' name='noscr' id='noscr' style='width:auto'";
  if (Sleep::noScreensaver()) out += " checked";
  out +=
    "><span>" D_WEB_NO_SCREENSAVER_LABEL "</span></label>"
    "<div class='hint'>" D_WEB_NO_SCREENSAVER_HINT "</div></div>"
    "<div style='margin-top:10px'>"
    "<label style='display:flex;align-items:center;gap:8px;font-weight:600;cursor:pointer'>"
    "<input type='checkbox' name='lckslp' id='lckslp' style='width:auto'";
  if (Sleep::lockOnSleep()) out += " checked";
  out +=
    "><span>" D_WEB_LOCK_ON_SLEEP_LABEL "</span></label>"
    "<div class='hint'>" D_WEB_LOCK_ON_SLEEP_HINT "</div></div>"
    "<input type='hidden' name='noscr_form' value='1'>"
    "<div class='actions' style='margin-top:24px'><button type='submit'>" D_WEB_SAVE_SETTINGS_BUTTON "</button>"
    "<span class='muted'>" D_WEB_SETTINGS_APPLY_HINT "</span></div>"
    "</form></div>";

  // Buttons card — submitted as a separate form so the gesture bindings
  // don't share POST state with the reading-form's reader-cursor remap.
  out += "<div class='card'><h2>" D_WEB_BUTTONS_HEADING "</h2>";
  out += "<p class='muted'>" D_WEB_BUTTONS_HINT "</p>";
  out += "<form method='POST' action='/settings' accept-charset='UTF-8'><div class='grid cols-2'>";
  appendActionSelect(out, "btnL",  D_WEB_BUTTONS_LONG,       (int)Gestures::actionLong());
  appendActionSelect(out, "btnXL", D_WEB_BUTTONS_EXTRA_LONG, (int)Gestures::actionExtraLong());
  appendActionSelect(out, "btnCH", D_WEB_BUTTONS_CLICK_HOLD, (int)Gestures::actionClickHold());
  out += "</div><div class='actions' style='margin-top:24px'><button type='submit'>" D_WEB_BUTTONS_SAVE "</button>";
  out += "<span class='muted'>" D_WEB_BUTTONS_LOCK_HINT "</span>";
  out += "</div></form></div>";

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

  // noscr_form is a hidden sentinel always present when the settings form is
  // submitted, which lets us distinguish an explicit unchecked checkbox from a
  // POST that didn't come from the settings form at all.
  if (server.hasArg("noscr_form")) {
    bool ns = server.hasArg("noscr");
    if (ns != Sleep::noScreensaver()) Sleep::setNoScreensaver(ns);
    bool ls = server.hasArg("lckslp");
    if (ls != Sleep::lockOnSleep()) Sleep::setLockOnSleep(ls);
  }

  // Header title — its own form card.
  if (server.hasArg("hdr")) {
    if (server.hasArg("hdr_rst")) {
      HeaderTitle::resetToDefault();
    } else {
      String hdr = server.arg("hdr");
      hdr.trim();
      HeaderTitle::set(hdr.c_str());
    }
  }

  // Gesture bindings — `setAction*` clamp internally, but we still check
  // `hasArg` because the page submits this section as a separate form
  // (so a Reading POST won't carry these keys at all).
  if (server.hasArg("btnL")) {
    Gestures::setActionLong((ButtonAction)server.arg("btnL").toInt());
  }
  if (server.hasArg("btnXL")) {
    Gestures::setActionExtraLong((ButtonAction)server.arg("btnXL").toInt());
  }
  if (server.hasArg("btnCH")) {
    Gestures::setActionClickHold((ButtonAction)server.arg("btnCH").toInt());
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
