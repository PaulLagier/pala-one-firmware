#include "src/web/api_reset.h"

#include "src/state.h"
#include "src/storage/fs_util.h"
#include "src/storage/library.h"
#include "src/ui/screens/library_screen.h"   // resetLibraryNav
#include "src/ui/toast.h"

// Same factory-reset core as the legacy handler in reset.cpp — keep the
// behaviour bit-identical so the SPA-side cutover is a pure transport
// change. When the legacy route is deleted in Phase 4, fold this into a
// single source of truth.
static void doFactoryReset() {
  Toast::reset();
  resetLibraryNav();

  prefs.clear();
  FS.end();
  delay(100);
  FS.format();
  delay(200);
  if (!FS.begin(true)) return;
  ensureBooksDir();
  loadBooks();
}

static void handleApiReset() {
  doFactoryReset();
  // Caller treats any 2xx as success; the JSON payload is just a stable
  // shape for future expansion (e.g. fresh storage stats).
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

void registerApiResetRoutes() {
  server.on("/api/reset", HTTP_POST, handleApiReset);
}
