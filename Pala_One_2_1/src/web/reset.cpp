#include "src/web/reset.h"

#include "src/state.h"
#include "src/storage/fs_util.h"
#include "src/storage/library.h"
#include "src/ui/screens/library_screen.h"   // resetLibraryNav
#include "src/ui/toast.h"
#include "src/web/chrome.h"

static void doFactoryReset() {
  // g_bookview was cleared on library entry (which we passed through to get
  // to upload mode, where this handler is served). The remaining helpers
  // wipe ambient ui/library state — wake state and the rest of NVS are
  // nuked by prefs.clear() below.
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

static void handleResetConfirm() {
  String out = webPageStart(
    D_WEB_RESET_HEADING,
    D_WEB_RESET_SUBTITLE,
    "<a href='/'>" D_WEB_NAV_BACK "</a>"
  );
  out +=
    "<div class='card'><h2>" D_WEB_RESET_CONFIRM_HEADING "</h2>"
    "<p><strong>" D_WEB_RESET_WARNING "</strong></p>"
    "<p class='muted'>" D_WEB_RESET_DETAIL "</p>"
    "<form method='POST' action='/reset' style='margin-top:14px'><button class='danger' type='submit'>" D_WEB_RESET_YES_BUTTON "</button></form>"
    "</div>";
  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleResetDo() {
  doFactoryReset();

  String inner;
  inner.reserve(600);
  inner += "<div class='card'><h2>" D_WEB_RESET_COMPLETE_HEADING "</h2><p class='muted'>" D_WEB_RESET_COMPLETE_DESC "</p><div class='actions'><a class='btn' href='/'>" D_WEB_GO_HOME_BUTTON "</a><a class='btn secondary' href='/files'>" D_WEB_OPEN_FILES_BUTTON "</a></div></div>";
  inner += storageCardHtml();

  String page = successPage(
    D_WEB_RESET_SUCCESS_TITLE,
    D_WEB_RESET_SUCCESS_SUBTITLE,
    D_WEB_RESET_BANNER,
    inner
  );
  server.send(200, "text/html; charset=utf-8", page);
}

void registerResetRoutes() {
  server.on("/reset", HTTP_GET,  handleResetConfirm);
  server.on("/reset", HTTP_POST, handleResetDo);
}
