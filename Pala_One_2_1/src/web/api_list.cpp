#include "src/web/api_list.h"

#include <ArduinoJson.h>

#include "src/config.h"
#include "src/state.h"
#include "src/storage/list_items.h"
#include "src/ui/screen.h"
#include "src/ui/screens/library_screen.h"   // g_libraryScreen
#include "src/ui/screens/list_screen.h"      // g_listScreen

// Rough upper bound for the wire payload: 16 items * ~80 bytes each
// (text + flag + JSON punctuation) + envelope. 2 KB is comfortable.
static const size_t kListJsonBufBytes = 2048;

// ----------------------------------------------------------------------------
//  GET /api/list
// ----------------------------------------------------------------------------
static void handleApiListGet() {
  JsonDocument doc;
  doc["max"] = MAX_LIST_ITEMS;
  JsonArray items = doc["items"].to<JsonArray>();
  for (int i = 0; i < g_list.count; i++) {
    JsonObject item = items.add<JsonObject>();
    item["text"] = g_list.items[i].text;
    item["done"] = (bool)g_list.items[i].done;
  }
  String out;
  out.reserve(kListJsonBufBytes);
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", out);
}

// ----------------------------------------------------------------------------
//  POST /api/list  — body: { "items": [ { "text": str, "done": bool }, ... ] }
//
//  Rebuilds g_list from the payload, skipping blank rows, capped at
//  MAX_LIST_ITEMS, then persists via saveListItems(). If the device is
//  currently on the list screen and the new list is empty, transition back
//  to the library so the device doesn't sit on a blank screen.
// ----------------------------------------------------------------------------
static void handleApiListPost() {
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
  JsonArrayConst items = doc["items"].as<JsonArrayConst>();
  if (items.isNull()) {
    server.send(400, "text/plain; charset=utf-8", "missing 'items' array");
    return;
  }

  ListState next;
  next.count = 0;
  next.selectedIndex = 0;
  for (JsonObjectConst item : items) {
    if (next.count >= MAX_LIST_ITEMS) break;
    String text = item["text"].as<const char*>() ? String(item["text"].as<const char*>()) : String();
    sanitizeListText(text);
    if (text.length() == 0) continue;
    strncpy(next.items[next.count].text, text.c_str(), MAX_LIST_TEXT);
    next.items[next.count].text[MAX_LIST_TEXT] = '\0';
    next.items[next.count].done = item["done"].as<bool>() ? 1 : 0;
    next.count++;
  }

  g_list = next;
  saveListItems();
  if (!listHasVisibleItems() && g_currentScreen == &g_listScreen) {
    g_currentScreen->nextScreen = &g_libraryScreen;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void registerApiListRoutes() {
  server.on("/api/list", HTTP_GET,  handleApiListGet);
  server.on("/api/list", HTTP_POST, handleApiListPost);
}
