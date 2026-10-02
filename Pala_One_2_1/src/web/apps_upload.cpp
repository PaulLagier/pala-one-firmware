#include "src/web/apps_upload.h"

#include <WiFi.h>

#include "src/config.h"                       // MAX_APPS, MAX_APP_BINARY
#include "pala_app.h"                     // PalaAppHeader, PALA_APP_MAGIC
#include "src/pure/app_header.h"              // validateAppHeader / status enum
#include "src/pure/paths.h"                   // sanitizeUploadedAppFilename
#include "src/state.h"                        // FS, server
#include "src/storage/app_catalog.h"          // g_apps, loadApps
#include "src/storage/fs_util.h"              // fsFreeBytesSafe
#include "src/web/chrome.h"                   // successPage, htmlEscape, humanBytes

// ============================================================================
//  Per-session state. File-static because the route handlers below are the
//  only readers; the screen reaches it only through resetAppUpload().
// ============================================================================
namespace {
struct AppUpload {
  File   tmpFile;
  String tmpPath;
  String finalName;
  bool   ok = false;
  String error;
};

AppUpload s_app;
}  // namespace

void resetAppUpload() {
  if (s_app.tmpFile) s_app.tmpFile.close();
  s_app = AppUpload{};
}

// ============================================================================
//  App .bin upload — single-pass streaming receiver.
//
//  Files land in /apps/<sanitized>.bin.tmp during the transfer; on
//  UPLOAD_FILE_END we read the first sizeof(PalaAppHeader) bytes back
//  off the temp file and run them through `validateAppHeader`. If that
//  passes, we rename the temp into place and refresh `g_apps` so the
//  on-device AppsScreen sees the new entry without a reboot.
//
//  Header validation here mirrors what the on-device loader does
//  (Stage 6). The intent is "fail at upload time, not at launch time" —
//  if the magic or api_version is wrong, we'd rather the browser show
//  the error now than have a useless entry sit in the launcher.
// ============================================================================

static void handleUploadAppDone() {
  AppUpload& s = s_app;
  if (!s.ok) {
    server.send(400, "text/plain; charset=utf-8",
                s.error.length() ? s.error : String(D_WEB_APP_UPLOAD_ERR_FALLBACK));
    return;
  }

  loadApps();  // refresh so AppsScreen sees the new entry on next entry

  String finalPath = String("/apps/") + s.finalName;
  size_t storedSize = 0;
  File stored = FS.open(finalPath, "r");
  if (stored) { storedSize = stored.size(); stored.close(); }

  String inner;
  inner.reserve(900);
  inner += "<div class='card'><h2>" D_WEB_APP_INSTALLED_HEADING "</h2>";
  inner += "<p class='muted'>" D_WEB_APP_INSTALLED_DESC "</p>";
  inner += "<div class='stats'>";
  inner += "<div class='stat'><span class='muted'>" D_WEB_APP_LABEL          "</span><b>" + htmlEscape(s.finalName)      + "</b></div>";
  inner += "<div class='stat'><span class='muted'>" D_WEB_UPLOAD_STORED_SIZE "</span><b>" + humanBytes(storedSize)        + "</b></div>";
  inner += "<div class='stat'><span class='muted'>" D_WEB_APPS_NOW           "</span><b>" + String(g_apps.count)          + "</b></div>";
  inner += "<div class='stat'><span class='muted'>" D_WEB_UPLOAD_FREE_SPACE  "</span><b>" + humanBytes(fsFreeBytesSafe()) + "</b></div>";
  inner += "</div><div class='actions'><a class='btn' href='/'>" D_WEB_UPLOAD_ANOTHER "</a></div></div>";
  inner += storageCardHtml();

  String page = successPage(
    D_WEB_APP_INSTALLED_HEADING,
    D_WEB_APP_INSTALLED_SUBTITLE,
    D_WEB_APP_INSTALLED_BANNER,
    inner
  );
  server.send(200, "text/html; charset=utf-8", page);
}

// Map an AppHeaderStatus into a user-facing message. Mirrors the
// runApp switch in ui/pala_api_impl.cpp; lives here so the upload route
// can present the same diagnostics at install time.
static const char* validationErrorMessage(AppHeaderStatus s, uint32_t fileApiVer) {
  static char buf[64];
  switch (s) {
    case AppHeaderStatus::Ok:             return D_WEB_APP_VALID_OK;
    case AppHeaderStatus::TooSmall:       return D_WEB_APP_VALID_TOO_SMALL;
    case AppHeaderStatus::BadMagic:       return D_WEB_APP_VALID_BAD_MAGIC;
    case AppHeaderStatus::BadEntryOffset: return D_WEB_APP_VALID_BAD_ENTRY;
    case AppHeaderStatus::BadRelocTable:  return D_WEB_APP_VALID_BAD_RELOC;
    case AppHeaderStatus::ApiMismatch:
      snprintf(buf, sizeof(buf), D_WEB_APP_VALID_API_FMT,
               (unsigned)fileApiVer, (unsigned)PALA_API_VERSION);
      return buf;
  }
  return D_WEB_APP_VALID_INVALID;
}

static void handleUploadAppStream() {
  AppUpload& s = s_app;
  HTTPUpload& up = server.upload();

  if (up.status == UPLOAD_FILE_START) {
    s.ok = false;
    s.error = "";
    s.finalName = "";
    s.tmpPath = "";

    // Defensive — protects MAX_APPS check from a stale catalog.
    loadApps();
    if (g_apps.count >= MAX_APPS) {
      s.error = D_WEB_APPS_DIR_FULL;
      return;
    }

    // Pre-check free space against the maximum app size — we don't know
    // the upload's final size yet, but we know it can't exceed
    // MAX_APP_BINARY. Lets us fail fast if storage is nearly full
    // rather than burning the whole transfer.
    if (fsFreeBytesSafe() < MAX_APP_BINARY) {
      s.error = D_WEB_ERR_NOT_ENOUGH_SPACE;
      return;
    }

    // Make sure /apps/ exists; loadApps creates it but we may have
    // bailed out above.
    if (!FS.exists("/apps")) FS.mkdir("/apps");

    s.finalName = sanitizeUploadedAppFilename(up.filename);
    s.tmpPath   = String("/apps/") + s.finalName + ".tmp";

    if (FS.exists(s.tmpPath)) FS.remove(s.tmpPath);
    s.tmpFile = FS.open(s.tmpPath, "w");
    if (!s.tmpFile) {
      s.error = D_WEB_ERR_CANT_CREATE_TEMP_APP;
      s.tmpPath = "";
    }
  }
  else if (up.status == UPLOAD_FILE_WRITE) {
    if (s.error.length() > 0) return;
    if (!s.tmpFile) return;

    // Hard cap on cumulative size during streaming so a runaway upload
    // can't fill the partition before END fires.
    if (s.tmpFile.size() + up.currentSize > MAX_APP_BINARY) {
      s.tmpFile.close();
      if (FS.exists(s.tmpPath)) FS.remove(s.tmpPath);
      s.tmpPath = "";
      s.error = D_WEB_APP_TOO_LARGE;
      return;
    }
    s.tmpFile.write(up.buf, up.currentSize);
  }
  else if (up.status == UPLOAD_FILE_END) {
    if (s.tmpFile) s.tmpFile.close();
    if (s.error.length() > 0 || s.tmpPath.length() == 0) return;

    if (up.totalSize < sizeof(PalaAppHeader) + 4) {
      if (FS.exists(s.tmpPath)) FS.remove(s.tmpPath);
      s.error = D_WEB_APP_BINARY_TOO_SMALL;
      s.tmpPath = "";
      return;
    }

    // Read the header back and validate it with the same pure routine
    // the on-device loader uses, so install-time and load-time agree on
    // what counts as a valid app.
    uint8_t hdrBuf[sizeof(PalaAppHeader)] = {0};
    File vf = FS.open(s.tmpPath, "r");
    bool readOk = false;
    if (vf) {
      readOk = (vf.read(hdrBuf, sizeof(hdrBuf)) == (int)sizeof(hdrBuf));
      vf.close();
    }
    if (!readOk) {
      if (FS.exists(s.tmpPath)) FS.remove(s.tmpPath);
      s.error = D_WEB_APP_CANT_READ_HEADER;
      s.tmpPath = "";
      return;
    }

    uint32_t fileApiVer = 0;
    // We only have the header bytes here, but `validateAppHeader` only
    // touches header fields. Pass the *real* file size so the
    // entry/reloc range checks operate against the true bounds.
    AppHeaderStatus st = validateAppHeader(hdrBuf, (size_t)up.totalSize, &fileApiVer);
    if (st != AppHeaderStatus::Ok) {
      if (FS.exists(s.tmpPath)) FS.remove(s.tmpPath);
      s.error = String(validationErrorMessage(st, fileApiVer));
      s.tmpPath = "";
      return;
    }

    // Commit: atomic rename into the canonical /apps/<name>.bin path.
    String finalPath = String("/apps/") + s.finalName;
    if (FS.exists(finalPath)) FS.remove(finalPath);
    if (FS.rename(s.tmpPath, finalPath)) {
      s.ok = true;
    } else {
      if (FS.exists(s.tmpPath)) FS.remove(s.tmpPath);
      s.error = D_WEB_APP_FINALIZE_FAILED;
    }
    s.tmpPath = "";
  }
  else if (up.status == UPLOAD_FILE_ABORTED) {
    if (s.tmpFile) s.tmpFile.close();
    if (s.tmpPath.length() > 0 && FS.exists(s.tmpPath)) FS.remove(s.tmpPath);
    s.tmpPath = "";
    s.ok = false;
    s.error = D_WEB_APP_UPLOAD_ABORTED;
  }
}

static void handleDownloadApp() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_NAME);
    return;
  }
  String name = server.arg("name");
  if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 || !name.endsWith(".bin")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_INVALID_NAME);
    return;
  }
  String path = String("/apps/") + name;
  File f = FS.open(path, "r");
  if (!f) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_ERR_APP_NOT_FOUND);
    return;
  }

  server.setContentLength(f.size());
  server.sendHeader("Content-Disposition",
                    "attachment; filename=\"" + name + "\"");
  server.send(200, "application/octet-stream", "");

  WiFiClient client = server.client();
  uint8_t buf[512];
  while (f.available()) {
    size_t n = f.read(buf, sizeof(buf));
    if (n > 0) client.write(buf, n);
  }
  f.close();
}

static void handleDeleteApp() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_MISSING_NAME);
    return;
  }
  String name = server.arg("name");
  if (name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 || !name.endsWith(".bin")) {
    server.send(400, "text/plain; charset=utf-8", D_WEB_ERR_INVALID_NAME);
    return;
  }
  String path = String("/apps/") + name;
  if (FS.exists(path)) FS.remove(path);
  loadApps();   // refresh catalog so the next read sees the deletion

  server.sendHeader("Location", "/files");
  server.send(303);
}

void registerAppUploadRoutes() {
  server.on("/upload-app",    HTTP_POST, handleUploadAppDone, handleUploadAppStream);
  server.on("/del-app",       HTTP_POST, handleDeleteApp);    // POST: destructive
  server.on("/download-app",  HTTP_GET,  handleDownloadApp);
}
