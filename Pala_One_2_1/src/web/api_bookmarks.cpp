#include "src/web/api_bookmarks.h"

#include <ArduinoJson.h>

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h"
#include "src/pure/paths.h"          // stripTxtExt, lastPathComponent
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"
#include "src/ui/text.h"             // resolveBookmarkOffset, extractPageText, readBookmarkLabelAtOffset

// ----------------------------------------------------------------------------
//  Shared helpers used by the list + per-bookmark views.
// ----------------------------------------------------------------------------

// Lift one bookmark's label snippet out of the open book file. Caller owns
// `f` (opened once per book, reused across all that book's bookmarks).
static String labelForBookmark(File& f, const String& bookPath,
                               uint16_t page, uint32_t storedOffset) {
  int targetPage = (int)page;
  if (targetPage < 0) targetPage = 0;
  uint32_t pageOff = resolveBookmarkOffset(bookPath, (uint16_t)targetPage, storedOffset);
  FileReadStream stream(f);
  return readBookmarkLabelAtOffset(stream, pageOff, targetPage);
}

// Read one page of text (~one screen on the device). Standalone — opens the
// file itself — for the per-bookmark view route, which isn't iterating.
static String readPageText(const String& bookPath, int targetPage, uint32_t storedOffset) {
  File f = FS.open(bookPath, "r");
  if (!f) return String(D_WEB_BOOKMARK_OPEN_FAILED_DOT);
  uint32_t pageOff = resolveBookmarkOffset(bookPath, (uint16_t)targetPage, storedOffset);
  String out;
  out.reserve(900);
  (void)extractPageText(f, pageOff, out);
  f.close();
  out.trim();
  if (out.length() == 0) out = D_WEB_BOOKMARK_PAGE_EMPTY;
  return out;
}

// Parse a non-negative int query arg, returning -1 if missing/invalid. Used
// by every "needs book/idx" handler — keeps validation uniform.
static int queryInt(const char* name) {
  if (!server.hasArg(name)) return -1;
  String s = server.arg(name);
  if (s.length() == 0) return -1;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c < '0' || c > '9') return -1;
  }
  return s.toInt();
}

// ----------------------------------------------------------------------------
//  GET /api/bookmarks
//  All books, each with their bookmarks + label snippets. Books with no
//  bookmarks are included with an empty array so the SPA can decide
//  whether to surface them.
// ----------------------------------------------------------------------------
static void handleApiBookmarksList() {
  JsonDocument doc;
  JsonArray books = doc["books"].to<JsonArray>();

  for (int i = 0; i < g_library.bookCount; i++) {
    JsonObject bookObj = books.add<JsonObject>();
    bookObj["id"]   = i;
    bookObj["name"] = g_library.books[i].name;

    String bookPath = String(g_library.books[i].path);
    String key      = prefKeyForBook(bookPath);
    uint16_t pages[MAX_BOOKMARKS];
    uint32_t offsets[MAX_BOOKMARKS];
    uint8_t count = loadBookmarksForKey(key, pages, offsets);

    JsonArray bms = bookObj["bookmarks"].to<JsonArray>();
    if (count == 0) continue;

    File f = FS.open(bookPath, "r");
    if (!f) {
      bookObj["error"] = "open_failed";
      continue;
    }
    for (int j = 0; j < count; j++) {
      JsonObject bm = bms.add<JsonObject>();
      bm["idx"]   = j;
      bm["page"]  = (int)pages[j];
      bm["label"] = labelForBookmark(f, bookPath, pages[j], offsets[j]);
    }
    f.close();
  }

  String out;
  out.reserve(2048);
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", out);
}

// ----------------------------------------------------------------------------
//  GET /api/bookmarks/view?book=N&idx=M
//  Returns the bookmark's page text + label + book name. SPA renders inline.
// ----------------------------------------------------------------------------
static void handleApiBookmarksView() {
  int b   = queryInt("book");
  int idx = queryInt("idx");
  if (b < 0 || idx < 0) {
    server.send(400, "text/plain; charset=utf-8", "missing or invalid book/idx");
    return;
  }
  if (b >= g_library.bookCount) {
    server.send(404, "text/plain; charset=utf-8", "book not found");
    return;
  }

  String bookPath = String(g_library.books[b].path);
  String key      = prefKeyForBook(bookPath);
  uint16_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t count = loadBookmarksForKey(key, pages, offsets);
  if (idx >= count) {
    server.send(404, "text/plain; charset=utf-8", "bookmark not found");
    return;
  }

  int targetPage = (int)pages[idx];
  if (targetPage < 0) targetPage = 0;

  // Two opens for one click: once to read the label snippet (consistent
  // with the list view), once to read the page body. Wash on flash IO —
  // both are sequential reads into a tiny page-cache window.
  String label;
  {
    File f = FS.open(bookPath, "r");
    if (f) {
      label = labelForBookmark(f, bookPath, pages[idx], offsets[idx]);
      f.close();
    }
  }
  String text = readPageText(bookPath, targetPage, offsets[idx]);

  JsonDocument doc;
  JsonObject book = doc["book"].to<JsonObject>();
  book["id"]   = b;
  book["name"] = g_library.books[b].name;
  JsonObject bm = doc["bookmark"].to<JsonObject>();
  bm["idx"]   = idx;
  bm["page"]  = targetPage;
  bm["label"] = label;
  bm["text"]  = text;

  String out;
  out.reserve(text.length() + label.length() + 256);
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", out);
}

// ----------------------------------------------------------------------------
//  POST /api/bookmarks/delete  — body { "book": N, "idx": M }
// ----------------------------------------------------------------------------
static void handleApiBookmarksDelete() {
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
  int b   = doc["book"].is<int>() ? doc["book"].as<int>() : -1;
  int idx = doc["idx"].is<int>()  ? doc["idx"].as<int>()  : -1;
  if (b < 0 || idx < 0) {
    server.send(400, "text/plain; charset=utf-8", "missing book/idx");
    return;
  }
  if (b >= g_library.bookCount) {
    server.send(404, "text/plain; charset=utf-8", "book not found");
    return;
  }

  String key = prefKeyForBook(String(g_library.books[b].path));
  uint16_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t count = loadBookmarksForKey(key, pages, offsets);
  if (idx >= count) {
    server.send(404, "text/plain; charset=utf-8", "bookmark not found");
    return;
  }

  for (int i = idx + 1; i < count; i++) {
    pages[i - 1]   = pages[i];
    offsets[i - 1] = offsets[i];
  }
  count--;
  saveBookmarksForKey(key, pages, offsets, count);

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

// ----------------------------------------------------------------------------
//  GET /api/bookmarks/export?book=N
//  text/plain download — page contents for every bookmark in the book.
//  Browser-driven (Content-Disposition); SPA just navigates to the URL.
// ----------------------------------------------------------------------------
static void handleApiBookmarksExport() {
  int b = queryInt("book");
  if (b < 0) {
    server.send(400, "text/plain; charset=utf-8", "missing book");
    return;
  }
  if (b >= g_library.bookCount) {
    server.send(404, "text/plain; charset=utf-8", "book not found");
    return;
  }

  String bookPath = String(g_library.books[b].path);
  String key = prefKeyForBook(bookPath);
  uint16_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t count = loadBookmarksForKey(key, pages, offsets);
  if (count == 0) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_NO_BOOKMARKS_THIS_BOOK);
    return;
  }

  File f = FS.open(bookPath, "r");
  if (!f) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_BOOKMARKS_OPEN_FAILED_CARD);
    return;
  }

  String exportName = stripTxtExt(lastPathComponent(bookPath));
  exportName.replace(' ', '_');
  exportName += "_bookmarks.txt";

  String out;
  out.reserve(8192);
  out += D_WEB_BMEXPORT_BOOK;
  out += stripTxtExt(lastPathComponent(bookPath));
  out += "\n";
  out += D_WEB_BMEXPORT_BOOKMARKS;
  out += String(count);
  out += "\n\n";

  for (int i = 0; i < count; i++) {
    int targetPage = (int)pages[i];
    if (targetPage < 0) targetPage = 0;
    String label = labelForBookmark(f, bookPath, pages[i], offsets[i]);
    String text  = readPageText(bookPath, targetPage, offsets[i]);
    out += "==================================================\n";
    out += D_WEB_BMEXPORT_BOOKMARK_LBL;
    out += String(i + 1);
    out += "\n";
    out += label;
    out += "\n";
    out += "--------------------------------------------------\n";
    out += text;
    out += "\n\n";
  }
  f.close();

  server.sendHeader(
    "Content-Disposition",
    String("attachment; filename=\"") + exportName + "\""
  );
  server.send(200, "text/plain; charset=utf-8", out);
}

void registerApiBookmarksRoutes() {
  server.on("/api/bookmarks",        HTTP_GET,  handleApiBookmarksList);
  server.on("/api/bookmarks/view",   HTTP_GET,  handleApiBookmarksView);
  server.on("/api/bookmarks/delete", HTTP_POST, handleApiBookmarksDelete);
  server.on("/api/bookmarks/export", HTTP_GET,  handleApiBookmarksExport);
}
