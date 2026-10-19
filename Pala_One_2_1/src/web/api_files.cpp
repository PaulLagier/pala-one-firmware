#include "src/web/api_files.h"

#include <ArduinoJson.h>
#include <WiFi.h>   // WiFiClient (download streaming)

#include "src/config.h"
#include "src/state.h"
#include "src/pure/hashing.h"             // prefKeyForBook
#include "src/pure/paths.h"               // sanitizeFolderInput, lastPathComponent
#include "src/storage/app_catalog.h"      // g_apps, loadApps
#include "src/storage/book_metadata.h"    // deleteBookMetadata, migrateBookMetadata,
                                          // savedPageForBookPath, saveSavedPage,
                                          // saveSavedOffset
#include "src/storage/fs_util.h"          // fs*BytesSafe, ensureBooksDir,
                                          // ensureDirRecursive, isDirEmpty
#include "src/storage/library.h"          // g_library, loadBooks
#include "src/storage/preferences_store.h"
#include "src/ui/text.h"                  // pageOffsetForPage

// Roughly bounds the JSON response: ~80 books * ~200 bytes each + folders +
// apps. 16 KB covers a full library comfortably without wasting heap for
// typical small libraries (ArduinoJson grows the doc as needed regardless).
static const size_t kFilesJsonReserve = 16 * 1024;

// ----------------------------------------------------------------------------
//  Small parse helpers (mirror api_bookmarks / api_settings patterns).
// ----------------------------------------------------------------------------
static bool readJsonBody(JsonDocument& doc) {
  const String& body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "empty body");
    return false;
  }
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    String msg = String("bad json: ") + err.c_str();
    server.send(400, "text/plain; charset=utf-8", msg);
    return false;
  }
  return true;
}

static void sendOk() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

// ----------------------------------------------------------------------------
//  GET /api/files  — single-call snapshot used to render the whole screen.
// ----------------------------------------------------------------------------
static void handleApiFilesList() {
  JsonDocument doc;

  // storage
  size_t total = fsTotalBytesSafe();
  size_t used  = fsUsedBytesSafe();
  size_t free  = fsFreeBytesSafe();
  int    pct   = (total == 0) ? 0 : (int)((used * 100UL) / total);
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  JsonObject storage = doc["storage"].to<JsonObject>();
  storage["total"] = (uint32_t)total;
  storage["used"]  = (uint32_t)used;
  storage["free"]  = (uint32_t)free;
  storage["pct"]   = pct;

  // limits
  JsonObject limits = doc["limits"].to<JsonObject>();
  limits["maxBooks"]   = MAX_BOOKS;
  limits["maxFolders"] = MAX_FOLDERS;
  limits["maxApps"]    = MAX_APPS;

  // books
  JsonArray books = doc["books"].to<JsonArray>();
  for (int i = 0; i < g_library.bookCount; i++) {
    JsonObject b = books.add<JsonObject>();
    b["id"]     = i;
    b["name"]   = g_library.books[i].name;
    b["size"]   = (uint32_t)g_library.books[i].size;
    b["folder"] = g_library.books[i].folder;
    // 1-based for display (the on-disk value is 0-based).
    int savedPage = savedPageForBookPath(String(g_library.books[i].path)) + 1;
    if (savedPage < 1) savedPage = 1;
    b["savedPage"] = savedPage;
  }

  // folders
  JsonArray folders = doc["folders"].to<JsonArray>();
  for (int i = 0; i < g_library.folderCount; i++) {
    folders.add(g_library.folders[i]);
  }

  // apps
  JsonArray apps = doc["apps"].to<JsonArray>();
  for (int i = 0; i < g_apps.count; i++) {
    const char* absPath = g_apps.entries[i].path;
    JsonObject a = apps.add<JsonObject>();
    a["name"]     = g_apps.entries[i].name;
    a["path"]     = absPath;
    a["fileName"] = lastPathComponent(String(absPath));
    size_t sz = 0;
    File af = FS.open(absPath, "r");
    if (af) { sz = af.size(); af.close(); }
    a["size"] = (uint32_t)sz;
  }

  String out;
  out.reserve(kFilesJsonReserve);
  serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", out);
}

// ----------------------------------------------------------------------------
//  POST /api/books/delete  body { id }
// ----------------------------------------------------------------------------
static void handleApiBookDelete() {
  JsonDocument doc;
  if (!readJsonBody(doc)) return;
  int id = doc["id"].is<int>() ? doc["id"].as<int>() : -1;
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return;
  }
  String path = String(g_library.books[id].path);
  if (FS.exists(path)) FS.remove(path);
  deleteBookMetadata(path);
  loadBooks();
  sendOk();
}

// ----------------------------------------------------------------------------
//  POST /api/books/move  body { id, folder }
//  Folder string is sanitized server-side. Empty folder = root (/books).
// ----------------------------------------------------------------------------
static void handleApiBookMove() {
  JsonDocument doc;
  if (!readJsonBody(doc)) return;
  int id = doc["id"].is<int>() ? doc["id"].as<int>() : -1;
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return;
  }
  String folderRaw = doc["folder"].is<const char*>() ? doc["folder"].as<const char*>() : "";
  String folder = sanitizeFolderInput(folderRaw);

  String oldPath = String(g_library.books[id].path);
  String destDir = (folder.length() == 0) ? String("/books")
                                          : String("/books/") + folder;
  String newPath = destDir + "/" + lastPathComponent(oldPath);

  // BookInfo::path holds MAX_FILE_NAME bytes including the NUL. Check before
  // creating the folder so a rejected move doesn't leave an empty one behind.
  if (newPath.length() >= MAX_FILE_NAME) {
    char msg[192];
    snprintf(msg, sizeof(msg), D_WEB_ERR_FILE_NAME_TOO_LONG, MAX_FILE_NAME - 1, newPath.c_str());
    server.send(400, "text/plain; charset=utf-8", msg);
    return;
  }

  if (!ensureDirRecursive(destDir)) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_FOLDER_CREATE_FAILED);
    return;
  }
  if (newPath == oldPath) {  // no-op move
    sendOk();
    return;
  }
  if (FS.exists(newPath)) {
    server.send(409, "text/plain; charset=utf-8", D_WEB_ERR_DEST_EXISTS);
    return;
  }
  if (!FS.rename(oldPath, newPath)) {
    server.send(500, "text/plain; charset=utf-8", D_WEB_ERR_MOVE_FAILED);
    return;
  }
  migrateBookMetadata(oldPath, newPath);
  loadBooks();
  sendOk();
}

// ----------------------------------------------------------------------------
//  POST /api/books/jumppage  body { id, page }   (page is 1-based)
//  Persists both the page number (display hint) and the canonical byte
//  offset that the reader uses on next open.
// ----------------------------------------------------------------------------
static void handleApiBookJumpPage() {
  JsonDocument doc;
  if (!readJsonBody(doc)) return;
  int id   = doc["id"].is<int>()   ? doc["id"].as<int>()   : -1;
  int page = doc["page"].is<int>() ? doc["page"].as<int>() : 0;
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return;
  }
  if (page < 1) page = 1;
  int zeroBasedPage = page - 1;

  String path = String(g_library.books[id].path);
  String key  = prefKeyForBook(path);
  PreferencesStore kv(prefs);
  saveSavedPage(kv, key, zeroBasedPage);

  File f = FS.open(path, "r");
  if (f) {
    uint32_t offset = pageOffsetForPage(f, path, zeroBasedPage);
    f.close();
    saveSavedOffset(kv, key, offset);
  }
  sendOk();
}

// ----------------------------------------------------------------------------
//  POST /api/folders/create  body { folder }
// ----------------------------------------------------------------------------
static void handleApiFolderCreate() {
  ensureBooksDir();
  JsonDocument doc;
  if (!readJsonBody(doc)) return;
  String raw = doc["folder"].is<const char*>() ? doc["folder"].as<const char*>() : "";
  String folder = sanitizeFolderInput(raw);
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
  loadBooks();
  sendOk();
}

// ----------------------------------------------------------------------------
//  POST /api/folders/delete  body { folder }   (must be empty)
// ----------------------------------------------------------------------------
static void handleApiFolderDelete() {
  JsonDocument doc;
  if (!readJsonBody(doc)) return;
  String raw = doc["folder"].is<const char*>() ? doc["folder"].as<const char*>() : "";
  String folder = sanitizeFolderInput(raw);
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
  loadBooks();
  sendOk();
}

// ----------------------------------------------------------------------------
//  POST /api/apps/delete  body { name }     (basename of the .bin)
// ----------------------------------------------------------------------------
static void handleApiAppDelete() {
  JsonDocument doc;
  if (!readJsonBody(doc)) return;
  const char* nameC = doc["name"].is<const char*>() ? doc["name"].as<const char*>() : nullptr;
  if (!nameC || !*nameC) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_NAME);
    return;
  }
  String name = nameC;
  // Path-traversal guard: name must be a bare basename, not a path.
  if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 || !name.endsWith(".bin")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_INVALID_NAME);
    return;
  }
  String path = String("/apps/") + name;
  if (FS.exists(path)) FS.remove(path);
  loadApps();
  sendOk();
}

// Stream an open file to the client as a download attachment, then close it.
// Shared by the book and app download handlers. `f` is consumed (closed here).
static void streamFileAsDownload(File& f, const String& filename,
                                 const char* contentType) {
  server.setContentLength(f.size());
  server.sendHeader("Content-Disposition",
                    "attachment; filename=\"" + filename + "\"");
  server.send(200, contentType, "");

  WiFiClient client = server.client();
  uint8_t buf[512];
  while (f.available()) {
    size_t n = f.read(buf, sizeof(buf));
    if (n > 0) client.write(buf, n);
  }
  f.close();
}

// ----------------------------------------------------------------------------
//  GET /api/books/download?id=N  — stream the book's .txt as an attachment.
// ----------------------------------------------------------------------------
static void handleApiBookDownload() {
  if (!server.hasArg("id")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_ID);
    return;
  }
  int id = server.arg("id").toInt();
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_BAD_ID);
    return;
  }
  String path = String(g_library.books[id].path);
  File f = FS.open(path, "r");
  if (!f) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_ERR_NOT_FOUND);
    return;
  }
  streamFileAsDownload(f, lastPathComponent(path), "text/plain; charset=utf-8");
}

// ----------------------------------------------------------------------------
//  GET /api/apps/download?name=X  — stream an app .bin as an attachment.
// ----------------------------------------------------------------------------
static void handleApiAppDownload() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_NAME);
    return;
  }
  String name = server.arg("name");
  // Path-traversal guard: name must be a bare .bin basename.
  if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 || !name.endsWith(".bin")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_INVALID_NAME);
    return;
  }
  File f = FS.open(String("/apps/") + name, "r");
  if (!f) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_ERR_APP_NOT_FOUND);
    return;
  }
  streamFileAsDownload(f, name, "application/octet-stream");
}

void registerApiFilesRoutes() {
  server.on("/api/files",           HTTP_GET,  handleApiFilesList);
  server.on("/api/books/delete",    HTTP_POST, handleApiBookDelete);
  server.on("/api/books/move",      HTTP_POST, handleApiBookMove);
  server.on("/api/books/jumppage",  HTTP_POST, handleApiBookJumpPage);
  server.on("/api/books/download",  HTTP_GET,  handleApiBookDownload);
  server.on("/api/folders/create",  HTTP_POST, handleApiFolderCreate);
  server.on("/api/folders/delete",  HTTP_POST, handleApiFolderDelete);
  server.on("/api/apps/delete",     HTTP_POST, handleApiAppDelete);
  server.on("/api/apps/download",   HTTP_GET,  handleApiAppDownload);
}
