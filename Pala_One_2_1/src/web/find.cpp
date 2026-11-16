#include "src/web/web_assets.h"
#include "src/lang/join.h"
#include "src/web/book_text_stream.h"
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
#include "src/web/chrome.h"

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

  streamBookText(f);
  f.close();
}

// ============================================================================
//  POST /jumpoffset — set the resume position to a specific byte offset.
//
//  Mirrors /jumppage's persistence: write both the byte offset (canonical)
//  and the derived page number (display hint). If the reader is currently
//  active on this book, also update the in-memory cursor so the next render
//  lands at the new position without needing a reopen.
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
  size_t fileSize = bookLogicalSize(f);
  long requested = server.arg("offset").toInt();
  if (requested < 0) requested = 0;
  if ((size_t)requested >= fileSize) requested = (long)(fileSize > 0 ? fileSize - 1 : 0);

  PreferencesStore kv(prefs);

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
    if (newPage < 0) { f.close(); server.send(409, "text/plain; charset=utf-8", "Cancelled"); return; }
    g_bookview.cursor.pageIndex = newPage;
    saveSavedPage(kv, key, newPage);
  }
  saveSavedOffset(kv, key, (uint32_t)requested);
  f.close();

  server.sendHeader("Location", String("/read?id=") + String(id));
  server.send(302, "text/plain", "");
}

// ============================================================================
//  GET /read?id=N — in-browser reader with find + jump UI.
//
//  The page loads the book text once via /readbook-text, runs all searches
//  client-side (browser handles the regex), and offers a "jump to here"
//  button per match that POSTs the byte offset back to the device.
// ============================================================================

// Static payload served from losslessly compressed, content-hashed assets.

// Embedded JS — loads the book text, builds a match list, scrolls between
// matches, and POSTs the byte offset of the active match to /jumpoffset.
//
// Matching is forgiving: the query gets the same typography folding that
// book uploads get on the device (smart quotes / dashes / ellipsis / NBSP
// to ASCII), then both sides are compared case-folded with whitespace runs
// collapsed. A sentence pasted from another reading app therefore matches
// the stored text even when quotes, dashes, or line breaks differ. An
// index map converts normalized match positions back to raw string
// positions for highlighting.
//
// Offsets sent to /jumpoffset are true UTF-8 byte offsets (TextEncoder),
// not JS string indices — the two diverge on any book containing
// multi-byte characters (accented letters, etc.).
// Static payload served from losslessly compressed, content-hashed assets.

static void handleReadView() {
  int id = requireBookId();
  if (id < 0) return;

  String path = String(g_library.books[id].path);
  int savedPage = savedPageForBookPath(path) + 1;
  if (savedPage < 1) savedPage = 1;

  String out = webPageStart(
    D_WEB_READ_TITLE,
    D_WEB_READ_SUBTITLE,
    Language::join({"<a href='/'>",  D_WEB_NAV_HOME,  "</a><a href='/files'>",  D_WEB_NAV_FILES,  "</a><a href='/bookmarks'>",  D_WEB_NAV_BOOKMARKS,  "</a>"}),
    true
  );
  out.reserve(out.length() + 6000);
  WebAssets::appendStyle(out,WebAssets::Id::ReadCss);

  out += "<div class='card'><h2>";
  out += htmlEscape(String(g_library.books[id].name));
  out += "</h2><div class='meta'>";
  out += String((int)g_library.books[id].size);
  out += Language::join({" ",  D_WEB_READ_BYTES_LABEL,  " &middot; ",  D_WEB_READ_CURRENT_PAGE_LABEL,  " "});
  out += String(savedPage);
  out += "</div>";

  // Find UI.
  out += Language::join({"<div class='bv-find'>", 
         "<input id='bvFind' type='search' placeholder='",  D_WEB_READ_FIND_PLACEHOLDER,  "' autocomplete='off'>", 
         "<button type='button' class='btn small' id='bvFindBtn'>",  D_WEB_READ_FIND_ALL,  "</button>", 
         "<button type='button' class='btn secondary small' id='bvFindPrev'>&#9650; ",  D_WEB_READ_FIND_PREV,  "</button>", 
         "<button type='button' class='btn secondary small' id='bvFindNext'>&#9660; ",  D_WEB_READ_FIND_NEXT,  "</button>", 
         "<button type='button' class='btn small' id='bvJumpBtn'>",  D_WEB_READ_JUMP_HERE,  "</button>", 
         "</div>", 
         "<div class='bv-status' id='bvFindStat'>",  D_WEB_READ_LOADING,  "</div>", 
         "<div class='bv-status' id='bvJumpStat'></div>"});

  // Jump-by-page form (uses the existing /jumppage route).
  out += "<div style='margin-top:14px;border-top:1px solid var(--line-soft);padding-top:12px'>"
         "<form method='POST' action='/jumppage' class='bv-find'>"
         "<input type='hidden' name='id' value='" + String(id) + Language::join({"'>", 
         "<input type='text' name='page' inputmode='numeric' placeholder='",  D_WEB_READ_PAGE_PLACEHOLDER,  "' value='"}) + String(savedPage) + Language::join({"' style='max-width:140px'>", 
         "<button type='submit' class='btn small'>",  D_WEB_READ_JUMP_PAGE,  "</button>", 
         "<span class='muted small'>",  D_WEB_READ_JUMP_HINT,  "</span>", 
         "</form></div>"});

  out += "<div id='bvData' data-id='" + String(id) + "'></div>";
  out += "<div id='bvText' class='bv-text'></div>";
  out += "</div>";

  WebAssets::appendScript(out,WebAssets::Id::ReadJs);
  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

void registerFindRoutes() {
  server.on("/read",          HTTP_GET,  handleReadView);
  server.on("/readbook-text", HTTP_GET,  handleReadbookText);
  server.on("/jumpoffset",    HTTP_POST, handleJumpOffset);
}
