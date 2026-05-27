#include "src/web/web.h"

#include "src/web/api_bookmarks.h"
#include "src/web/api_files.h"
#include "src/web/api_list.h"
#include "src/web/api_reset.h"
#include "src/web/api_screensavers.h"
#include "src/web/api_settings.h"
#include "src/web/app.h"
#include "src/web/apps_upload.h"
#include "src/web/bookmarks.h"
#include "src/web/chrome.h"
#include "src/web/files.h"
#include "src/web/list.h"
#include "src/web/reset.h"
#include "src/web/screensavers.h"
#include "src/web/settings.h"
#include "src/web/upload.h"

// ============================================================================
//  Web routes — split per topic across the files in this directory. This
//  function is the single entry point called from setup() to mount them all.
//  Each `register*Routes` is an `HTTP_GET`/`HTTP_POST` registration on the
//  shared global `server`.
//
//  The SPA-style routes (registerAppRoutes — /app, /api/*) live alongside
//  the legacy String-built routes during the web-UI rewrite. Old paths
//  remain reachable until Phase 4 cutover (see plan in commit history).
// ============================================================================
void registerWebRoutes() {
  registerChromeRoutes();      // /style.css
  registerFilesRoutes();       // /, /files, /del, /mkdir, /rmdir, /move, /jumppage
  registerBookmarksRoutes();   // /bookmarks, /viewbm, /delbm, /exportbm
  registerListRoutes();        // /list, /list-clear-done
  registerSettingsRoutes();    // /settings, /del-sleep
  registerScreensaverRoutes(); // /screensavers and subroutes
  registerUploadRoutes();      // /upload, /upload-sleep (legacy)
  registerAppUploadRoutes();   // /upload-app
  registerResetRoutes();       // /reset
  registerAppRoutes();         // /app, /api/info  (SPA rewrite, Phase 0+)
  registerApiResetRoutes();    // /api/reset
  registerApiListRoutes();     // /api/list  (GET + POST)
  registerApiBookmarksRoutes();// /api/bookmarks{,/view,/delete,/export}
  registerApiSettingsRoutes(); // /api/settings  (GET + POST) + /api/sleep-image/delete
  registerApiFilesRoutes();    // /api/files + /api/books/* + /api/folders/* + /api/apps/delete
  registerApiScreensaversRoutes(); // /api/screensavers (GET + mode + delete)
}
