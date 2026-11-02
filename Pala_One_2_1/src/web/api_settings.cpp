#include "src/web/api_settings.h"

#include <ArduinoJson.h>

#include "src/config.h"                   // LIB_HEADER_TITLE (compile-time default)
#include "src/state.h"
#include "src/storage/book_metadata.h"   // saveSavedPage
#include "src/storage/click_timings.h"
#include "src/storage/library_menu_order.h"
#include "src/storage/preferences_store.h"
#include "src/storage/wifi_creds.h"
#include "src/ui/font.h"
#include "src/ui/header_title.h"
#include "src/ui/icons.h"                  // Icons::sleepIconEnabled/setSleepIconEnabled
#include "src/ui/reader.h"                // g_bookview, findPageForOffset, renderCurrentPage
#include "src/ui/reader_actions.h"        // ButtonAction + Gestures
#include "src/ui/screens/reader_screen.h" // g_readerScreen
#include "src/ui/screen_settings.h"        // ScreenSettings::isScreenFlipped/setScreenRotation
#include "src/ui/sleep.h"

// Wire string for the font family. Two-way mapping with Font::Family.
static const char* familyToWire(Font::Family f) {
  return (f == Font::Family::OpenDyslexic) ? "dys" : "helv";
}
static Font::Family familyFromWire(const char* s) {
  return (s && strcmp(s, "dys") == 0) ? Font::Family::OpenDyslexic
                                      : Font::Family::Helvetica;
}

// Wire string for a gesture binding. Strings (rather than the enum's int
// values) so the SPA can name things readably without leaking enum integers
// across the wire.
static const char* actionToWire(ButtonAction a) {
  switch (a) {
    case ACTION_NEXT:     return "next";
    case ACTION_PREV:     return "prev";
    case ACTION_MENU:     return "menu";
    case ACTION_LOCK:     return "lock";
    case ACTION_HOME:     return "home";
    case ACTION_BOOKMARK: return "bookmark";
    case ACTION_ROTATE:   return "rotate";
    case ACTION_NONE:
    default:              return "none";
  }
}
static ButtonAction actionFromWire(const char* s) {
  if (!s) return ACTION_NONE;
  if (strcmp(s, "next")     == 0) return ACTION_NEXT;
  if (strcmp(s, "prev")     == 0) return ACTION_PREV;
  if (strcmp(s, "menu")     == 0) return ACTION_MENU;
  if (strcmp(s, "lock")     == 0) return ACTION_LOCK;
  if (strcmp(s, "home")     == 0) return ACTION_HOME;
  if (strcmp(s, "bookmark") == 0) return ACTION_BOOKMARK;
  if (strcmp(s, "rotate")   == 0) return ACTION_ROTATE;
  return ACTION_NONE;
}

// The six remappable gestures, in wire-key order. One table drives GET,
// POST and the required-action check so they can't drift apart.
struct GestureBinding {
  const char* wireKey;
  ButtonAction (*get)();
  void (*set)(ButtonAction);
};
static const GestureBinding kGestureBindings[] = {
  {"short",     Gestures::actionShort,     Gestures::setActionShort},
  {"double",    Gestures::actionDouble,    Gestures::setActionDouble},
  {"triple",    Gestures::actionTriple,    Gestures::setActionTriple},
  {"long",      Gestures::actionLong,      Gestures::setActionLong},
  {"extraLong", Gestures::actionExtraLong, Gestures::setActionExtraLong},
  {"clickHold", Gestures::actionClickHold, Gestures::setActionClickHold},
};

// Next, Home and OK/Menu must stay reachable from some gesture or the
// device can't be navigated. Checks the bindings that would result from
// applying `g` on top of the current ones (fields are individually
// optional), so a partial update is validated against the full picture.
static bool gestureBindingsValid(JsonObjectConst g) {
  bool hasNext = false, hasHome = false, hasMenu = false;
  for (const GestureBinding& b : kGestureBindings) {
    ButtonAction a = g[b.wireKey].is<const char*>()
                         ? actionFromWire(g[b.wireKey].as<const char*>())
                         : b.get();
    if (a == ACTION_NEXT) hasNext = true;
    if (a == ACTION_HOME) hasHome = true;
    if (a == ACTION_MENU) hasMenu = true;
  }
  return hasNext && hasHome && hasMenu;
}

// Wire keys for ClickTimings::timingSettings(), index-aligned with that
// table (gap, sequence, long, very-long, debounce). Applying in table order
// matters: sequence's minimum is the gap, very-long's is long + 1.
static const char* const kTimingWireKeys[] = {
  "gap", "sequence", "long", "veryLong", "debounce",
};
static constexpr uint8_t kTimingWireKeyCount =
    sizeof(kTimingWireKeys) / sizeof(kTimingWireKeys[0]);

static uint8_t timingCount() {
  uint8_t n = ClickTimings::timingSettingsCount();
  return n < kTimingWireKeyCount ? n : kTimingWireKeyCount;
}

// Wire names for the system entries the library menu can show. Order here
// is the order the SPA lists them in its dropdowns.
struct LibraryEntryWire {
  LibraryEntryType type;
  const char* wire;
};
static const LibraryEntryWire kLibraryEntries[] = {
  {LIB_ENTRY_BOOKMARKS,  "bookmarks"},
  {LIB_ENTRY_LIST,       "list"},
  {LIB_ENTRY_APPS,       "apps"},
  {LIB_ENTRY_STATISTICS, "statistics"},
  {LIB_ENTRY_ABOUT,      "device"},
  {LIB_ENTRY_UPLOAD,     "upload"},
  {LIB_ENTRY_UPDATE,     "update"},
  {LIB_ENTRY_SETTINGS,   "settings"},
};

static const char* libraryEntryToWire(LibraryEntryType t) {
  for (const LibraryEntryWire& e : kLibraryEntries) {
    if (e.type == t) return e.wire;
  }
  return nullptr;
}
static bool libraryEntryFromWire(const char* s, LibraryEntryType* out) {
  if (!s) return false;
  for (const LibraryEntryWire& e : kLibraryEntries) {
    if (strcmp(e.wire, s) == 0) { *out = e.type; return true; }
  }
  return false;
}

// 802.11 SSID and WPA passphrase limits, in bytes.
static constexpr size_t kMaxSsidBytes = 32;
static constexpr size_t kMaxPassBytes = 64;

static bool jsonArrayContains(JsonArrayConst arr, const char* s) {
  for (JsonVariantConst v : arr) {
    if (v.is<const char*>() && strcmp(v.as<const char*>(), s) == 0) return true;
  }
  return false;
}

// Serialise the current settings into the destination object. Used by both
// GET (just dump state) and POST (echo back the resulting state).
static void writeSettingsTo(JsonObject obj) {
  obj["font"]          = Font::currentBodySize();
  obj["family"]        = familyToWire(Font::currentFamily());
  obj["sleep"]         = Sleep::idleTimeoutSecs();
  obj["lgap"]          = Font::currentLineGap();
  obj["bionic"]        = Font::bionicEnabled();
  obj["halfParagraphGaps"] = Font::halfParagraphGapsEnabled();
  obj["noScreensaver"] = Sleep::noScreensaver();
  obj["lockOnSleep"]   = Sleep::lockOnSleep();
  obj["screenRotation"] = ScreenSettings::isScreenFlipped();
  obj["sleepIcon"]         = Icons::sleepIconEnabled();
  obj["batteryIndicators"] = ScreenSettings::batteryIndicatorsEnabled();
  obj["hasSleepImage"] = FS.exists("/sleep.bin");
  // headerTitleDefault lets the SPA show a "Reset to default" affordance
  // without having to hard-code LIB_HEADER_TITLE (which includes the build
  // git hash on PlatformIO).
  obj["headerTitle"]        = HeaderTitle::current();
  obj["headerTitleDefault"] = LIB_HEADER_TITLE;

  JsonObject g = obj["gestures"].to<JsonObject>();
  for (const GestureBinding& b : kGestureBindings) {
    g[b.wireKey] = actionToWire(b.get());
  }
  g["legacyControls"] = Gestures::legacyControlsOn();

  // Stored home network. The password itself never leaves the device —
  // the SPA only needs to know whether one is saved.
  JsonObject w = obj["wifi"].to<JsonObject>();
  w["ssid"]        = WifiCreds::ssid();
  w["hasPassword"] = WifiCreds::pass().length() > 0;

  // Library menu: visible system entries, top to bottom. Slots tells the
  // SPA how many positions to offer.
  obj["libraryMenuSlots"] = LibraryMenuOrder::kMaxSystemEntries;
  JsonArray lib = obj["libraryMenu"].to<JsonArray>();
  for (int i = 0; i < LibraryMenuOrder::count(); i++) {
    const char* w = libraryEntryToWire(LibraryMenuOrder::entryAt(i));
    if (w) lib.add(w);
  }

  // Click timings, in ms. min/max are live: sequence's floor follows the
  // current gap and very-long's follows long.
  JsonArray tim = obj["timings"].to<JsonArray>();
  const ClickTimings::TimingSettingSpec* specs = ClickTimings::timingSettings();
  for (uint8_t i = 0; i < timingCount(); i++) {
    JsonObject t = tim.add<JsonObject>();
    t["key"]     = kTimingWireKeys[i];
    t["value"]   = specs[i].current();
    t["default"] = specs[i].defaultValue;
    t["min"]     = specs[i].minValue();
    t["max"]     = specs[i].maxValue();
  }
}

static void sendSettings(int status) {
  JsonDocument doc;
  writeSettingsTo(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(status, "application/json; charset=utf-8", out);
}

// ----------------------------------------------------------------------------
//  GET /api/settings
// ----------------------------------------------------------------------------
static void handleApiSettingsGet() {
  sendSettings(200);
}

// ----------------------------------------------------------------------------
//  POST /api/settings
//  Applies any provided fields and returns the resulting state. Missing
//  fields are left unchanged — this is PATCH-flavoured even though the
//  method is POST.
//
//  Reader-recovery: if a layout-affecting field changes while the reader
//  screen is active, snapshot the current byte offset, reset the in-memory
//  page table, and re-locate the cursor on the page that contains the
//  saved byte. The on-disk page cache is layout-stamped so it self-
//  invalidates on next load.
// ----------------------------------------------------------------------------
static bool applyFromJson(JsonObjectConst body) {
  bool layoutChanged = false;

  if (body["font"].is<int>()) {
    int v = body["font"].as<int>();
    if (v != Font::currentBodySize()) { Font::setBodySize(v); layoutChanged = true; }
  }
  if (body["family"].is<const char*>()) {
    Font::Family want = familyFromWire(body["family"].as<const char*>());
    if (want != Font::currentFamily()) { Font::setFamily(want); layoutChanged = true; }
  }
  if (body["sleep"].is<int>()) {
    int v = body["sleep"].as<int>();
    if (v != Sleep::idleTimeoutSecs()) Sleep::setIdleTimeout(v);
  }
  if (body["lgap"].is<int>()) {
    int v = body["lgap"].as<int>();
    if (v != Font::currentLineGap()) { Font::setLineGap(v); layoutChanged = true; }
  }
  if (body["bionic"].is<bool>()) {
    bool v = body["bionic"].as<bool>();
    if (v != Font::bionicEnabled()) { Font::setBionic(v); layoutChanged = true; }
  }
  if (body["halfParagraphGaps"].is<bool>()) {
    bool v = body["halfParagraphGaps"].as<bool>();
    if (v != Font::halfParagraphGapsEnabled()) { Font::setHalfParagraphGaps(v); layoutChanged = true; }
  }
  if (body["noScreensaver"].is<bool>()) {
    bool v = body["noScreensaver"].as<bool>();
    if (v != Sleep::noScreensaver()) Sleep::setNoScreensaver(v);
  }
  if (body["lockOnSleep"].is<bool>()) {
    bool v = body["lockOnSleep"].as<bool>();
    if (v != Sleep::lockOnSleep()) Sleep::setLockOnSleep(v);
  }
  if (body["screenRotation"].is<bool>()) {
    bool v = body["screenRotation"].as<bool>();
    if (v != ScreenSettings::isScreenFlipped()) ScreenSettings::setScreenRotation(v);
  }
  if (body["sleepIcon"].is<bool>()) {
    bool v = body["sleepIcon"].as<bool>();
    if (v != Icons::sleepIconEnabled()) Icons::setSleepIconEnabled(v);
  }
  if (body["batteryIndicators"].is<bool>()) {
    bool v = body["batteryIndicators"].as<bool>();
    if (v != ScreenSettings::batteryIndicatorsEnabled()) ScreenSettings::setBatteryIndicatorsEnabled(v);
  }

  // Header title — `headerTitleReset:true` wins over `headerTitle` so the
  // SPA's "Reset" button works even if the text field is also sent. Empty
  // string = hide the header (per HeaderTitle's spec); missing both fields
  // = unchanged.
  if (body["headerTitleReset"].is<bool>() && body["headerTitleReset"].as<bool>()) {
    HeaderTitle::resetToDefault();
  } else if (body["headerTitle"].is<const char*>()) {
    String s = body["headerTitle"].as<const char*>();
    s.trim();
    HeaderTitle::set(s.c_str());
  }

  // Gestures — each field individually optional. The setters clamp +
  // persist internally, so we don't need to read-back to filter no-ops.
  // The required-action check already ran in handleApiSettingsPost.
  JsonObjectConst g = body["gestures"].as<JsonObjectConst>();
  if (!g.isNull()) {
    for (const GestureBinding& b : kGestureBindings) {
      if (g[b.wireKey].is<const char*>())
        b.set(actionFromWire(g[b.wireKey].as<const char*>()));
    }
    if (g["legacyControls"].is<bool>()) {
      bool v = g["legacyControls"].as<bool>();
      if (v != Gestures::legacyControlsOn()) Gestures::setLegacyControls(v);
    }
  }

  // Wi-Fi — blank SSID forgets the network. An omitted `pass` keeps the
  // stored one (the SPA never has it to send back); an explicit "" saves
  // an open network. Lengths were checked in handleApiSettingsPost.
  JsonObjectConst w = body["wifi"].as<JsonObjectConst>();
  if (!w.isNull() && w["ssid"].is<const char*>()) {
    String ssid = w["ssid"].as<const char*>();
    ssid.trim();
    if (ssid.length() == 0) {
      WifiCreds::clear();
    } else {
      String pass = w["pass"].is<const char*>() ? String(w["pass"].as<const char*>())
                                                : WifiCreds::pass();
      WifiCreds::save(ssid, pass);
    }
  }

  // Library menu — `libraryMenuReset:true` wins over `libraryMenu`.
  // Unknown names are skipped; LibraryMenuOrder dedupes and always keeps
  // Upload, so the device stays reachable over Wi-Fi.
  if (body["libraryMenuReset"].is<bool>() && body["libraryMenuReset"].as<bool>()) {
    LibraryMenuOrder::resetToDefaults();
  } else if (body["libraryMenu"].is<JsonArrayConst>()) {
    LibraryEntryType entries[LibraryMenuOrder::kMaxSystemEntries];
    int n = 0;
    for (JsonVariantConst v : body["libraryMenu"].as<JsonArrayConst>()) {
      if (n >= LibraryMenuOrder::kMaxSystemEntries) break;
      LibraryEntryType t;
      if (v.is<const char*>() && libraryEntryFromWire(v.as<const char*>(), &t)) {
        entries[n++] = t;
      }
    }
    LibraryMenuOrder::setEntries(entries, n);
  }

  // Click timings — `timingsReset: [key…]` restores defaults and wins over a
  // value sent for the same key. Setters clamp to the live range + persist.
  JsonObjectConst tim = body["timings"].as<JsonObjectConst>();
  JsonArrayConst timReset = body["timingsReset"].as<JsonArrayConst>();
  if (!tim.isNull() || !timReset.isNull()) {
    const ClickTimings::TimingSettingSpec* specs = ClickTimings::timingSettings();
    for (uint8_t i = 0; i < timingCount(); i++) {
      const char* key = kTimingWireKeys[i];
      if (!timReset.isNull() && jsonArrayContains(timReset, key)) {
        specs[i].reset();
      } else if (!tim.isNull() && tim[key].is<long>()) {
        // Floor negatives first: a straight cast to uint32_t would wrap to
        // ~4.29e9 and clamp to the max instead of the min.
        long raw = tim[key].as<long>();
        specs[i].set(raw < 0 ? 0u : (uint32_t)raw);
      }
    }
  }

  return layoutChanged;
}

static void handleApiSettingsPost() {
  const String& body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "empty body");
    return;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    String msg = String("bad json: ") + err.c_str();
    server.send(400, "text/plain; charset=utf-8", msg);
    return;
  }

  // Reject before applying anything so a bad gesture map can't half-land.
  // The SPA runs the same check client-side and shows a localised message;
  // this is the backstop for other clients.
  JsonObjectConst gestures = doc["gestures"].as<JsonObjectConst>();
  if (!gestures.isNull() && !gestureBindingsValid(gestures)) {
    server.send(400, "text/plain; charset=utf-8",
                "gestures must include next, home and menu");
    return;
  }
  JsonObjectConst wifi = doc["wifi"].as<JsonObjectConst>();
  if (!wifi.isNull()) {
    if (wifi["ssid"].is<const char*>() && strlen(wifi["ssid"].as<const char*>()) > kMaxSsidBytes) {
      server.send(400, "text/plain; charset=utf-8", "ssid too long");
      return;
    }
    if (wifi["pass"].is<const char*>() && strlen(wifi["pass"].as<const char*>()) > kMaxPassBytes) {
      server.send(400, "text/plain; charset=utf-8", "password too long");
      return;
    }
  }

  // Snapshot the reader's current byte offset before applying changes so
  // we can re-land on the same byte under the new layout.
  bool readerActive =
      (g_currentScreen == &g_readerScreen) &&
      g_bookview.book.isOpen();
  uint32_t savedByte = 0;
  if (readerActive
      && g_bookview.cursor.pageIndex >= 0
      && g_bookview.cursor.pageIndex < g_bookview.pages.count) {
    savedByte = g_bookview.pages.offsets[g_bookview.cursor.pageIndex];
  }

  bool layoutChanged = applyFromJson(doc.as<JsonObjectConst>());

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
    renderCurrentPage();
  }

  sendSettings(200);
}

// ----------------------------------------------------------------------------
//  POST /api/sleep-image/delete  — wipes /sleep.bin and reports new state.
// ----------------------------------------------------------------------------
static void handleApiSleepImageDelete() {
  if (FS.exists("/sleep.bin")) FS.remove("/sleep.bin");
  JsonDocument doc;
  doc["ok"]            = true;
  doc["hasSleepImage"] = FS.exists("/sleep.bin");
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", out);
}

void registerApiSettingsRoutes() {
  server.on("/api/settings",           HTTP_GET,  handleApiSettingsGet);
  server.on("/api/settings",           HTTP_POST, handleApiSettingsPost);
  server.on("/api/sleep-image/delete", HTTP_POST, handleApiSleepImageDelete);
}
