#include "src/storage/list_items.h"

#include <cstring>

static const char* LIST_KEY = "list_v1";

ListData loadList(KeyValueStore& kv) {
  ListData data;
  uint8_t buf[LIST_ENCODED_MAX_SIZE] = {0};
  size_t got = kv.getBytes(LIST_KEY, buf, sizeof(buf));
  decodeList(buf, got, data);
  return data;
}

void saveList(KeyValueStore& kv, const ListData& data) {
  uint8_t buf[LIST_ENCODED_MAX_SIZE];
  size_t n = encodeList(data, buf);

  // Skip the NVS write if the stored bytes already match. The list screen
  // calls saveListItems on every change AND on screen exit; without this
  // guard, untouched lists still take a flash-write hit on exit.
  uint8_t existing[LIST_ENCODED_MAX_SIZE];
  size_t got = kv.getBytes(LIST_KEY, existing, sizeof(existing));
  if (got == n && std::memcmp(existing, buf, n) == 0) return;

  kv.putBytes(LIST_KEY, buf, n);
}

#ifdef ARDUINO
#include "src/storage/preferences_store.h"
#include "src/state.h"

// The firmware-side working copy. Loaded by `loadListItems()`, saved by
// `saveListItems()`; the list screen reads/writes it for navigation.
ListState g_list;

void sanitizeListText(String& s) {
  sanitizeListItemText(s);
}

void loadListItems() {
  PreferencesStore kv(prefs);
  ListData data = loadList(kv);

  // Reload the items from NVS, but preserve the user's UI cursor across
  // the load (clamped to the new range). The cursor is RAM-only — nothing
  // outside this function should be resetting it on us.
  int prevSelection = g_list.selectedIndex;
  g_list.count = data.count;
  for (int i = 0; i < data.count; i++) {
    std::strncpy(g_list.items[i].text, data.items[i].text, MAX_LIST_TEXT);
    g_list.items[i].text[MAX_LIST_TEXT] = '\0';
    g_list.items[i].done = data.items[i].done;
  }
  if (g_list.count <= 0)              g_list.selectedIndex = 0;
  else if (prevSelection < 0)         g_list.selectedIndex = 0;
  else if (prevSelection >= g_list.count) g_list.selectedIndex = g_list.count - 1;
  else                                g_list.selectedIndex = prevSelection;
}

void saveListItems() {
  ListData data;
  data.count = g_list.count;
  for (int i = 0; i < g_list.count && i < MAX_LIST_ITEMS; i++) {
    std::strncpy(data.items[i].text, g_list.items[i].text, MAX_LIST_TEXT);
    data.items[i].text[MAX_LIST_TEXT] = '\0';
    data.items[i].done = g_list.items[i].done;
  }
  PreferencesStore kv(prefs);
  saveList(kv, data);
}

bool listHasVisibleItems() {
  for (int i = 0; i < g_list.count; i++) {
    if (g_list.items[i].text[0]) return true;
  }
  return false;
}
#endif
