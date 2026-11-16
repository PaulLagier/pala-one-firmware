#include "src/web/localized_scripts.h"
#include "src/lang/join.h"
#include "src/web/find.h"

#include <WiFi.h> // WiFiClient

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h" // prefKeyForBook
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"
#include "src/storage/preferences_store.h"
#include "src/ui/reader.h"                // g_bookview / findPageForOffset
#include "src/ui/screens/reader_screen.h" // g_readerScreen (active-reader check)
#include "src/ui/text.h"                  // pageOffsetForPage
#include "src/web/chrome.h"

namespace {

// Validate ?id and return the book index, or -1 with an error response sent.
int requireBookId(const char *arg = "id") {
  if (!server.hasArg(arg)) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_ID);
    return -1;
  }
  int id = server.arg(arg).toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return -1;
  }
  return id;
}

} // namespace

// ============================================================================
//  GET /readbook-text?id=N — stream decoded UTF-8 book bytes.
//
//  Plain text and independently compressed PBT blocks share logical offsets.
//  Decode incrementally; never load a complete book into device RAM.
// ============================================================================
static void handleReadbookText() {
  int id = requireBookId();
  if (id < 0)
    return;

  String path = String(g_library.books[id].path);
  File f = FS.open(path, "r");
  if (!f) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_ERR_OPEN_FAILED);
    return;
  }

  BookReadStream stream(f);
  if (!stream.valid()) {
    server.send(500, "text/plain", D_WEB_ERR_OPEN_FAILED);
    return;
  }
  size_t total = stream.size();
  server.setContentLength(total);
  server.send(200, "text/plain; charset=utf-8", "");
  WiFiClient client = server.client();

  uint8_t buf[512];
  while (stream.available() && client.connected()) {
    size_t got = 0;
    while (got < sizeof(buf)) {
      const int byte = stream.read();
      if (byte < 0)
        break;
      buf[got++] = uint8_t(byte);
    }
    if (!got || client.write(buf, got) != got)
      break;
  }
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
  if (id < 0)
    return;
  if (!server.hasArg("offset")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_OFFSET);
    return;
  }

  String path = String(g_library.books[id].path);
  String key = prefKeyForBook(path);

  File f = FS.open(path, "r");
  if (!f) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_OPEN_FAILED);
    return;
  }
  size_t fileSize = bookLogicalSize(f);
  long requested = server.arg("offset").toInt();
  if (requested < 0)
    requested = 0;
  if ((size_t)requested >= fileSize)
    requested = (long)(fileSize > 0 ? fileSize - 1 : 0);

  PreferencesStore kv(prefs);
  saveSavedOffset(kv, key, (uint32_t)requested);

  // If the reader is currently active on this book, update the in-memory
  // cursor + save the matching page number so the device sees the jump on
  // its next render. Otherwise the offset alone is enough: openBookByIndex
  // resolves it to a page on next open via findPageForOffset (the saved
  // page-number key is just a stale display hint, refreshed on next open).
  bool activeOnThisBook = (g_currentScreen == &g_readerScreen) && g_bookview.book.isOpen() &&
                          g_bookview.book.path() == path;

  if (activeOnThisBook) {
    int newPage = findPageForOffset((uint32_t)requested);
    g_bookview.cursor.pageIndex = newPage;
    saveSavedPage(kv, key, newPage);
  }
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

static const char kReadStyle[] PROGMEM =
    "<style>"
    ".bv-find{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin-top:8px}"
    ".bv-find input[type=search]{flex:1;min-width:160px}"
    ".bv-find .small{padding:6px 12px;font-size:13px}"
    ".bv-status{margin-top:8px;font-size:13px;color:var(--muted)}"
    ".bv-text{margin-top:14px;max-height:60vh;overflow:auto;padding:12px;border:1px solid "
    "var(--line);border-radius:10px;background:var(--pre-bg);white-space:pre-wrap;line-height:1.5;"
    "font:14px/1.5 system-ui,sans-serif}"
    ".find-hit{background:#fff4a3;color:#1f2328;padding:0 1px;border-radius:3px}"
    ".find-cur{background:#ff9c2b;color:#1f2328}"
    "html[data-theme=dark] .find-hit{background:#3a3220;color:#e8eaef}"
    "html[data-theme=dark] .find-cur{background:#b46a0a;color:#fff}"
    "</style>";

// Embedded JS — loads the book text, builds a match list, scrolls between
// matches, and POSTs the byte offset of the active match to /jumpoffset.

static void handleReadView() {
  int id = requireBookId();
  if (id < 0)
    return;

  String path = String(g_library.books[id].path);
  int savedPage = savedPageForBookPath(path) + 1;
  if (savedPage < 1)
    savedPage = 1;

  String out = webPageStart(
      D_WEB_READ_TITLE, D_WEB_READ_SUBTITLE,
      Language::join({"<a href='/'>", D_WEB_NAV_HOME, "</a><a href='/files'>", D_WEB_NAV_FILES,
                      "</a><a href='/bookmarks'>", D_WEB_NAV_BOOKMARKS, "</a>"})
          .c_str(),
      true);
  out.reserve(out.length() + 6000);
  out += FPSTR(kReadStyle);

  out += "<div class='card'><h2>";
  out += htmlEscape(String(g_library.books[id].name));
  out += "</h2><div class='meta'>";
  out += String((int)g_library.books[id].size);
  out += Language::join(
             {" ", D_WEB_READ_BYTES_LABEL, " &middot; ", D_WEB_READ_CURRENT_PAGE_LABEL, " "})
             .c_str();
  out += String(savedPage);
  out += "</div>";

  // Find UI.
  out +=
      Language::join({"<div class='bv-find'>",
                      "<input id='bvFind' type='search' placeholder='",
                      D_WEB_READ_FIND_PLACEHOLDER,
                      "' autocomplete='off'>",
                      "<button type='button' class='btn small' id='bvFindBtn'>",
                      D_WEB_READ_FIND_ALL,
                      "</button>",
                      "<button type='button' class='btn secondary small' id='bvFindPrev'>&#9650; ",
                      D_WEB_READ_FIND_PREV,
                      "</button>",
                      "<button type='button' class='btn secondary small' id='bvFindNext'>&#9660; ",
                      D_WEB_READ_FIND_NEXT,
                      "</button>",
                      "<button type='button' class='btn small' id='bvJumpBtn'>",
                      D_WEB_READ_JUMP_HERE,
                      "</button>",
                      "</div>",
                      "<div class='bv-status' id='bvFindStat'>",
                      D_WEB_READ_LOADING,
                      "</div>",
                      "<div class='bv-status' id='bvJumpStat'></div>"})
          .c_str();

  // Jump-by-page form (uses the existing /jumppage route).
  out += "<div style='margin-top:14px;border-top:1px solid var(--line-soft);padding-top:12px'>"
         "<form method='POST' action='/jumppage' class='bv-find'>"
         "<input type='hidden' name='id' value='" +
         String(id) +
         Language::join({"'>", "<input type='text' name='page' inputmode='numeric' placeholder='",
                         D_WEB_READ_PAGE_PLACEHOLDER, "' value='"})
             .c_str() +
         String(savedPage) +
         Language::join({"' style='max-width:140px'>", "<button type='submit' class='btn small'>",
                         D_WEB_READ_JUMP_PAGE, "</button>", "<span class='muted small'>",
                         D_WEB_READ_JUMP_HINT, "</span>", "</form></div>"})
             .c_str();

  out += "<div id='bvData' data-id='" + String(id) + "'></div>";
  out += "<div id='bvText' class='bv-text'></div>";
  out += "</div>";

  appendReadScript(out);
  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

void registerFindRoutes() {
  server.on("/read", HTTP_GET, handleReadView);
  server.on("/readbook-text", HTTP_GET, handleReadbookText);
  server.on("/jumpoffset", HTTP_POST, handleJumpOffset);
}
