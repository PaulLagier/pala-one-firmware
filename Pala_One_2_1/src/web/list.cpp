#include "src/web/list.h"

#include "src/config.h"
#include "src/state.h"
#include "src/storage/list_items.h"
#include "src/ui/screen.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screens/list_screen.h"
#include "src/web/chrome.h"

static void handleListWeb() {
  String out = webPageStart(
    D_WEB_LIST_HEADING,
    D_WEB_LIST_SUBTITLE,
    "<a href='/'>" D_WEB_NAV_HOME "</a><a href='/files'>" D_WEB_NAV_FILES "</a><a href='/bookmarks'>" D_WEB_NAV_BOOKMARKS "</a><a href='/settings'>" D_WEB_NAV_SETTINGS "</a>",
    true
  );

  out += "<div class='card'><h2>" D_WEB_LIST_EDIT_HEADING "</h2><p class='muted'>" D_WEB_LIST_EDIT_DESC "</p>";
  out += "<form method='POST' action='/list' class='stack' accept-charset='UTF-8' style='margin-top:12px'>";
  for (int i = 0; i < MAX_LIST_ITEMS; i++) {
    String value   = (i < g_list.count) ? htmlEscape(String(g_list.items[i].text)) : String("");
    String checked = (i < g_list.count && g_list.items[i].done) ? " checked" : "";
    out += "<div class='row' style='align-items:center;gap:10px'><div style='width:26px;text-align:center'><input type='checkbox' name='done" + String(i) + "' value='1'" + checked + "></div><div style='flex:1'><input type='text' name='item" + String(i) + "' value='" + value + "' maxlength='64' placeholder='" D_WEB_LIST_ITEM_PLACEHOLDER "'></div></div>";
  }
  out += "<div class='actions'><button type='submit'>" D_WEB_LIST_SAVE_BUTTON "</button><button type='submit' formaction='/list-clear-done'>" D_WEB_LIST_DELETE_DONE "</button><span class='muted'>" D_WEB_LIST_HINT "</span></div></form></div>";
  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

// Build `g_list` from the form fields and persist. If `dropChecked`, items
// whose checkbox is set are discarded entirely; otherwise their done state
// is recorded. Either way, blank rows are skipped.
static void rebuildListFromForm(bool dropChecked) {
  ListState newList;
  newList.count = 0;
  newList.selectedIndex = 0;

  for (int i = 0; i < MAX_LIST_ITEMS; i++) {
    String text = server.arg(String("item") + String(i));
    sanitizeListText(text);
    if (text.length() == 0) continue;

    bool checked = server.hasArg(String("done") + String(i));
    if (dropChecked && checked) continue;

    strncpy(newList.items[newList.count].text, text.c_str(), MAX_LIST_TEXT);
    newList.items[newList.count].text[MAX_LIST_TEXT] = '\0';
    newList.items[newList.count].done = (checked && !dropChecked) ? 1 : 0;
    newList.count++;
    if (newList.count >= MAX_LIST_ITEMS) break;
  }

  g_list = newList;
  saveListItems();
  if (!listHasVisibleItems() && g_currentScreen == &g_listScreen) {
    g_currentScreen->nextScreen = &g_libraryScreen;
  }
  server.sendHeader("Location", "/list");
  server.send(302, "text/plain", "");
}

static void handleListSaveWeb()      { rebuildListFromForm(false); }
static void handleListClearDoneWeb() { rebuildListFromForm(true);  }

void registerListRoutes() {
  server.on("/list",            HTTP_GET,  handleListWeb);
  server.on("/list",            HTTP_POST, handleListSaveWeb);
  server.on("/list-clear-done", HTTP_POST, handleListClearDoneWeb);
}
