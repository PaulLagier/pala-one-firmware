#include "src/web/api_screensavers.h"

#include <ArduinoJson.h>

#include "src/state.h"
#include "src/ui/screensavers.h"

// Wire string for Screensavers::Mode. Mirrors the legacy /screensavers/mode
// form contract so the SPA can talk to either endpoint with the same value.
static const char* modeToWire(Screensavers::Mode m) {
  switch (m) {
    case Screensavers::Mode::Cycle:   return "cycle";
    case Screensavers::Mode::Shuffle: return "shuffle";
    case Screensavers::Mode::Single:
    default:                          return "single";
  }
}
static Screensavers::Mode modeFromWire(const char* s) {
  if (!s) return Screensavers::Mode::Single;
  if (strcmp(s, "cycle")   == 0) return Screensavers::Mode::Cycle;
  if (strcmp(s, "shuffle") == 0) return Screensavers::Mode::Shuffle;
  return Screensavers::Mode::Single;
}

// ----------------------------------------------------------------------------
//  GET /api/screensavers
// ----------------------------------------------------------------------------
static void handleApiScreensaversGet() {
  JsonDocument doc;
  doc["mode"]      = modeToWire(Screensavers::currentMode());
  doc["populated"] = Screensavers::populatedCount();
  doc["max"]       = Screensavers::MAX_SLOTS;
  doc["hasSingle"] = FS.exists("/sleep.bin");
  doc["firstFree"] = Screensavers::firstFreeSlot();
  JsonArray slots = doc["slots"].to<JsonArray>();
  for (int i = 0; i < Screensavers::MAX_SLOTS; i++) {
    JsonObject s = slots.add<JsonObject>();
    s["id"]     = i;
    s["exists"] = Screensavers::slotExists(i);
  }
  String out;
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", out);
}

// ----------------------------------------------------------------------------
//  POST /api/screensavers/mode  body { "mode": "single"|"cycle"|"shuffle" }
// ----------------------------------------------------------------------------
static void handleApiScreensaversMode() {
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
  Screensavers::setMode(modeFromWire(doc["mode"].as<const char*>()));
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

// ----------------------------------------------------------------------------
//  POST /api/screensavers/delete   body { "single": true } or { "slot": N }
// ----------------------------------------------------------------------------
static void handleApiScreensaversDelete() {
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

  if (doc["single"].as<bool>()) {
    if (FS.exists("/sleep.bin")) FS.remove("/sleep.bin");
  } else if (doc["slot"].is<int>()) {
    int slot = doc["slot"].as<int>();
    if (slot < 0 || slot >= Screensavers::MAX_SLOTS) {
      server.send(400, "text/plain; charset=utf-8", "bad slot");
      return;
    }
    Screensavers::deleteSlot(slot);
  } else {
    server.send(400, "text/plain; charset=utf-8", "missing single or slot");
    return;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void registerApiScreensaversRoutes() {
  server.on("/api/screensavers",        HTTP_GET,  handleApiScreensaversGet);
  server.on("/api/screensavers/mode",   HTTP_POST, handleApiScreensaversMode);
  server.on("/api/screensavers/delete", HTTP_POST, handleApiScreensaversDelete);
}
