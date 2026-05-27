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
//  Post-Phase-4: the legacy String-built handlers (chrome, files, bookmarks,
//  list, settings, reset) are gone. The SPA at `/` (served by app.cpp) calls
//  /api/* for state and actions, plus three surviving binary endpoints under
//  the `/screensavers/` and `/upload*` paths for multipart uploads + raw
//  bitmap thumbnails / downloads.
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
