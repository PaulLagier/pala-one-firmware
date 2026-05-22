#include "src/web/web.h"

#include "src/web/apps_upload.h"
#include "src/web/bookmarks.h"
#include "src/web/chrome.h"
#include "src/web/files.h"
#include "src/web/list.h"
#include "src/web/reset.h"
#include "src/web/settings.h"
#include "src/web/upload.h"

// ============================================================================
//  Web routes — split per topic across the files in this directory. This
//  function is the single entry point called from setup() to mount them all.
//  Each `register*Routes` is an `HTTP_GET`/`HTTP_POST` registration on the
//  shared global `server`.
// ============================================================================
void registerWebRoutes() {
  registerChromeRoutes();      // /style.css
  registerFilesRoutes();       // /, /files, /del, /mkdir, /rmdir, /move, /jumppage
  registerBookmarksRoutes();   // /bookmarks, /viewbm, /delbm, /exportbm
  registerListRoutes();        // /list, /list-clear-done
  registerSettingsRoutes();    // /settings, /del-sleep
  registerUploadRoutes();      // /upload, /upload-sleep
  registerAppUploadRoutes();   // /upload-app
  registerResetRoutes();       // /reset
}
