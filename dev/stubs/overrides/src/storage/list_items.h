#pragma once
// Emulator override: exposes firmware-glue types and functions without #ifdef ARDUINO.
// Definitions live in dev/stubs/list_items_glue.cpp.

#include "src/storage/kv_store.h"
#include "src/pure/list_codec.h"
#include "src/config.h"

// ---------------------------------------------------------------------------
//  Testable KV-store-backed API (identical to real header)
// ---------------------------------------------------------------------------
ListData loadList(KeyValueStore& kv);
void     saveList(KeyValueStore& kv, const ListData& data);

// ---------------------------------------------------------------------------
//  Firmware-glue runtime state — exposed unconditionally in the emulator
// ---------------------------------------------------------------------------
struct ListItem {
  char    text[MAX_LIST_TEXT + 1];
  uint8_t done = 0;
};

struct ListState {
  ListItem items[MAX_LIST_ITEMS];
  int count         = 0;
  int selectedIndex = 0;
};

extern ListState g_list;

void sanitizeListText(String& s);
void loadListItems();
void saveListItems();
bool listHasVisibleItems();
