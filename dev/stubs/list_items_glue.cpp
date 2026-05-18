// Host implementations of the list_items.h firmware-glue functions
// (normally compiled only under #ifdef ARDUINO).

#include "src/storage/list_items.h"
#include "src/pure/list_codec.h"
#include "map_kv_store.h"

static MapKvStore g_list_kv;

ListState g_list;

void sanitizeListText(String& s) {
  // Mirror the codec sanitizer: strip leading/trailing whitespace, collapse
  // internal runs. For the emulator a simple trim is enough.
  s.trim();
}

void loadListItems() {
  ListData data = loadList(g_list_kv);
  g_list.count = 0;
  g_list.selectedIndex = 0;
  for (int i = 0; i < data.count && i < MAX_LIST_ITEMS; i++) {
    strncpy(g_list.items[i].text, data.items[i].text, MAX_LIST_TEXT);
    g_list.items[i].text[MAX_LIST_TEXT] = '\0';
    g_list.items[i].done = data.items[i].done;
    g_list.count++;
  }
}

void saveListItems() {
  ListData data;
  data.count = g_list.count;
  for (int i = 0; i < g_list.count; i++) {
    strncpy(data.items[i].text, g_list.items[i].text, MAX_LIST_TEXT);
    data.items[i].text[MAX_LIST_TEXT] = '\0';
    data.items[i].done = g_list.items[i].done;
  }
  saveList(g_list_kv, data);
}

bool listHasVisibleItems() {
  for (int i = 0; i < g_list.count; i++) {
    if (g_list.items[i].text[0] != '\0') return true;
  }
  return false;
}
