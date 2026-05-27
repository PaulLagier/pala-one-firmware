#include "src/web/web.h"

#include "src/web/api_bookmarks.h"
#include "src/web/api_files.h"
#include "src/web/api_list.h"
#include "src/web/api_reset.h"
#include "src/web/api_screensavers.h"
#include "src/web/api_settings.h"
#include "src/web/app.h"
#include "src/web/apps_upload.h"
#include "src/web/screensavers.h"
#include "src/web/upload.h"

// ============================================================================
//  Web routes — entry point called from setup() to mount everything.
//
//  The SPA at `/` (served by app.cpp) calls /api/* for state and actions.
//  Three companion paths handle bytes that aren't JSON: /upload and
//  /upload-app for multipart file uploads, /screensavers/{thumb,download,
//  upload} for raw bitmap I/O.
// ============================================================================
void registerWebRoutes() {
  registerAppRoutes();             // /, /api/info  (SPA shell + boot info)
  registerApiResetRoutes();        // /api/reset
  registerApiListRoutes();         // /api/list                (GET + POST)
  registerApiBookmarksRoutes();    // /api/bookmarks{,/view,/delete,/export}
  registerApiSettingsRoutes();     // /api/settings + /api/sleep-image/delete
  registerApiFilesRoutes();        // /api/files + /api/books/* + /api/folders/* + /api/apps/delete
  registerApiScreensaversRoutes(); // /api/screensavers + /mode + /delete
  registerUploadRoutes();          // /upload         (book multipart)
  registerAppUploadRoutes();       // /upload-app     (app .bin multipart)
  registerScreensaverRoutes();     // /screensavers/{thumb,download,upload}
}
