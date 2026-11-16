#include "src/storage/navigation_profile.h"
#include "src/lang/join.h"
#include "src/web/settings.h"
#include "src/web/settings_controls.h"
#include "src/web/display_settings.h"
#include "src/web/reading_settings.h"
#include "src/ui/statusbar.h"

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h" // prefKeyForBook
#include "src/storage/book_metadata.h"
#include "src/storage/page_cache.h" // deletePageCacheForBook
#include "src/storage/preferences_store.h"
#include "src/storage/library_menu_order.h"
#include "src/ui/font.h"
#include "src/ui/widgets.h"
#include "src/ui/header_title.h"
#include "src/ui/reader.h"                // g_bookview, findPageForOffset, renderCurrentPage
#include "src/ui/reader_actions.h"        // ButtonAction + Gestures
#include "src/ui/screens/reader_screen.h" // g_readerScreen — active-reader check
#include "src/storage/click_timings.h"
#include "src/storage/wifi_creds.h"
#include "src/ui/icons.h"
#include "src/ui/lock.h"
#include "src/ui/bookmark_settings.h"
#include "src/ui/sleep.h"
#include "src/web/chrome.h"
#include "src/ui/screen_settings.h"
#include "src/pure/library_nav.h"

// ----------------------------------------------------------------------------
//  HTML escaping for user-supplied text rendered in attributes
// ----------------------------------------------------------------------------
static String htmlAttrEscape(const char *raw) {
  String out;
  out.reserve(strlen(raw) + 8);
  while (*raw) {
    switch (*raw) {
    case '&':
      out += "&amp;";
      break;
    case '"':
      out += "&quot;";
      break;
    case '\'':
      out += "&#39;";
      break;
    case '<':
      out += "&lt;";
      break;
    case '>':
      out += "&gt;";
      break;
    default:
      out += *raw;
      break;
    }
    raw++;
  }
  return out;
}

// ----------------------------------------------------------------------------
//  Helpers for the remappable-button section
// ----------------------------------------------------------------------------
static void appendActionOption(String &out, int val, const char *label, int current) {
  out += "<option value='";
  out += val;
  out += "'";
  if (val == current)
    out += " selected";
  out += ">";
  out += label;
  out += "</option>";
}

static void appendActionSelect(String &out, const char *nameId, const char *label, int current) {
  out += "<div><label for='";
  out += nameId;
  out += "'>";
  out += label;
  out += "</label><select id='";
  out += nameId;
  out += "' name='";
  out += nameId;
  out += "'>";
  appendActionOption(out, ACTION_NONE, D_WEB_BUTTONS_ACTION_NONE, current);
  appendActionOption(out, ACTION_NEXT, D_WEB_BUTTONS_ACTION_NEXT, current);
  appendActionOption(out, ACTION_PREV, D_WEB_BUTTONS_ACTION_PREV, current);
  appendActionOption(out, ACTION_MENU, D_WEB_BUTTONS_ACTION_MENU, current);
  appendActionOption(out, ACTION_LOCK, D_WEB_BUTTONS_ACTION_LOCK, current);
  appendActionOption(out, ACTION_HOME, D_WEB_BUTTONS_ACTION_HOME, current);
  appendActionOption(out, ACTION_BOOKMARK, D_WEB_BUTTONS_ACTION_BOOKMARK, current);
  appendActionOption(out, ACTION_ROTATE, D_WEB_BUTTONS_ACTION_ROTATE, current);
  out += "</select></div>";
}

struct TimingUiField {
  const char *nameId;
  const char *label;
  const char *hint;
  const char *resetName;
};

// Resolve labels per request; catalog pool pointers expire on a language switch.
static TimingUiField timingUiField(size_t index) {
  const TimingUiField fields[] = {
      {"tim_gap", D_WEB_TIMINGS_GAP_LABEL, D_WEB_TIMINGS_GAP_HINT, "tim_gap_rst"},
      {"tim_seq", D_WEB_TIMINGS_SEQUENCE_LABEL, D_WEB_TIMINGS_SEQUENCE_HINT, "tim_seq_rst"},
      {"tim_long", D_WEB_TIMINGS_LONG_LABEL, D_WEB_TIMINGS_LONG_HINT, "tim_long_rst"},
      {"tim_vlong", D_WEB_TIMINGS_VLONG_LABEL, D_WEB_TIMINGS_VLONG_HINT, "tim_vlong_rst"},
      {"tim_dbnc", D_WEB_TIMINGS_DEBOUNCE_LABEL, D_WEB_TIMINGS_DEBOUNCE_HINT, "tim_dbnc_rst"},
  };
  return fields[index];
}

static void appendTimingField(String &out, const TimingUiField &uiField,
                              const ClickTimings::TimingSettingSpec &timing);

static constexpr int kLibraryMenuHidden = -1;

static const char *libraryMenuLabel(int value) {
  switch (value) {
  case LIB_ENTRY_BOOKMARKS:
    return D_MENU_BOOKMARKS;
  case LIB_ENTRY_LIST:
    return D_MENU_LIST;
  case LIB_ENTRY_APPS:
    return D_MENU_APPS;
  case LIB_ENTRY_STATISTICS:
    return D_MENU_STATISTICS;
  case LIB_ENTRY_ABOUT:
    return D_MENU_DEVICE;
  case LIB_ENTRY_UPDATE:
    return D_MENU_UPDATE;
  case LIB_ENTRY_UPLOAD:
    return D_MENU_UPLOAD;
  case LIB_ENTRY_SETTINGS:
    return D_MENU_SETTINGS;
  default:
    return D_WEB_LIBRARY_ORDER_HIDDEN;
  }
}

static void appendLibraryMenuOption(String &out, int val, const char *label, int current) {
  out += "<option value='";
  out += val;
  out += "'";
  if (val == current)
    out += " selected";
  out += ">";
  out += label;
  out += "</option>";
}

static void appendLibraryMenuSelect(String &out, int slotIndex, int current) {
  String nameId = "lib";
  nameId += slotIndex;
  out += "<div><label for='";
  out += nameId;
  out += "'>";
  out += D_WEB_LIBRARY_ORDER_SLOT_LABEL;
  out += " ";
  out += slotIndex + 1;
  out += "</label><select id='";
  out += nameId;
  out += "' name='";
  out += nameId;
  out += "'>";
  appendLibraryMenuOption(out, kLibraryMenuHidden, D_WEB_LIBRARY_ORDER_HIDDEN, current);
  appendLibraryMenuOption(out, LIB_ENTRY_BOOKMARKS, libraryMenuLabel(LIB_ENTRY_BOOKMARKS), current);
  appendLibraryMenuOption(out, LIB_ENTRY_LIST, libraryMenuLabel(LIB_ENTRY_LIST), current);
  appendLibraryMenuOption(out, LIB_ENTRY_APPS, libraryMenuLabel(LIB_ENTRY_APPS), current);
  appendLibraryMenuOption(out, LIB_ENTRY_STATISTICS, libraryMenuLabel(LIB_ENTRY_STATISTICS),
                          current);
  appendLibraryMenuOption(out, LIB_ENTRY_ABOUT, libraryMenuLabel(LIB_ENTRY_ABOUT), current);
  appendLibraryMenuOption(out, LIB_ENTRY_UPLOAD, libraryMenuLabel(LIB_ENTRY_UPLOAD), current);
  appendLibraryMenuOption(out, LIB_ENTRY_UPDATE, libraryMenuLabel(LIB_ENTRY_UPDATE), current);
  appendLibraryMenuOption(out, LIB_ENTRY_SETTINGS, libraryMenuLabel(LIB_ENTRY_SETTINGS), current);
  out += "</select></div>";
}

static void handleLibraryMenuOrderPost() {
  if (!server.hasArg("lib_menu_form"))
    return;

  if (server.hasArg("lib_menu_reset")) {
    LibraryMenuOrder::resetToDefaults();
    return;
  }

  LibraryEntryType entries[LibraryMenuOrder::kMaxSystemEntries];
  int entryCount = 0;

  for (int i = 0; i < LibraryMenuOrder::kMaxSystemEntries; i++) {
    String key = "lib";
    key += i;
    if (!server.hasArg(key))
      continue;

    int value = server.arg(key).toInt();
    if (value == kLibraryMenuHidden || !isValidLibEntry((LibraryEntryType)value))
      continue;

    LibraryEntryType type = (LibraryEntryType)value;
    bool duplicate = false;
    for (int j = 0; j < entryCount; j++) {
      if (entries[j] == type) {
        duplicate = true;
        break;
      }
    }
    if (duplicate)
      continue;
    entries[entryCount++] = type;
  }

  LibraryMenuOrder::setEntries(entries, entryCount);
}

static void handleSettings() {
  int curFont = Font::currentBodySize();

  int curSleep = Sleep::idleTimeoutSecs();

  int curGap = Font::currentLineGap();

  Font::Family curFam = Font::currentFamily();

  bool curBionic = Font::bionicEnabled();
  String bChecked = curBionic ? " checked" : "";
  bool curHalfGaps = Font::halfParagraphGapsEnabled();
  String hgChecked = curHalfGaps ? " checked" : "";
  bool curMarkdown = Font::markdownEnabled();
  String mChecked = curMarkdown ? " checked" : "";

  String alertText;
  if (server.hasArg("alert")) {
    if (server.arg("alert") == "buttons_invalid") {
      alertText = D_WEB_MISSING_REQUIRED_BUTTON_MSG;
    }
  }

  String out = webPageStart(
      D_WEB_SETTINGS_TITLE,
      Language::join({D_WEB_SETTINGS_SUBTITLE_PREFIX, FW_VERSION, D_WEB_SETTINGS_SUBTITLE_SUFFIX})
          .c_str(),
      Language::join({"<a href='/'>", D_WEB_NAV_HOME, "</a><a href='/screensavers'>",
                      D_WEB_NAV_SCREENSAVER, "</a>"})
          .c_str());
  if (alertText.length() > 0) {
    out += "<script>window.onload=function(){alert(";
    out += webJsString(alertText.c_str());
    out += ");};</script>";
  }
  out.reserve(out.length() + 7200);

  // Language has its own form: selecting it never mutates reading or network settings.
  out += "<div class='card'><h2>";
  out += htmlEscape(D_LANGUAGE_LABEL);
  out += "</h2><form method='POST' action='/settings'><select name='language'>";
  for (size_t i = 0; i < Language::count(); ++i) {
    const char *code = Language::codeAt(i);
    out += "<option value='";
    out += code; // Generated, fixed two-letter ISO codes.
    out += "'";
    if (strcmp(code, Language::currentCode()) == 0)
      out += " selected";
    out += ">";
    out += htmlEscape(Language::nameAt(i));
    out += "</option>";
  }
  out += "</select><div class='actions'><button type='submit'>";
  out += htmlEscape(D_WEB_SAVE_SETTINGS_BUTTON);
  out += "</button></div></form></div>";

  // Separate form: changing menu visibility must not rewrite reader preferences.
  out += "<div class='card'><form method='POST' action='/settings'>";
  out += "<input type='hidden' name='menuForm' value='1'><label>";
  out += "<input type='checkbox' name='readingMenu' value='1'";
  if (NavigationProfile::readingFocused())
    out += " checked";
  out += "> ";
  out += htmlEscape(D_SETTINGS_READING_MENU);
  out += "</label><button type='submit'>";
  out += htmlEscape(D_WEB_SAVE_SETTINGS_BUTTON);
  out += "</button></form></div>";

  // Device personalization card — separate form, no layout-remap interaction.
  out += Language::join({"<div class='card'><h2>", D_WEB_DEVICE_HEADING, "</h2>"}).c_str();
  out += Language::join({"<p class='muted'>", D_WEB_DEVICE_INTRO, "</p>"}).c_str();
  out += "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>";
  out += Language::join({"<div><label for='hdr'>", D_WEB_HEADER_TITLE_LABEL, "</label>"}).c_str();
  out += "<input type='text' id='hdr' name='hdr' maxlength='31' value='";
  out += htmlAttrEscape(HeaderTitle::current());
  out += "' placeholder='" LIB_HEADER_TITLE "'>";
  out += Language::join({"<div class='hint'>", D_WEB_HEADER_TITLE_HINT, "</div></div>"}).c_str();
  out += "<label style='display:flex;gap:8px;align-items:center;margin-top:10px;cursor:pointer'>";
  out += "<input type='checkbox' name='hdr_rst' value='1' style='width:auto'>";
  out += Language::join({"<span>", D_WEB_HEADER_TITLE_RESET, "</span></label>"}).c_str();

  // Toggle for setting the inversed screen orientation
  out += "<label style='display:flex;gap:8px;align-items:center;margin-top:10px;cursor:pointer'>";
  out += "<input type='checkbox' name='flip_rot' value='1' style='width:auto'";
  out += ScreenSettings::isScreenFlipped() ? " checked" : "";
  out += ">";
  out += Language::join({"<span>", D_WEB_FLIP_SCREEN, "</span></label>"}).c_str();

  out += Language::join(
             {"<span class='muted' style='display:inline'>", D_WEB_SETTINGS_APPLY_HINT, "</span>"})
             .c_str();

  out += Language::join({"<div class='actions' style='margin-top:14px'><button type='submit'>",
                         D_WEB_SAVE_SETTINGS_BUTTON, "</button></div>"})
             .c_str();
  out += "</form></div>";

  // Header status icons — own form (icons_form sentinel) so an unchecked box
  // here is never confused with a POST from one of the other cards.
  out += Language::join({"<div class='card'><h2>", D_WEB_ICONS_HEADING, "</h2>"}).c_str();
  out += Language::join({"<p class='muted'>", D_WEB_ICONS_INTRO, "</p>"}).c_str();
  out += "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>";
  out += "<label style='display:flex;gap:8px;align-items:center;margin-top:10px;cursor:pointer'>";
  out += "<input type='checkbox' name='ico_sleep' value='1' style='width:auto'";
  out += Icons::sleepIconEnabled() ? " checked" : "";
  out += Language::join({"><span>", D_WEB_ICON_SLEEP_LABEL, "</span></label>"}).c_str();
  out += Language::join({"<div class='hint'>", D_WEB_ICON_SLEEP_HINT, "</div>"}).c_str();
  out += "<input type='hidden' name='icons_form' value='1'>";

  out += "<label style='display:flex;gap:8px;align-items:center;margin-top:10px;cursor:pointer'>";
  out += "<input type='checkbox' name='batt_ind' value='1' style='width:auto'";
  out += !ScreenSettings::batteryIndicatorsEnabled() ? " checked" : "";
  out += ">";
  out += Language::join({"<span>", D_WEB_BATTERY_INDICATORS_LABEL, "</span></label>"}).c_str();
  out += Language::join({"<div class='hint'>", D_WEB_BATTERY_INDICATORS_HINT, "</div>"}).c_str();
  out += Language::join({"<div class='actions' style='margin-top:14px'><button type='submit'>",
                         D_WEB_SAVE_SETTINGS_BUTTON, "</button></div>"})
             .c_str();
  out += "</form></div>";

  out +=
      Language::join(
          {"<div class='card'><h2>", D_WEB_LIBRARY_ORDER_HEADING, "</h2>", "<p class='muted'>",
           D_WEB_LIBRARY_ORDER_INTRO, "</p>", "<p class='muted'>", D_WEB_LIBRARY_ORDER_REQUIRED,
           "</p>",
           "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>",
           "<div class='grid cols-2'>"})
          .c_str();
  LibraryEntryType currentOrder[LibraryMenuOrder::kMaxSystemEntries];
  int currentCount =
      LibraryMenuOrder::copyEntries(currentOrder, LibraryMenuOrder::kMaxSystemEntries);
  for (int i = 0; i < LibraryMenuOrder::kMaxSystemEntries; i++) {
    int current = (i < currentCount) ? (int)currentOrder[i] : kLibraryMenuHidden;
    appendLibraryMenuSelect(out, i, current);
  }
  out += Language::join({"</div>", "<input type='hidden' name='lib_menu_form' value='1'>",
                         "<div class='actions' style='margin-top:14px'>", "<button type='submit'>",
                         D_WEB_SAVE_SETTINGS_BUTTON, "</button>",
                         "<button type='submit' name='lib_menu_reset' value='1'>",
                         D_WEB_LIBRARY_ORDER_RESET, "</button>", "<span class='muted'>",
                         D_WEB_LIBRARY_ORDER_HINT, "</span>", "</div>", "</form></div>"})
             .c_str();

  out += Language::join({"<div class='card'><h2>", D_WEB_READING_HEADING,
                         "</h2><p class='muted'>", D_WEB_READING_INTRO, "</p>",
                         "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>",
                         "<div class='grid cols-2'>"});
  appendSettingsSelect(out, "font", D_WEB_FONT_SIZE_LABEL, D_WEB_FONT_SIZE_HINT,
      {{"8", D_WEB_FONT_SIZE_8}, {"10", D_WEB_FONT_SIZE_10},
       {"12", D_WEB_FONT_SIZE_12}, {"14", D_WEB_FONT_SIZE_14}}, String(curFont));
  appendSettingsSelect(out, "family", D_WEB_FONT_FAMILY_LABEL, D_WEB_FONT_FAMILY_HINT,
      {{"helv", D_WEB_FONT_FAMILY_HELVETICA}, {"dys", D_WEB_FONT_FAMILY_DYSLEXIC}},
      curFam == Font::Family::OpenDyslexic ? "dys" : "helv");
  appendSettingsSelect(out, "sleep", D_WEB_SLEEP_AFTER_LABEL, D_WEB_SLEEP_HINT,
      {{"30", D_WEB_SLEEP_30S}, {"60", D_WEB_SLEEP_1M}, {"120", D_WEB_SLEEP_2M},
       {"300", D_WEB_SLEEP_5M}, {"600", D_WEB_SLEEP_10M}, {"1800", D_WEB_SLEEP_30M}},
      String(curSleep), String(curSleep) + " s");
  appendSettingsSelect(out, "lgap", D_WEB_LINE_SPACING_LABEL, D_WEB_LINE_SPACING_HINT,
      {{"0", D_WEB_LINE_SPACING_0}, {"1", D_WEB_LINE_SPACING_1},
       {"2", D_WEB_LINE_SPACING_2}, {"3", D_WEB_LINE_SPACING_3}},
      String(curGap), String(curGap) + " px");
  out += "<div class='full' style='grid-column:1/-1'><label style='display:flex;gap:10px;align-items:center;font-weight:600'>";
  out += "<input type='checkbox' name='bionic' value='1'";
  out += bChecked;
  out += Language::join({"><span>", D_WEB_BIONIC_LABEL, "</span></label>", "<div class='hint'>",
                         D_WEB_BIONIC_HINT, "</div></div>",
                         "<div class='full' style='grid-column:1/-1'><label "
                         "style='display:flex;gap:10px;align-items:center;font-weight:600'>",
                         "<input type='checkbox' name='hgap' value='1'"})
             .c_str();
  out += mChecked;
  out += Language::join({"><span>", D_WEB_MARKDOWN_LABEL, "</span></label>", "<div class='hint'>",
                         D_WEB_MARKDOWN_HINT, "</div></div>",
                         "<div class='full' style='grid-column:1/-1'><label "
                         "style='display:flex;gap:10px;align-items:center;font-weight:600'>",
                         "<input type='checkbox' name='hgap' value='1'"})
             .c_str();
  out += hgChecked;
  out +=
      Language::join(
          {"><span>", D_WEB_PARA_GAP_LABEL, "</span></label>", "<div class='hint'>",
           D_WEB_PARA_GAP_HINT, "</div></div>", "</div>", "<div style='margin-top:14px'>",
           "<label style='display:flex;align-items:center;gap:8px;font-weight:600;cursor:pointer'>",
           "<input type='checkbox' name='noscr' id='noscr' style='width:auto'"})
          .c_str();
  if (Sleep::noScreensaver())
    out += " checked";
  out +=
      Language::join(
          {"><span>", D_WEB_NO_SCREENSAVER_LABEL, "</span></label>", "<div class='hint'>",
           D_WEB_NO_SCREENSAVER_HINT, "</div></div>", "<div style='margin-top:10px'>",
           "<label style='display:flex;align-items:center;gap:8px;font-weight:600;cursor:pointer'>",
           "<input type='checkbox' name='lckslp' id='lckslp' style='width:auto'"})
          .c_str();
  if (Sleep::lockOnSleep())
    out += " checked";
  out += Language::join({"><span>", D_WEB_LOCK_ON_SLEEP_LABEL, "</span></label>",
                         "<div class='hint'>", D_WEB_LOCK_ON_SLEEP_HINT, "</div></div>",
                         "<input type='hidden' name='noscr_form' value='1'>",
                         "<div class='actions' style='margin-top:24px'><button type='submit'>",
                         D_WEB_SAVE_SETTINGS_BUTTON, "</button>", "<span class='muted'>",
                         D_WEB_SETTINGS_APPLY_HINT, "</span></div>", "</form></div>"})
             .c_str();

  // Buttons card — submitted as a separate form so the gesture bindings
  // don't share POST state with the reading-form's reader-cursor remap.
  out += Language::join({"<div class='card'><h2>", D_WEB_BUTTONS_HEADING, "</h2>"}).c_str();

  out += Language::join({"<p class='muted'>", D_WEB_BUTTONS_HINT, "</p>"}).c_str();
  out += "<form method='POST' action='/settings' accept-charset='UTF-8'><div class='grid cols-2'>";
  appendActionSelect(out, "btnS", D_WEB_BUTTONS_SHORT, (int)Gestures::actionShort());
  appendActionSelect(out, "btnD", D_WEB_BUTTONS_DOUBLE, (int)Gestures::actionDouble());
  appendActionSelect(out, "btnT", D_WEB_BUTTONS_TRIPLE, (int)Gestures::actionTriple());
  appendActionSelect(out, "btnQ", D_WEB_BUTTONS_QUAD, (int)Gestures::actionQuad());
  appendActionSelect(out, "btnL", D_WEB_BUTTONS_LONG, (int)Gestures::actionLong());
  appendActionSelect(out, "btnXL", D_WEB_BUTTONS_EXTRA_LONG, (int)Gestures::actionExtraLong());
  appendActionSelect(out, "btnCH", D_WEB_BUTTONS_CLICK_HOLD, (int)Gestures::actionClickHold());

  String legacyControlsOn = Gestures::legacyControlsOn() ? "checked" : "";
  // Toggle for legacy controls
  out += "<label style='display:flex;gap:8px;align-items:center;margin-top:10px;cursor:pointer'>";
  out += "<input type='checkbox' name='legacy_cont' value='1' " + legacyControlsOn +
         " style='width:auto'>";
  out += Language::join({"<span> ", D_WEB_SETTINGS_LEGACY_CONTROLS, "</span></label>"}).c_str();
  out += Language::join({"<div class='hint'>", D_WEB_BUTTONS_LEGACY_HINT, "</div>"});
  out +=
      Language::join({"</div><div class='actions' style='margin-top:24px'><button type='submit'>",
                      D_WEB_BUTTONS_SAVE, "</button>"})
          .c_str();
  out += Language::join({"<span class='muted'>", D_WEB_BUTTONS_LOCK_HINT, "</span>"}).c_str();
  out += "</div></form></div>";

  out +=
      Language::join(
          {"<div class='card'><h2>", D_WEB_TIMINGS_HEADING, "</h2>", "<p class='muted'>",
           D_WEB_TIMINGS_INTRO, "</p>",
           "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>",
           "<div class='grid cols-2'>"})
          .c_str();
  const ClickTimings::TimingSettingSpec *timingSettings = ClickTimings::timingSettings();
  for (uint8_t i = 0; i < ClickTimings::timingSettingsCount(); i++) {
    appendTimingField(out, timingUiField(i), timingSettings[i]);
  }
  out += Language::join({"</div><input type='hidden' name='timings_form' value='1'>",
                         "<div class='actions' style='margin-top:24px'><button type='submit'>",
                         D_WEB_TIMINGS_SAVE, "</button>", "<span class='muted'>",
                         D_WEB_SETTINGS_APPLY_HINT, "</span></div>", "</form></div>"})
             .c_str();

  // Wi-Fi card — own form (wifi_form sentinel), posts back to the same
  // /settings endpoint as the other cards. Edits the single stored network
  // the upload screen joins.
  out +=
      Language::join(
          {"<div class='card'><h2>", D_WEB_WIFI_HEADING, "</h2>", "<p class='muted'>",
           D_WEB_WIFI_INTRO, "</p>",
           "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:12px'>",
           "<div class='grid cols-2'>", "<div><label for='wssid'>", D_WEB_WIFI_SSID_LABEL,
           "</label>", "<input type='text' id='wssid' name='wssid' maxlength='32' placeholder='",
           D_WEB_WIFI_SSID_PLACEHOLDER, "' value='"})
          .c_str();
  out += htmlAttrEscape(WifiCreds::ssid().c_str());
  out +=
      Language::join({"'></div>", "<div><label for='wpass'>", D_WEB_WIFI_PASSWORD_LABEL, "</label>",
                      "<div class='pwwrap'>",
                      "<input type='password' id='wpass' name='wpass' maxlength='64' placeholder='",
                      D_WEB_WIFI_PASSWORD_PLACEHOLDER, "' value='"})
          .c_str();
  // Never echo out the saved password verbatim for security reasons.
  // Keep the input empty; never render the stored password or a magic placeholder value.
  // Pasword area includes a crude SVG of an eye with a strike through that
  // acts as a button to allow it to be shown/hidden as appropriate.
  out += Language::join(
             {"'>", "<button type='button' class='pweye' onclick='palaTogglePw(this)' aria-label='",
              D_WEB_WIFI_SHOW_PASSWORD, "'>",
              "<svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor'",
              " stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>",
              "<path d='M1 12s4-7 11-7 11 7 11 7-4 7-11 7S1 12 1 12z'/><circle cx='12' cy='12' "
              "r='3'/>",
              "<line class='pwh' x1='3' y1='21' x2='21' y2='3'/></svg>", "</button></div></div>",
              "</div>", "<input type='hidden' name='wifi_form' value='1'>",
              "<div class='actions' style='margin-top:14px'><button type='submit'>",
              D_WEB_WIFI_SAVE_BUTTON, "</button>", "<span class='muted'>", D_WEB_WIFI_HINT,
              "</span></div>", "</form></div>"})
             .c_str();

  out += Language::join({"<div class='card'><h2>", D_WEB_READING_DISPLAY,
                         "</h2><form method='POST' action='/settings'>",
                         "<input type='hidden' name='display_form' value='1'>",
                         "<label for='statusbar'>", D_WEB_STATUSBAR_LABEL,
                         "</label><select id='statusbar' name='statusbar'>"});
  const char *modes[] = {D_STATUSBAR_MODE_FULL, D_STATUSBAR_MODE_MINIMAL, D_STATUSBAR_MODE_HIDDEN};
  for (int i = 0; i < 3; ++i) {
    out += "<option value='";
    out += i;
    out += "'";
    if (i == int(Statusbar::mode()))
      out += " selected";
    out += ">";
    out += modes[i];
    out += "</option>";
  }
  out += "</select><label style='display:flex;gap:8px;align-items:center;margin-top:12px'>";
  out += "<input type='checkbox' name='lock_icon' value='1' style='width:auto'";
  if (Lock::iconEnabled())
    out += " checked";
  out += Language::join({">", D_WEB_LOCK_ICON_LABEL, "</label><button type='submit'>",
                         D_WEB_SAVE_SETTINGS_BUTTON, "</button></form></div>"});

  out += "<div class='card'><form method='POST' action='/settings'>";
  out += "<input type='hidden' name='bookmark_form' value='1'>";
  out += "<label style='display:flex;gap:8px;align-items:center'>";
  out += "<input type='checkbox' name='bookmark_toggle' value='1' style='width:auto'";
  if (BookmarkSettings::toggleEnabled())
    out += " checked";
  out += Language::join({">", D_WEB_BOOKMARK_TOGGLE_LABEL, "</label><button type='submit'>",
                         D_WEB_SAVE_SETTINGS_BUTTON, "</button></form></div>"});

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void appendTimingField(String &out, const TimingUiField &uiField,
                              const ClickTimings::TimingSettingSpec &timing) {
  out += "<div><label for='";
  out += uiField.nameId;
  out += "'>";
  out += uiField.label;
  out += "</label>";
  out += "<div style='display:flex;gap:8px;align-items:end;margin-top:6px'>";
  out += "<input type='number' id='";
  out += uiField.nameId;
  out += "' name='";
  out += uiField.nameId;
  out += "' min='";
  out += timing.minValue();
  out += "' max='";
  out += timing.maxValue();
  out += "' step='1' value='";
  out += timing.current();
  out += "' style='max-width:120px'>";
  out += "<button type='submit' name='";
  out += uiField.resetName;
  out += "' value='1' class='btn secondary' formnovalidate>";
  out += D_WEB_TIMINGS_RESET;
  out += "</button></div>";
  out += "<div class='hint'>";
  out += uiField.hint;
  out += Language::join({" <span class='muted'>", D_WEB_TIMINGS_DEFAULT_PREFIX}).c_str();
  out += timing.defaultValue;
  out += " ms, min ";
  out += timing.minValue();
  out += " ms, max ";
  out += timing.maxValue();
  out += " ms</span></div></div>";
}

static bool validateRequiredActionsOnPost() {
  // Collect submitted remap values (only if the keys exist).
  // Your HTML always submits these fields, but an attacker/client may omit them.
  if (!server.hasArg("btnS") || !server.hasArg("btnD") || !server.hasArg("btnT") ||
      !server.hasArg("btnL") || !server.hasArg("btnXL") || !server.hasArg("btnCH")) {
    return false;
  }

  const int v[7] = {
      server.arg("btnS").toInt(),
      server.arg("btnD").toInt(),
      server.arg("btnT").toInt(),
      server.arg("btnL").toInt(),
      server.arg("btnXL").toInt(),
      server.arg("btnCH").toInt(),
      server.hasArg("btnQ") ? server.arg("btnQ").toInt() : (int)Gestures::actionQuad(),
  };

  bool hasNext = false, hasHome = false, hasOkMenu = false;
  for (int i = 0; i < 7; i++) {
    if (v[i] == (int)ACTION_NEXT)
      hasNext = true;
    else if (v[i] == (int)ACTION_HOME)
      hasHome = true;
    else if (v[i] == (int)ACTION_MENU)
      hasOkMenu = true;
  }

  return hasNext && hasHome && hasOkMenu;
}

static void applyTimingSettingsForm() {
  if (!server.hasArg("timings_form"))
    return;
  const ClickTimings::TimingSettingSpec *timingSettings = ClickTimings::timingSettings();
  for (uint8_t i = 0; i < ClickTimings::timingSettingsCount(); i++) {
    const TimingUiField &uiField = timingUiField(i);
    const ClickTimings::TimingSettingSpec &timing = timingSettings[i];
    if (server.hasArg(uiField.resetName)) {
      timing.reset();
    } else if (server.hasArg(uiField.nameId)) {
      // toInt() is signed; casting a negative straight to uint32_t wraps to
      // ~4.29e9, which clampU32 then pins to the field's max instead of its
      // min. Floor it first so bad input lands on the minimum.
      long raw = server.arg(uiField.nameId).toInt();
      timing.set(raw < 0 ? 0u : (uint32_t)raw);
    }
  }
}

static void handleSettingsPost() {
  if (handleDisplaySettingsPost())
    return;
  if (server.hasArg("menuForm")) {
    if (!NavigationProfile::setReadingFocused(server.hasArg("readingMenu") &&
                                              server.arg("readingMenu") == "1")) {
      server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_WRITE_FAILED);
      return;
    }
    forceNextMenuFrameFull();
    if (g_currentScreen)
      g_currentScreen->draw();
    server.sendHeader("Location", "/settings");
    server.send(303, "text/plain", "");
    return;
  }

  if (server.hasArg("language")) {
    const String code = server.arg("language");
    if (code.length() != 2 || !Language::setLanguage(code.c_str())) {
      server.send(400, "text/plain; charset=utf-8", D_LANGUAGE_INVALID);
      return;
    }
    // UI text changes; reading geometry and canonical book position do not.
    forceNextMenuFrameFull();
    if (g_currentScreen)
      g_currentScreen->draw();
    server.sendHeader("Location", "/settings");
    server.send(303, "text/plain", "");
    return;
  }

  // Snapshot the reader's current byte offset before applying changes, so
  // we can re-land on the same byte under the new layout. The on-disk page
  // cache self-invalidates via its layout stamp (see page_cache.cpp), so the
  // page table will rebuild lazily — no explicit cache wipe needed.
  bool readerActive = (g_currentScreen == &g_readerScreen) && g_bookview.book.isOpen();
  uint32_t savedByte = 0;
  if (readerActive && g_bookview.pages.contains(g_bookview.cursor.pageIndex)) {
    savedByte = g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);
  }

  bool layoutChanged = applyReadingSettingsForm();
  applyTimingSettingsForm();

  // Layout shifted under our feet — the in-memory page table no longer
  // matches the active layout. Reset it (the on-disk cache, if any, will
  // be rejected on the next loadPageOffsetCacheForBook because of the
  // layout-stamp mismatch) and re-locate the cursor on the page that
  // contains the saved byte offset. Then render so the user sees the new
  // layout immediately when they return to the device.
  if (readerActive && layoutChanged) {
    g_bookview.pages.reset();
    int newPage = findPageForOffset(savedByte);
    if (newPage < 0)
      newPage = 0;
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
    if (ns != Sleep::noScreensaver())
      Sleep::setNoScreensaver(ns);
    bool ls = server.hasArg("lckslp");
    if (ls != Sleep::lockOnSleep())
      Sleep::setLockOnSleep(ls);
  }

  // Same sentinel pattern as noscr_form above — without it, any other card's
  // POST would read the checkbox as absent and switch the icon off.
  if (server.hasArg("icons_form")) {
    bool si = server.hasArg("ico_sleep");
    if (si != Icons::sleepIconEnabled())
      Icons::setSleepIconEnabled(si);
    ScreenSettings::setBatteryIndicatorsEnabled(!server.hasArg("batt_ind"));
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
  if (server.hasArg("hdr")) {
    if (server.hasArg("flip_rot")) {
      ScreenSettings::setScreenRotation(true);
    } else {
      ScreenSettings::setScreenRotation(false);
    }
  }

  handleLibraryMenuOrderPost();

  // Gesture bindings — `setAction*` clamp internally, but we still check
  // `hasArg` because the page submits this section as a separate form
  // (so a Reading POST won't carry these keys at all).

  // Gesture bindings
  if (server.hasArg("btnS") || server.hasArg("btnD") || server.hasArg("btnT") ||
      server.hasArg("btnL") || server.hasArg("btnXL") || server.hasArg("btnCH") ||
      server.hasArg("btnQ")) {

    // Enforce required action contract.
    if (!validateRequiredActionsOnPost()) {
      server.sendHeader("Location", "/settings?alert=buttons_invalid");
      server.send(302, "text/plain", "");
      return;
    } else {
      if (server.hasArg("btnS")) {
        Gestures::setActionShort((ButtonAction)server.arg("btnS").toInt());
      }
      if (server.hasArg("btnD")) {
        Gestures::setActionDouble((ButtonAction)server.arg("btnD").toInt());
      }
      if (server.hasArg("btnT")) {
        Gestures::setActionTriple((ButtonAction)server.arg("btnT").toInt());
      }
      if (server.hasArg("btnQ")) {
        Gestures::setActionQuad((ButtonAction)server.arg("btnQ").toInt());
      }
      if (server.hasArg("btnL")) {
        Gestures::setActionLong((ButtonAction)server.arg("btnL").toInt());
      }
      if (server.hasArg("btnXL")) {
        Gestures::setActionExtraLong((ButtonAction)server.arg("btnXL").toInt());
      }
      if (server.hasArg("btnCH")) {
        Gestures::setActionClickHold((ButtonAction)server.arg("btnCH").toInt());
      }

      Gestures::setLegacyControls(server.hasArg("legacy_cont"));
    }
  }

  // Wi-Fi — its own form, guarded by wifi_form sentinel, so a Reading/Device/Buttons
  // POST won't accidentally wipe the stored network via absent fields.
  // A blank SSID forgets the stored network.
  if (server.hasArg("wifi_form")) {
    String ssid = server.hasArg("wssid") ? server.arg("wssid") : "";
    ssid.trim();
    if (ssid.length() == 0) {
      WifiCreds::clear();
    } else {
      // An empty password keeps credentials only for the unchanged network.
      String pass = server.hasArg("wpass") ? server.arg("wpass") : "";
      if (pass.isEmpty() && ssid == WifiCreds::ssid()) {
        pass = WifiCreds::pass();
      }
      WifiCreds::save(ssid, pass);
    }
  }

  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

static void handleDeleteSleepImg() {
  if (FS.exists("/sleep.bin"))
    FS.remove("/sleep.bin");
  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

void registerSettingsRoutes() {
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/settings", HTTP_POST, handleSettingsPost);
  server.on("/del-sleep", HTTP_POST, handleDeleteSleepImg);
}
