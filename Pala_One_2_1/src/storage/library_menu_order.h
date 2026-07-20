#ifndef PALA_STORAGE_LIBRARY_MENU_ORDER_H
#define PALA_STORAGE_LIBRARY_MENU_ORDER_H

#include "src/pure/library_nav.h"

namespace LibraryMenuOrder {

static constexpr int kMaxSystemEntries = 7;

void loadSettings();

int count();
LibraryEntryType entryAt(int index);
int copyEntries(LibraryEntryType* out, int outCap);

void setEntries(const LibraryEntryType* entries, int entryCount);
void resetToDefaults();

}  // namespace LibraryMenuOrder

#endif  // PALA_STORAGE_LIBRARY_MENU_ORDER_H
