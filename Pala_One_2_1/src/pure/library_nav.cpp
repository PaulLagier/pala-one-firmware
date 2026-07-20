#include "library_nav.h"

#include "paths.h"   // folderParent

static void addBookEntry(int bookIdx, int depth,
                         LibEntry* out, int& count, int cap) {
  if (count >= cap) return;
  out[count++] = { LIB_ENTRY_BOOK, bookIdx, depth };
}

bool isValidLibEntry(LibraryEntryType type, bool includeSystemEntries) {
  switch (type) {
    case LIB_ENTRY_BOOKMARKS:
    case LIB_ENTRY_LIST:
    case LIB_ENTRY_APPS:
    case LIB_ENTRY_STATISTICS:
    case LIB_ENTRY_ABOUT:
    case LIB_ENTRY_UPDATE:
    case LIB_ENTRY_UPLOAD:
      return true;
    case LIB_ENTRY_FOLDER:
    case LIB_ENTRY_BOOK:
      if (includeSystemEntries) {
        return true;
      }
      return false;
    default:
      return false;
  }
}

static void addFolderTree(const Catalog& cat, const bool* folderExpanded,
                          const String& parent, int depth,
                          LibEntry* out, int& count, int cap) {
  for (int i = 0; i < cat.folderCount && count < cap; i++) {
    String folderRel = String(cat.folders[i]);
    if (folderParent(folderRel) != parent) continue;

    if (count >= cap) return;
    out[count++] = { LIB_ENTRY_FOLDER, i, depth };

    if (!folderExpanded[i]) continue;

    for (int b = 0; b < cat.bookCount && count < cap; b++) {
      if (String(cat.books[b].folder) == folderRel) {
        addBookEntry(b, depth + 1, out, count, cap);
      }
    }

    addFolderTree(cat, folderExpanded, folderRel, depth + 1, out, count, cap);
  }
}

int buildLibraryEntries(
    const Catalog& cat,
    const bool* folderExpanded,
    const LibraryEntryType* systemEntries, int systemCount,
    LibEntry* out, int outCap) {
  int count = 0;

  addFolderTree(cat, folderExpanded, String(""), 0, out, count, outCap);

  for (int b = 0; b < cat.bookCount && count < outCap; b++) {
    if (String(cat.books[b].folder).length() == 0) {
      addBookEntry(b, 0, out, count, outCap);
    }
  }

  for (int i = 0; i < systemCount && count < outCap; i++) {
    out[count++] = { systemEntries[i], -1, 0 };
  }

  return count;
}
