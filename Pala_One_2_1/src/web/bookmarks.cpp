#include "src/web/bookmarks.h"

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h"
#include "src/pure/paths.h"         // stripTxtExt, lastPathComponent
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"
#include "src/ui/text.h"            // resolveBookmarkOffset, extractPageText, pageOffsetForPage
#include "src/web/chrome.h"

// One book page rendered as plain text — used by the bookmark export
// download. Opens the file, locates page N's offset, captures one page's
// worth of lines into a String. Single-caller helper; lives here rather than
// in text.cpp because it's pure web concern (the "Open failed" / "(empty)"
// strings are user-facing for this download flow).
static String readPageTextForWeb(const String& path, int page) {
  File f = FS.open(path, "r");
  if (!f) return String(D_WEB_BOOKMARK_OPEN_FAILED_DOT);
  uint32_t off = pageOffsetForPage(f, path, page);
  String out;
  out.reserve(900);
  (void)extractPageText(f, off, out);
  f.close();
  out.trim();
  if (out.length() == 0) out = D_WEB_BOOKMARK_PAGE_EMPTY;
  return out;
}

static void handleBookmarksWeb() {
  String out = webPageStart(
    D_WEB_BOOKMARKS_HEADING,
    D_WEB_BOOKMARKS_SUBTITLE,
    "<a href='/'>" D_WEB_NAV_HOME "</a><a href='/files'>" D_WEB_NAV_FILES "</a><a href='/settings'>" D_WEB_NAV_SETTINGS "</a>",
    true
  );

  if (g_library.bookCount == 0) out += "<div class='card'><p class='muted'>" D_WEB_NO_BOOKS_YET "</p></div>";

  for (int i = 0; i < g_library.bookCount; i++) {
    String filePath = String(g_library.books[i].path);
    String key = prefKeyForBook(filePath);
    uint16_t pages[MAX_BOOKMARKS];
    uint32_t offsets[MAX_BOOKMARKS];
    uint8_t count = loadBookmarksForKey(key, pages, offsets);

    out += "<div class='card'><h2>";
    out += htmlEscape(String(g_library.books[i].name));
    out += "</h2>";

    if (count == 0) {
      out += "<p class='muted'>" D_WEB_NO_BOOKMARKS_CARD "</p></div>";
      continue;
    }

    File f = FS.open(filePath, "r");
    if (!f) {
      out += "<p class='muted'>" D_WEB_BOOKMARKS_OPEN_FAILED_CARD "</p></div>";
      continue;
    }

    out += "<ul class='list'>";

    for (int j = 0; j < count; j++) {
      int targetPage = (int)pages[j];
      uint32_t pageOff = resolveBookmarkOffset(filePath, (uint16_t)targetPage, offsets[j]);
      FileReadStream fs(f);
      String sn = readBookmarkLabelAtOffset(fs, pageOff, targetPage);
      out += "<li><div class='row'><div><div class='pill'>" D_WEB_BOOKMARK_PILL_PREFIX;
      out += String(j + 1);
      out += "</div><p class='meta' style='margin-top:8px'>";
      out += htmlEscape(sn);
      out += "</p></div><div><a class='link' href='/viewbm?book=" + String(i) + "&idx=" + String(j) + "'>" D_WEB_BOOKMARK_VIEW "</a> | ";
      out += "<form method='POST' action='/delbm' style='display:inline'>";
      out += "<input type='hidden' name='book' value='" + String(i) + "'>";
      out += "<input type='hidden' name='idx' value='" + String(j) + "'>";
      out += "<button type='submit' class='btn secondary' style='padding:4px 8px;font-size:13px' onclick=\"return confirm('" D_WEB_CONFIRM_DELETE_BOOKMARK "')\">" D_WEB_DELETE_BUTTON "</button>";
      out += "</form></div></div></li>";
    }

    out += "</ul><div class='actions'><a class='btn secondary' href='/exportbm?book=" + String(i) + "'>" D_WEB_BOOKMARK_DOWNLOAD_ALL "</a></div></div>";
    f.close();
  }

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleDeleteBookmarkWeb() {
  if (!server.hasArg("book") || !server.hasArg("idx")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_BOOK_IDX);
    return;
  }

  int b   = server.arg("book").toInt();
  int idx = server.arg("idx").toInt();
  if (b < 0 || b >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_BOOK);
    return;
  }

  String key = prefKeyForBook(String(g_library.books[b].path));
  uint16_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t count = loadBookmarksForKey(key, pages, offsets);
  if (idx < 0 || idx >= count) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_IDX);
    return;
  }

  for (int i = idx + 1; i < count; i++) {
    pages[i - 1]   = pages[i];
    offsets[i - 1] = offsets[i];
  }
  count--;
  saveBookmarksForKey(key, pages, offsets, count);

  server.sendHeader("Location", "/bookmarks");
  server.send(302, "text/plain", "");
}

static void handleViewBookmarkWeb() {
  if (!server.hasArg("book") || !server.hasArg("idx")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_BOOK_IDX);
    return;
  }

  int b   = server.arg("book").toInt();
  int idx = server.arg("idx").toInt();
  if (b < 0 || b >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_BOOK);
    return;
  }

  String key = prefKeyForBook(String(g_library.books[b].path));
  uint16_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t count = loadBookmarksForKey(key, pages, offsets);
  if (idx < 0 || idx >= count) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_IDX);
    return;
  }

  int page = (int)pages[idx];
  String filePath = String(g_library.books[b].path);
  File vf = FS.open(filePath, "r");
  String txt;
  if (!vf) {
    txt = D_WEB_BOOKMARK_OPEN_FAILED_DOT;
  } else {
    uint32_t off = resolveBookmarkOffset(filePath, (uint16_t)page, offsets[idx]);
    txt.reserve(900);
    (void)extractPageText(vf, off, txt);
    vf.close();
    txt.trim();
    if (txt.length() == 0) txt = D_WEB_BOOKMARK_PAGE_EMPTY;
  }
  String out = webPageStart(
    D_WEB_BOOKMARK_VIEW_HEADING,
    D_WEB_BOOKMARK_VIEW_SUBTITLE,
    "<a href='/bookmarks'>" D_WEB_BOOKMARK_VIEW_BACK_NAV "</a><a href='/files'>" D_WEB_NAV_FILES "</a><a href='/'>" D_WEB_NAV_HOME "</a>",
    true
  );

  out += "<div class='card'><h2>";
  out += htmlEscape(String(g_library.books[b].name));
  out += "</h2><p class='muted'>" D_WEB_BOOKMARK_PILL_PREFIX;
  out += String(idx + 1);
  out += "</p><pre class='pre'>";
  out += htmlEscape(txt);
  out += "</pre><div class='actions'><a class='btn secondary' href='/exportbm?book=" + String(b) + "'>" D_WEB_BOOKMARK_DOWNLOAD_ALL "</a></div></div>";
  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleExportBookmarksWeb() {
  if (!server.hasArg("book")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_BOOK);
    return;
  }

  int b = server.arg("book").toInt();
  if (b < 0 || b >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_BOOK);
    return;
  }

  String filePath = String(g_library.books[b].path);
  String key = prefKeyForBook(filePath);
  uint16_t pages[MAX_BOOKMARKS];
  uint32_t offsets[MAX_BOOKMARKS];
  uint8_t count = loadBookmarksForKey(key, pages, offsets);

  if (count == 0) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_NO_BOOKMARKS_THIS_BOOK);
    return;
  }

  File f = FS.open(filePath, "r");
  if (!f) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_BOOKMARKS_OPEN_FAILED_CARD);
    return;
  }

  String exportName = stripTxtExt(lastPathComponent(filePath));
  exportName.replace(' ', '_');
  exportName += "_bookmarks.txt";

  String out;
  out.reserve(8192);

  out += D_WEB_BMEXPORT_BOOK;
  out += stripTxtExt(lastPathComponent(filePath));
  out += "\n";

  out += D_WEB_BMEXPORT_BOOKMARKS;
  out += String(count);
  out += "\n\n";

  for (int i = 0; i < count; i++) {
    int targetPage = (int)pages[i];
    uint32_t pageOff = resolveBookmarkOffset(filePath, (uint16_t)targetPage, offsets[i]);
    FileReadStream fs(f);
    String label = readBookmarkLabelAtOffset(fs, pageOff, targetPage);
    String txt = readPageTextForWeb(filePath, targetPage);

    out += "==================================================\n";
    out += D_WEB_BMEXPORT_BOOKMARK_LBL;
    out += String(i + 1);
    out += "\n";
    out += label;
    out += "\n";
    out += "--------------------------------------------------\n";
    out += txt;
    out += "\n\n";
  }

  f.close();

  server.sendHeader(
    "Content-Disposition",
    String("attachment; filename=\"") + exportName + "\""
  );
  server.send(200, "text/plain; charset=utf-8", out);
}

void registerBookmarksRoutes() {
  server.on("/bookmarks", HTTP_GET,  handleBookmarksWeb);
  server.on("/viewbm",    HTTP_GET,  handleViewBookmarkWeb);
  server.on("/delbm",     HTTP_POST, handleDeleteBookmarkWeb);   // POST: destructive
  server.on("/exportbm",  HTTP_GET,  handleExportBookmarksWeb);
}
