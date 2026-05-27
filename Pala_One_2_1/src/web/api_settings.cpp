#include "src/web/api_settings.h"

#include <ArduinoJson.h>

#include "src/state.h"
#include "src/storage/book_metadata.h"   // saveSavedPage
#include "src/storage/preferences_store.h"
#include "src/ui/font.h"
#include "src/ui/reader.h"                // g_bookview, findPageForOffset, renderCurrentPage
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
