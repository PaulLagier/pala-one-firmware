#ifndef PALA_PURE_LIBRARY_NAV_H
#define PALA_PURE_LIBRARY_NAV_H

#include "arduino_compat.h"
#include "../config.h"

// ============================================================================
//  Library catalog and menu-list assembly.
//
//  `Catalog` is the inventory of books + folders discovered on disk under
//  `/books/**`. Pure data — no `File`, no `String`, no Arduino types.
//  Populated by `loadBooks()` in `storage/library.cpp`.
//
//  `buildLibraryEntries` flattens the catalog + folder-expansion state +
//  caller-supplied system entries into the menu rows the library screen
//  draws. Pure logic — testable in the host build.
// ============================================================================

struct BookInfo {
  char   name[80];
  char   path[96];
  size_t size;
  char   folder[MAX_FOLDER_PATH + 1];
};

struct Catalog {
  BookInfo books[MAX_BOOKS];
  int      bookCount = 0;

  char folders[MAX_FOLDERS][MAX_FOLDER_PATH + 1];
  int  folderCount = 0;
};

enum LibraryEntryType {
  LIB_ENTRY_FOLDER,
  LIB_ENTRY_BOOK,
  LIB_ENTRY_BOOKMARKS,
  LIB_ENTRY_LIST,
  LIB_ENTRY_APPS,
  LIB_ENTRY_STATISTICS,
  LIB_ENTRY_ABOUT,
  LIB_ENTRY_UPDATE,
  LIB_ENTRY_UPLOAD
};

// Checks whether `type` is a valid library entry. 
// Set `includeSystemEntries` to `true` if the check should also include the LIBRARY and BOOK entries.
bool isValidLibEntry(LibraryEntryType type, bool includeSystemEntries = false);

// One row in the assembled menu. `ref` is a catalog book/folder index for
// BOOK/FOLDER, and -1 (unused) for system entries. `depth` indents nested
// folder/book rows.
struct LibEntry {
  LibraryEntryType type;
  int              ref;
  int              depth;
};

// Assemble the library menu list. Walks the folder tree recursively (each
// folder followed by its books and subfolders, when expanded), then root
// books, then the caller-supplied system entries in given order. Writes up
// to `outCap` rows into `out` and returns the count actually written.
//
// `folderExpanded` must be at least `cat.folderCount` long; entry `i`
// indicates whether `cat.folders[i]` is currently expanded.
//
// `systemEntries` is the list of system entries (Bookmarks/List/About/
// Upload, in any order the caller wants — the screen decides which apply).
int buildLibraryEntries(
    const Catalog& cat,
    const bool* folderExpanded,
    const LibraryEntryType* systemEntries, int systemCount,
    LibEntry* out, int outCap);

#endif  // PALA_PURE_LIBRARY_NAV_H
