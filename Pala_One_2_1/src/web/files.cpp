#include "src/web/files.h"

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h"
#include "src/pure/paths.h"
#include "src/storage/book_metadata.h"
#include "src/storage/fs_util.h"
#include "src/storage/app_catalog.h"
#include "src/storage/library.h"
#include "src/storage/preferences_store.h"
#include "src/ui/text.h"            // pageOffsetForPage
#include "src/web/chrome.h"

// ============================================================================
//  /            — landing page (storage card + upload form)
//  /files       — file/folder browser
//  /del         — POST: delete book
//  /mkdir       — POST: create folder
//  /rmdir       — POST: delete (empty) folder
//  /move        — POST: move book to a different folder
//  /jumppage    — POST: set the page that opens next on device
// ============================================================================

static void handleRoot() {
  String subtitle = D_WEB_HOME_FW_PREFIX;
  subtitle += FW_BUILD;
  subtitle += D_WEB_HOME_MIDDOT_SEP;
  subtitle += String(g_library.bookCount);
  subtitle += D_WEB_HOME_BOOKS_SUFFIX;
  subtitle += D_WEB_HOME_MIDDOT_SEP;
  subtitle += D_WEB_HOME_FREE_LABEL;
  subtitle += humanBytes(fsFreeBytesSafe());
  subtitle += " / ";
  subtitle += humanBytes(fsTotalBytesSafe());

  String out = webPageStart(
    D_WEB_HOME_TITLE,
    subtitle,
    "<a href='/files'>" D_WEB_NAV_FILES "</a><a href='/bookmarks'>" D_WEB_NAV_BOOKMARKS "</a><a href='/list'>" D_WEB_NAV_LIST "</a><a href='/settings'>" D_WEB_NAV_SETTINGS "</a><a href='/reset'>" D_WEB_NAV_FACTORY_RESET "</a>"
  );

  out += storageCardHtml();

  if (fsTotalBytesSafe() == 0 || fsFreeBytesSafe() < 8192) {
    out += "<div class='banner-warn'>" D_WEB_HOME_STORAGE_WARN "</div>";
  }

  out +=
    "<div class='card'><h2>" D_WEB_UPLOAD_BOOK_HEADING "</h2>"
    "<p class='muted'>" D_WEB_UPLOAD_BOOK_DESC "</p>"
    "<form method='POST' action='/upload' enctype='multipart/form-data' accept-charset='UTF-8' style='margin-top:14px'>"
    "<input type='file' name='file' accept='.txt,text/plain' required>"
    "<div class='actions'><button type='submit'>" D_WEB_UPLOAD_BOOK_BUTTON "</button><a class='btn secondary' href='/files'>" D_WEB_MANAGE_FILES_BUTTON "</a></div>"
    "</form></div>";

  out +=
    "<div class='card'><h2>" D_WEB_INSTALL_APP_HEADING "</h2>"
    "<p class='muted'>" D_WEB_INSTALL_APP_DESC "</p>"
    "<form method='POST' action='/upload-app' enctype='multipart/form-data' style='margin-top:14px'>"
    "<input type='file' name='file' accept='.bin' required>"
    "<div class='actions'><button type='submit'>" D_WEB_INSTALL_APP_BUTTON "</button></div>"
    "</form></div>";

  out += "<div class='card'><h2>" D_WEB_NOTES_HEADING "</h2><p class='muted'>" D_WEB_NOTES_DESC "</p></div>";

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleFiles() {
  String out = webPageStart(
    D_WEB_FILES_HEADING,
    D_WEB_FILES_SUBTITLE,
    "<a href='/'>" D_WEB_NAV_HOME "</a><a href='/bookmarks'>" D_WEB_NAV_BOOKMARKS "</a><a href='/settings'>" D_WEB_NAV_SETTINGS "</a>",
    true
  );

  out +=
    "<div class='card'><h2>" D_WEB_CREATE_FOLDER_HEADING "</h2>"
    "<form method='POST' action='/mkdir' class='stack' accept-charset='UTF-8' style='margin-top:12px'>"
    "<input type='text' name='folder' placeholder='" D_WEB_CREATE_FOLDER_PLACEHOLDER "' maxlength='64'>"
    "<div class='actions'><button type='submit'>" D_WEB_CREATE_FOLDER_BUTTON "</button><span class='muted'>" D_WEB_CREATE_FOLDER_HINT "</span></div>"
    "</form></div>";

  out += "<div class='card'><h2>" D_WEB_FOLDERS_HEADING "</h2>";
  if (g_library.folderCount == 0) {
    out += "<p class='muted'>" D_WEB_NO_FOLDERS "</p>";
  } else {
    out += "<ul class='list'>";
    for (int i = 0; i < g_library.folderCount; i++) {
      out += "<li><div class='row'><div><span class='pill'>";
      out += htmlEscape(prettyRelativeLabel(String(g_library.folders[i])));
      out += "</span></div><div><form method='POST' action='/rmdir' style='display:inline'>";
      out += "<input type='hidden' name='folder' value='";
      out += htmlEscape(g_library.folders[i]);
      out += "'><button type='submit' class='btn secondary' onclick=\"return confirm('" D_WEB_CONFIRM_DELETE_FOLDER "')\">" D_WEB_DELETE_BUTTON "</button></form></div></div></li>";
    }
    out += "</ul>";
  }
  out += "</div>";

  out += "<div class='card'><h2>" D_WEB_LIBRARY_FILES_HEADING "</h2>";
  if (g_library.bookCount   >= MAX_BOOKS)   out += "<p style='color:#b91c1c;font-weight:600'>" D_WEB_LIBRARY_FULL_WARN "</p>";
  if (g_library.folderCount >= MAX_FOLDERS) out += "<p style='color:#b91c1c;font-weight:600'>" D_WEB_FOLDER_LIMIT_WARN "</p>";

  if (g_library.bookCount == 0) {
    out += "<p class='muted'>" D_WEB_NO_BOOKS_UPLOADED "</p>";
  } else {
    out += "<ul class='list'>";
    for (int i = 0; i < g_library.bookCount; i++) {
      String bookPath = String(g_library.books[i].path);
      String folderLabel = g_library.books[i].folder[0] ? prettyRelativeLabel(g_library.books[i].folder) : String(D_WEB_BOOK_ROOT);
      int savedPage = savedPageForBookPath(bookPath) + 1;
      if (savedPage < 1) savedPage = 1;

      out += "<li><div class='row'><div><h3>";
      out += htmlEscape(String(g_library.books[i].name));
      out += "</h3><div class='meta'>";
      out += String((int)g_library.books[i].size);
      out += D_WEB_BOOK_BYTES_LABEL D_WEB_BOOK_FOLDER_LABEL;
      out += htmlEscape(folderLabel);
      out += D_WEB_BOOK_CURRENT_PAGE;
      out += String(savedPage);
      out += "</div>";

      out += "<form method='POST' action='/jumppage' class='stack small' accept-charset='UTF-8' style='margin-top:10px'>";
      out += "<input type='hidden' name='id' value='" + String(i) + "'>";
      out += "<div class='row' style='align-items:end;gap:10px'><div style='flex:1'><input type='text' name='page' value='" + String(savedPage) + "' inputmode='numeric' placeholder='" D_WEB_PAGE_PLACEHOLDER "'></div><div><button type='submit'>" D_WEB_JUMP_BUTTON "</button></div></div>";
      out += "<div class='muted'>" D_WEB_JUMP_HINT "<br><span class='muted'>" D_WEB_JUMP_HINT2 "</span></div></form>";

      out += "<form method='POST' action='/move' class='stack small' accept-charset='UTF-8' style='margin-top:10px'>";
      out += "<input type='hidden' name='id' value='" + String(i) + "'>";
      out += "<input type='text' name='folder' value='" + htmlEscape(String(g_library.books[i].folder)) + "' placeholder='" D_WEB_MOVE_PLACEHOLDER "' maxlength='64'>";
      out += "<div class='actions'><button type='submit'>" D_WEB_MOVE_BUTTON "</button><span class='muted'>" D_WEB_MOVE_HINT "</span></div></form></div>";
      out += "<div><form method='POST' action='/del' style='display:inline'><input type='hidden' name='id' value='" + String(i) + "'>";
      out += "<button type='submit' class='btn secondary' onclick=\"return confirm('" D_WEB_CONFIRM_DELETE_FILE "')\">" D_WEB_DELETE_BUTTON "</button></form></div></div></li>";
    }
    out += "</ul>";
  }

  out += "</div>";

  out += "<div class='card'><h2>" D_WEB_APPS_PAGE_HEADING "</h2>";
  if (g_apps.count == 0) {
    out += "<p class='muted'>" D_WEB_NO_APPS_INSTALLED "</p>";
  } else {
    out += "<ul class='list'>";
    for (int i = 0; i < g_apps.count; i++) {
      const char* absPath = g_apps.entries[i].path;
      String fileName = lastPathComponent(String(absPath));
      size_t sz = 0;
      File af = FS.open(absPath, "r");
      if (af) { sz = af.size(); af.close(); }

      out += "<li><div class='row'><div><h3>";
      out += htmlEscape(String(g_apps.entries[i].name));
      out += "</h3><div class='meta'>";
      out += String((int)sz);
      out += D_WEB_BOOK_BYTES_LABEL " &middot; ";
      out += htmlEscape(fileName);
      out += "</div></div><div><form method='POST' action='/del-app' style='display:inline'>";
      out += "<input type='hidden' name='name' value='";
      out += htmlEscape(fileName);
      out += "'><button type='submit' class='btn secondary' onclick=\"return confirm('" D_WEB_CONFIRM_DELETE_APP "')\">" D_WEB_DELETE_BUTTON "</button></form></div></div></li>";
    }
    out += "</ul>";
  }
  out += "</div>";

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleDelete() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_ID);
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return;
  }

  // Library entry already cleared g_bookview; no book is "current" here.
  String path = String(g_library.books[id].path);
  if (FS.exists(path)) FS.remove(path);
  deleteBookMetadata(path);
  loadBooks();   // refresh catalog so the next read sees the deletion

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

static void handleCreateFolder() {
  ensureBooksDir();
  if (!server.hasArg("folder")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_FOLDER);
    return;
  }

  String folder = sanitizeFolderInput(server.arg("folder"));
  if (folder.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_FOLDER);
    return;
  }
  if (g_library.folderCount >= MAX_FOLDERS) {
    server.send(409, "text/plain; charset=utf-8", D_WEB_ERR_FOLDER_LIMIT);
    return;
  }

  String fullPath = "/books/" + folder;
  if (!ensureDirRecursive(fullPath)) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_MKDIR_FAILED);
    return;
  }
  loadBooks();   // refresh catalog so the new folder appears

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

static void handleDeleteFolder() {
  if (!server.hasArg("folder")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_FOLDER);
    return;
  }

  String folder = sanitizeFolderInput(server.arg("folder"));
  if (folder.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_FOLDER);
    return;
  }

  String fullPath = "/books/" + folder;
  if (!FS.exists(fullPath)) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_ERR_FOLDER_NOT_FOUND);
    return;
  }
  if (!isDirEmpty(fullPath)) {
    server.send(409, "text/plain; charset=utf-8", D_WEB_ERR_FOLDER_NOT_EMPTY);
    return;
  }
  if (!FS.rmdir(fullPath)) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_DELETE_FAILED);
    return;
  }
  loadBooks();   // refresh catalog so the folder disappears

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

static void handleMoveBook() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_ID);
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return;
  }

  String oldPath = String(g_library.books[id].path);
  String folder = sanitizeFolderInput(server.arg("folder"));
  String destDir = (folder.length() == 0) ? String("/books") : String("/books/") + folder;

  if (!ensureDirRecursive(destDir)) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_FOLDER_CREATE_FAILED);
    return;
  }

  String newPath = destDir + "/" + lastPathComponent(oldPath);
  if (newPath == oldPath) {
    server.sendHeader("Location", "/files");
    server.send(302, "text/plain", "");
    return;
  }
  if (FS.exists(newPath)) {
    server.send(409, "text/plain; charset=utf-8", D_WEB_ERR_DEST_EXISTS);
    return;
  }

  // Library entry already cleared g_bookview; no book is "current" here.
  if (!FS.rename(oldPath, newPath)) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_MOVE_FAILED);
    return;
  }

  migrateBookMetadata(oldPath, newPath);   // NVS keys + page-cache file
  loadBooks();   // refresh catalog so the moved book shows in its new folder

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

static void handleJumpPageWeb() {
  if (!server.hasArg("id") || !server.hasArg("page")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_ID_PAGE);
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return;
  }

  int page = server.arg("page").toInt();
  if (page < 1) page = 1;
  int zeroBasedPage = page - 1;

  // Library entry already cleared g_bookview. We write both the page number
  // (display hint) and the byte offset (canonical position used by the
  // reader on next open). Computing the offset requires paginating the
  // book at the current font — file open + page walk. Same cost as the
  // bookmark resolve path.
  String path = String(g_library.books[id].path);
  String key = prefKeyForBook(path);
  PreferencesStore kv(prefs);
  saveSavedPage(kv, key, zeroBasedPage);

  File f = FS.open(path, "r");
  if (f) {
    uint32_t offset = pageOffsetForPage(f, path, zeroBasedPage);
    f.close();
    saveSavedOffset(kv, key, offset);
  }

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

void registerFilesRoutes() {
  server.on("/",         HTTP_GET,  handleRoot);
  server.on("/files",    HTTP_GET,  handleFiles);
  server.on("/del",      HTTP_POST, handleDelete);          // POST: prevents accidental deletion via browser prefetch
  server.on("/mkdir",    HTTP_POST, handleCreateFolder);
  server.on("/rmdir",    HTTP_POST, handleDeleteFolder);    // POST: destructive
  server.on("/move",     HTTP_POST, handleMoveBook);
  server.on("/jumppage", HTTP_POST, handleJumpPageWeb);
}
