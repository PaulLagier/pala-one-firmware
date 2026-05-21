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
  String subtitle = "Firmware ";
  subtitle += FW_BUILD;
  subtitle += " &middot; ";
  subtitle += String(g_library.bookCount);
  subtitle += " books &middot; Free: ";
  subtitle += humanBytes(fsFreeBytesSafe());
  subtitle += " / ";
  subtitle += humanBytes(fsTotalBytesSafe());

  String out = webPageStart(
    "Pala One",
    subtitle,
    "<a href='/files'>Files</a><a href='/bookmarks'>Bookmarks</a><a href='/list'>List</a><a href='/settings'>Settings</a><a href='/reset'>Factory reset</a>"
  );

  out += storageCardHtml();

  if (fsTotalBytesSafe() == 0 || fsFreeBytesSafe() < 8192) {
    out += "<div class='banner-warn'>&#9888; Storage is not available or almost full. If uploads fail, delete books or use Factory reset from this web UI.</div>";
  }

  out +=
    "<div class='card'><h2>Upload book</h2>"
    "<p class='muted'>Send UTF-8 plain text files to <b>/books</b> on the device, then sort them into folders from the Files page.</p>"
    "<form method='POST' action='/upload' enctype='multipart/form-data' accept-charset='UTF-8' style='margin-top:14px'>"
    "<input type='file' name='file' accept='.txt,text/plain' required>"
    "<div class='actions'><button type='submit'>Upload</button><a class='btn secondary' href='/files'>Manage files</a></div>"
    "</form></div>";

  out +=
    "<div class='card'><h2>Install app</h2>"
    "<p class='muted'>Upload a Pala app binary (<b>.bin</b>) to <b>/apps</b>. The header is validated before commit; only files with the correct magic and API version are accepted. Open <b>Apps</b> from the library to launch.</p>"
    "<form method='POST' action='/upload-app' enctype='multipart/form-data' style='margin-top:14px'>"
    "<input type='file' name='file' accept='.bin' required>"
    "<div class='actions'><button type='submit'>Install app</button></div>"
    "</form></div>";

  out += "<div class='card'><h2>Notes</h2><p class='muted'>Uploaded books are normalized and compacted before saving, so a source TXT can be larger than the final stored file. The reader is optimized for UTF-8 plain text and Latin-based languages.</p></div>";

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleFiles() {
  String out = webPageStart(
    "Files",
    "Manage books, folders and library structure for Pala One.",
    "<a href='/'>Home</a><a href='/bookmarks'>Bookmarks</a><a href='/settings'>Settings</a>",
    true
  );

  out +=
    "<div class='card'><h2>Create folder</h2>"
    "<form method='POST' action='/mkdir' class='stack' accept-charset='UTF-8' style='margin-top:12px'>"
    "<input type='text' name='folder' placeholder='books or classics/english' maxlength='64'>"
    "<div class='actions'><button type='submit'>Create folder</button><span class='muted'>Folders live inside /books.</span></div>"
    "</form></div>";

  out += "<div class='card'><h2>Folders</h2>";
  if (g_library.folderCount == 0) {
    out += "<p class='muted'>No folders yet. Books currently live in the root of /books.</p>";
  } else {
    out += "<ul class='list'>";
    for (int i = 0; i < g_library.folderCount; i++) {
      out += "<li><div class='row'><div><span class='pill'>";
      out += htmlEscape(prettyRelativeLabel(String(g_library.folders[i])));
      out += "</span></div><div><form method='POST' action='/rmdir' style='display:inline'>";
      out += "<input type='hidden' name='folder' value='";
      out += htmlEscape(g_library.folders[i]);
      out += "'><button type='submit' class='btn secondary' onclick=\"return confirm('Delete folder? Only empty folders can be deleted.')\">Delete</button></form></div></div></li>";
    }
    out += "</ul>";
  }
  out += "</div>";

  out += "<div class='card'><h2>Library files</h2>";
  if (g_library.bookCount   >= MAX_BOOKS)   out += "<p style='color:#b91c1c;font-weight:600'>&#9888; Library full (80 books max). Delete books to make room.</p>";
  if (g_library.folderCount >= MAX_FOLDERS) out += "<p style='color:#b91c1c;font-weight:600'>&#9888; Folder limit reached (32 max).</p>";

  if (g_library.bookCount == 0) {
    out += "<p class='muted'>No books uploaded yet.</p>";
  } else {
    out += "<ul class='list'>";
    for (int i = 0; i < g_library.bookCount; i++) {
      String bookPath = String(g_library.books[i].path);
      String folderLabel = g_library.books[i].folder[0] ? prettyRelativeLabel(g_library.books[i].folder) : String("Root");
      int savedPage = savedPageForBookPath(bookPath) + 1;
      if (savedPage < 1) savedPage = 1;

      out += "<li><div class='row'><div><h3>";
      out += htmlEscape(String(g_library.books[i].name));
      out += "</h3><div class='meta'>";
      out += String((int)g_library.books[i].size);
      out += " bytes &middot; folder: ";
      out += htmlEscape(folderLabel);
      out += " &middot; current page: ";
      out += String(savedPage);
      out += "</div>";

      out += "<form method='POST' action='/jumppage' class='stack small' accept-charset='UTF-8' style='margin-top:10px'>";
      out += "<input type='hidden' name='id' value='" + String(i) + "'>";
      out += "<div class='row' style='align-items:end;gap:10px'><div style='flex:1'><input type='text' name='page' value='" + String(savedPage) + "' inputmode='numeric' placeholder='Page'></div><div><button type='submit'>Jump</button></div></div>";
      out += "<div class='muted'>Set the page that should open next on the device.<br><span class='muted'>The first open may take a moment.</span></div></form>";

      out += "<div class='actions small' style='margin-top:8px'><a class='btn secondary small' href='/read?id=" + String(i) + "' style='padding:6px 10px;font-size:13px'>Read &amp; find</a></div>";

      out += "<form method='POST' action='/move' class='stack small' accept-charset='UTF-8' style='margin-top:10px'>";
      out += "<input type='hidden' name='id' value='" + String(i) + "'>";
      out += "<input type='text' name='folder' value='" + htmlEscape(String(g_library.books[i].folder)) + "' placeholder='leave blank for root' maxlength='64'>";
      out += "<div class='actions'><button type='submit'>Move</button><span class='muted'>Use the exact folder path.</span></div></form></div>";
      out += "<div><form method='POST' action='/del' style='display:inline'><input type='hidden' name='id' value='" + String(i) + "'>";
      out += "<button type='submit' class='btn secondary' onclick=\"return confirm('Delete file?')\">Delete</button></form></div></div></li>";
    }
    out += "</ul>";
  }

  out += "</div>";

  out += "<div class='card'><h2>Apps</h2>";
  if (g_apps.count == 0) {
    out += "<p class='muted'>No apps installed.</p>";
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
      out += " bytes &middot; ";
      out += htmlEscape(fileName);
      out += "</div></div><div><form method='POST' action='/del-app' style='display:inline'>";
      out += "<input type='hidden' name='name' value='";
      out += htmlEscape(fileName);
      out += "'><button type='submit' class='btn secondary' onclick=\"return confirm('Delete app?')\">Delete</button></form></div></div></li>";
    }
    out += "</ul>";
  }
  out += "</div>";

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleDelete() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain; charset=utf-8", "missing id");
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", "bad id");
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
    server.send(400, "text/plain; charset=utf-8", "missing folder");
    return;
  }

  String folder = sanitizeFolderInput(server.arg("folder"));
  if (folder.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "bad folder");
    return;
  }
  if (g_library.folderCount >= MAX_FOLDERS) {
    server.send(409, "text/plain; charset=utf-8", "folder limit reached");
    return;
  }

  String fullPath = "/books/" + folder;
  if (!ensureDirRecursive(fullPath)) {
    server.send(500, "text/plain; charset=utf-8", "mkdir failed");
    return;
  }
  loadBooks();   // refresh catalog so the new folder appears

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

static void handleDeleteFolder() {
  if (!server.hasArg("folder")) {
    server.send(400, "text/plain; charset=utf-8", "missing folder");
    return;
  }

  String folder = sanitizeFolderInput(server.arg("folder"));
  if (folder.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "bad folder");
    return;
  }

  String fullPath = "/books/" + folder;
  if (!FS.exists(fullPath)) {
    server.send(404, "text/plain; charset=utf-8", "folder not found");
    return;
  }
  if (!isDirEmpty(fullPath)) {
    server.send(409, "text/plain; charset=utf-8", "folder not empty");
    return;
  }
  if (!FS.rmdir(fullPath)) {
    server.send(500, "text/plain; charset=utf-8", "delete failed");
    return;
  }
  loadBooks();   // refresh catalog so the folder disappears

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

static void handleMoveBook() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain; charset=utf-8", "missing id");
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", "bad id");
    return;
  }

  String oldPath = String(g_library.books[id].path);
  String folder = sanitizeFolderInput(server.arg("folder"));
  String destDir = (folder.length() == 0) ? String("/books") : String("/books/") + folder;

  if (!ensureDirRecursive(destDir)) {
    server.send(500, "text/plain; charset=utf-8", "folder create failed");
    return;
  }

  String newPath = destDir + "/" + lastPathComponent(oldPath);
  if (newPath == oldPath) {
    server.sendHeader("Location", "/files");
    server.send(302, "text/plain", "");
    return;
  }
  if (FS.exists(newPath)) {
    server.send(409, "text/plain; charset=utf-8", "destination exists");
    return;
  }

  // Library entry already cleared g_bookview; no book is "current" here.
  if (!FS.rename(oldPath, newPath)) {
    server.send(500, "text/plain; charset=utf-8", "move failed");
    return;
  }

  migrateBookMetadata(oldPath, newPath);   // NVS keys + page-cache file
  loadBooks();   // refresh catalog so the moved book shows in its new folder

  server.sendHeader("Location", "/files");
  server.send(302, "text/plain", "");
}

static void handleJumpPageWeb() {
  if (!server.hasArg("id") || !server.hasArg("page")) {
    server.send(400, "text/plain; charset=utf-8", "missing id/page");
    return;
  }

  int id = server.arg("id").toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", "bad id");
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
