#include "src/web/api_files.h"

#include <ArduinoJson.h>

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
    // 1-based for display, matching the legacy form value.
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
    server.send(400, "text/plain; charset=utf-8", "bad id");
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
    server.send(400, "text/plain; charset=utf-8", "bad id");
    return;
  }
  String folderRaw = doc["folder"].is<const char*>() ? doc["folder"].as<const char*>() : "";
  String folder = sanitizeFolderInput(folderRaw);

  String oldPath = String(g_library.books[id].path);
  String destDir = (folder.length() == 0) ? String("/books")
                                          : String("/books/") + folder;
  if (!ensureDirRecursive(destDir)) {
    server.send(500, "text/plain; charset=utf-8", "create folder failed");
    return;
  }
  String newPath = destDir + "/" + lastPathComponent(oldPath);
  if (newPath == oldPath) {  // no-op move
    sendOk();
    return;
  }
  if (FS.exists(newPath)) {
    server.send(409, "text/plain; charset=utf-8", "destination exists");
    return;
  }
  if (!FS.rename(oldPath, newPath)) {
    server.send(500, "text/plain; charset=utf-8", "move failed");
    return;
  }
  migrateBookMetadata(oldPath, newPath);
  loadBooks();
  sendOk();
}

// ----------------------------------------------------------------------------
//  POST /api/books/jumppage  body { id, page }   (1-based, matches legacy UX)
//  Persists both the page number (display hint) and the canonical byte
//  offset that the reader uses on next open.
// ----------------------------------------------------------------------------
static void handleApiBookJumpPage() {
  JsonDocument doc;
  if (!readJsonBody(doc)) return;
  int id   = doc["id"].is<int>()   ? doc["id"].as<int>()   : -1;
  int page = doc["page"].is<int>() ? doc["page"].as<int>() : 0;
  if (id < 0 || id >= g_library.bookCount) {
    server.send(400, "text/plain; charset=utf-8", "bad id");
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
    server.send(400, "text/plain; charset=utf-8", "bad folder");
    return;
  }
  if (g_library.folderCount >= MAX_FOLDERS) {
    server.send(409, "text/plain; charset=utf-8", "folder limit");
    return;
  }
  String fullPath = "/books/" + folder;
  if (!ensureDirRecursive(fullPath)) {
    server.send(500, "text/plain; charset=utf-8", "mkdir failed");
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
    server.send(500, "text/plain; charset=utf-8", "rmdir failed");
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
    server.send(400, "text/plain; charset=utf-8", "missing name");
    return;
  }
  String name = nameC;
  // Path-traversal guard, matches the legacy handleDeleteApp checks.
  if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 || !name.endsWith(".bin")) {
    server.send(400, "text/plain; charset=utf-8", "invalid name");
    return;
  }
  String path = String("/apps/") + name;
  if (FS.exists(path)) FS.remove(path);
  loadApps();
  sendOk();
}

void registerApiFilesRoutes() {
  server.on("/api/files",           HTTP_GET,  handleApiFilesList);
  server.on("/api/books/delete",    HTTP_POST, handleApiBookDelete);
  server.on("/api/books/move",      HTTP_POST, handleApiBookMove);
  server.on("/api/books/jumppage",  HTTP_POST, handleApiBookJumpPage);
  server.on("/api/folders/create",  HTTP_POST, handleApiFolderCreate);
  server.on("/api/folders/delete",  HTTP_POST, handleApiFolderDelete);
  server.on("/api/apps/delete",     HTTP_POST, handleApiAppDelete);
}
