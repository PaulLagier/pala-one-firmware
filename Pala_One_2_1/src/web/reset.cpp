#include "src/lang/join.h"
#include "src/web/reset.h"
#include "src/state.h"
#include "src/storage/factory_reset.h"
#include "src/web/chrome.h"
#include <esp_system.h>

static void handleResetConfirm() {
  String out = webPageStart(D_WEB_RESET_HEADING, "",
                            Language::join({"<a href='/'>", D_WEB_NAV_BACK, "</a>"}));
  out += Language::join({"<div class='card'><h2>", D_WEB_RESET_CONFIRM_HEADING, "</h2>",
                         "<p><strong>", D_WEB_RESET_WARNING, "</strong></p>", "<p class='muted'>",
                         D_WEB_RESET_DETAIL, "</p>",
                         "<form method='POST' action='/reset' style='margin-top:14px'><button "
                         "class='danger' type='submit'>",
                         D_WEB_RESET_YES_BUTTON, "</button></form>", "</div>"});
  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleResetDo() {
  const auto result = factoryResetConfirmed();
  if (result != FactoryResetResult::Success) {
    String page = webPageStart(D_WEB_RESET_HEADING, D_WEB_RESET_FAILED, "");
    page += Language::join({"<div class='card'><p>", D_WEB_RESET_FAILURE_DESC,
                            "</p><div class='actions'><a class='btn' href='/reset'>",
                            D_RECOVERY_RETRY, "</a></div></div>"});
    page += webPageEnd();
    server.send(500, "text/html; charset=utf-8", page);
    return;
  }
  // Keep the current in-memory language for this final response. The fresh
  // boot reloads all owners from the now-empty settings namespace.
  String page = webPageStart(D_WEB_RESET_COMPLETE_HEADING, D_WEB_RESET_RESTARTING, "");
  page += Language::join({"<div class='card'><p>", D_WEB_RESET_COMPLETE_DESC, "</p></div>"});
  page += webPageEnd();
  server.sendHeader("Connection", "close");
  server.send(200, "text/html; charset=utf-8", page);
  // Leave the network task time to transmit the response. No application
  // loop runs here, so stale RAM counters/settings cannot repopulate NVS.
  delay(1000);
  esp_restart();
}

void registerResetRoutes() {
  server.on("/reset", HTTP_GET, handleResetConfirm);
  server.on("/reset", HTTP_POST, handleResetDo);
}
