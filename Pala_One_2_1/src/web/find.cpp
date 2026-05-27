#include "src/web/find.h"

#include <WiFi.h>   // WiFiClient

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h"             // prefKeyForBook
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"
#include "src/storage/preferences_store.h"
#include "src/ui/reader.h"                // g_bookview / findPageForOffset
#include "src/ui/screens/reader_screen.h" // g_readerScreen (active-reader check)
#include "src/ui/text.h"                  // pageOffsetForPage

namespace {

// Validate ?id and return the book index, or -1 with an error response sent.
int requireBookId(const char* arg = "id") {
  if (!server.hasArg(arg)) {
    server.send(400, "text/plain; charset=utf-8", "missing id");
    return -1;
  }
  int id = server.arg(arg).toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", "bad id");
    return -1;
  }
  return id;
}

}  // namespace

// ============================================================================
//  GET /readbook-text?id=N — stream the raw book bytes.
//
//  Books are stored as normalized UTF-8 plain text after upload, so the
//  client can search them directly without any device-side processing. Use
//  chunked transfer so we don't have to load the whole book into RAM.
// ============================================================================
static void handleReadbookText() {
  int id = requireBookId();
  if (id < 0) return;

  String path = String(g_library.books[id].path);
  File f = FS.open(path, "r");
  if (!f) {
    server.send(404, "text/plain; charset=utf-8", "Open failed");
    return;
  }

  size_t total = f.size();
  server.setContentLength(total);
  server.send(200, "text/plain; charset=utf-8", "");
  WiFiClient client = server.client();

  uint8_t buf[512];
  while (f.available() && client.connected()) {
    size_t want = f.available() > sizeof(buf) ? sizeof(buf) : f.available();
    size_t got = f.read(buf, want);
    if (got == 0) break;
    client.write(buf, got);
  }
  f.close();
}

// ============================================================================
//  POST /jumpoffset — set the resume position to a specific byte offset.
//
//  Mirrors /api/books/jumppage's persistence: write both the byte offset
//  (canonical) and the derived page number (display hint). If the reader is
//  currently active on this book, also update the in-memory cursor so the
//  next render lands at the new position without needing a reopen.
//
//  Accepts urlencoded form bodies (legacy form-POST flow) as well as
//  ?id=N&offset=BYTES on the URL — both surface via server.arg().
// ============================================================================
static void handleJumpOffset() {
  int id = requireBookId();
  if (id < 0) return;
  if (!server.hasArg("offset")) {
    server.send(400, "text/plain; charset=utf-8", "missing offset");
    return;
  }

  String path = String(g_library.books[id].path);
  String key  = prefKeyForBook(path);

  File f = FS.open(path, "r");
  if (!f) {
    server.send(500, "text/plain; charset=utf-8", "Open failed");
    return;
  }
  size_t fileSize = f.size();
  long requested = server.arg("offset").toInt();
  if (requested < 0) requested = 0;
  if ((size_t)requested >= fileSize) requested = (long)(fileSize > 0 ? fileSize - 1 : 0);

  PreferencesStore kv(prefs);
  saveSavedOffset(kv, key, (uint32_t)requested);

  // If the reader is currently active on this book, update the in-memory
  // cursor + save the matching page number so the device sees the jump on
  // its next render. Otherwise the offset alone is enough: openBookByIndex
  // resolves it to a page on next open via findPageForOffset (the saved
  // page-number key is just a stale display hint, refreshed on next open).
  bool activeOnThisBook =
      (g_currentScreen == &g_readerScreen) &&
      g_bookview.book.isOpen() &&
      g_bookview.book.path() == path;

  if (activeOnThisBook) {
    int newPage = findPageForOffset((uint32_t)requested);
    g_bookview.cursor.pageIndex = newPage;
    saveSavedPage(kv, key, newPage);
  }
  f.close();

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void registerFindRoutes() {
  server.on("/readbook-text", HTTP_GET,  handleReadbookText);
  server.on("/jumpoffset",    HTTP_POST, handleJumpOffset);
}
