#include "src/web/api_settings.h"

#include <ArduinoJson.h>

#include "src/config.h"                   // LIB_HEADER_TITLE (compile-time default)
#include "src/state.h"
#include "src/storage/book_metadata.h"   // saveSavedPage
#include "src/storage/preferences_store.h"
#include "src/ui/font.h"
#include "src/ui/header_title.h"
#include "src/ui/reader.h"                // g_bookview, findPageForOffset, renderCurrentPage
#include "src/ui/reader_actions.h"        // ButtonAction + Gestures
#include "src/ui/screens/reader_screen.h" // g_readerScreen
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
    case ACTION_BOOKMARK: return "bookmark";
    case ACTION_LOCK:     return "lock";
    case ACTION_MENU:     return "menu";
    case ACTION_NONE:
    default:              return "none";
  }
}
static ButtonAction actionFromWire(const char* s) {
  if (!s) return ACTION_NONE;
  if (strcmp(s, "bookmark") == 0) return ACTION_BOOKMARK;
  if (strcmp(s, "lock")     == 0) return ACTION_LOCK;
  if (strcmp(s, "menu")     == 0) return ACTION_MENU;
  return ACTION_NONE;
}

// Serialise the current settings into the destination object. Used by both
// GET (just dump state) and POST (echo back the resulting state).
static void writeSettingsTo(JsonObject obj) {
  obj["font"]          = Font::currentBodySize();
  obj["family"]        = familyToWire(Font::currentFamily());
  obj["sleep"]         = Sleep::idleTimeoutSecs();
  obj["lgap"]          = Font::currentLineGap();
  obj["bionic"]        = Font::bionicEnabled();
  obj["noScreensaver"] = Sleep::noScreensaver();
  obj["hasSleepImage"] = FS.exists("/sleep.bin");
  // headerTitleDefault lets the SPA show a "Reset to default" affordance
  // without having to hard-code LIB_HEADER_TITLE (which includes the build
  // git hash on PlatformIO).
  obj["headerTitle"]        = HeaderTitle::current();
  obj["headerTitleDefault"] = LIB_HEADER_TITLE;

  JsonObject g = obj["gestures"].to<JsonObject>();
  g["long"]      = actionToWire(Gestures::actionLong());
  g["extraLong"] = actionToWire(Gestures::actionExtraLong());
  g["clickHold"] = actionToWire(Gestures::actionClickHold());
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
  if (body["noScreensaver"].is<bool>()) {
    bool v = body["noScreensaver"].as<bool>();
    if (v != Sleep::noScreensaver()) Sleep::setNoScreensaver(v);
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
  JsonObjectConst g = body["gestures"].as<JsonObjectConst>();
  if (!g.isNull()) {
    if (g["long"].is<const char*>())
      Gestures::setActionLong(actionFromWire(g["long"].as<const char*>()));
    if (g["extraLong"].is<const char*>())
      Gestures::setActionExtraLong(actionFromWire(g["extraLong"].as<const char*>()));
    if (g["clickHold"].is<const char*>())
      Gestures::setActionClickHold(actionFromWire(g["clickHold"].as<const char*>()));
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
