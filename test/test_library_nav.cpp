#include <cstring>

#include "test_framework.h"
#include "pure/library_nav.h"

namespace {

// Tiny builder helpers — keep tests readable when constructing fixtures.

void addFolder(Catalog& cat, const char* relPath) {
  std::strncpy(cat.folders[cat.folderCount], relPath, 63);
  cat.folders[cat.folderCount][63] = '\0';
  cat.folderCount++;
}

void addBook(Catalog& cat, const char* name, const char* path, const char* folder) {
  BookInfo& b = cat.books[cat.bookCount];
  std::strncpy(b.name,   name,   79); b.name[79] = '\0';
  std::strncpy(b.path,   path,   95); b.path[95] = '\0';
  std::strncpy(b.folder, folder, 63); b.folder[63] = '\0';
  b.size = 0;
  cat.bookCount++;
}

}  // namespace

TEST_CASE("buildLibraryEntries: empty catalog yields only system entries") {
  Catalog cat;
  bool expanded[MAX_FOLDERS] = {false};
  LibraryEntryType sys[] = { LIB_ENTRY_BOOKMARKS, LIB_ENTRY_ABOUT };
  LibEntry out[MAX_LIBRARY_ENTRIES];

  int n = buildLibraryEntries(cat, expanded, sys, 2, out, MAX_LIBRARY_ENTRIES);
  REQUIRE(n == 2);
  CHECK(out[0].type == LIB_ENTRY_BOOKMARKS);
  CHECK(out[0].depth == 0);
  CHECK(out[0].ref == -1);
  CHECK(out[1].type == LIB_ENTRY_ABOUT);
}

TEST_CASE("buildLibraryEntries: root books appear after folder tree") {
  Catalog cat;
  addFolder(cat, "fiction");
  addBook(cat, "rootbook.txt", "/books/rootbook.txt", "");
  addBook(cat, "novel.txt",    "/books/fiction/novel.txt", "fiction");

  bool expanded[MAX_FOLDERS] = {false};   // fiction collapsed
  LibEntry out[MAX_LIBRARY_ENTRIES];
  int n = buildLibraryEntries(cat, expanded, nullptr, 0, out, MAX_LIBRARY_ENTRIES);

  // Folder header first (collapsed → no children), then root book.
  REQUIRE(n == 2);
  CHECK(out[0].type == LIB_ENTRY_FOLDER);
  CHECK(out[0].ref == 0);    // fiction
  CHECK(out[0].depth == 0);
  CHECK(out[1].type == LIB_ENTRY_BOOK);
  CHECK(out[1].ref == 0);    // rootbook (was added first → catalog index 0)
  CHECK(out[1].depth == 0);
}

TEST_CASE("buildLibraryEntries: expanded folder shows its books with depth+1") {
  Catalog cat;
  addFolder(cat, "fiction");
  addBook(cat, "a.txt", "/books/fiction/a.txt", "fiction");
  addBook(cat, "b.txt", "/books/fiction/b.txt", "fiction");

  bool expanded[MAX_FOLDERS] = {false};
  expanded[0] = true;  // fiction expanded
  LibEntry out[MAX_LIBRARY_ENTRIES];
  int n = buildLibraryEntries(cat, expanded, nullptr, 0, out, MAX_LIBRARY_ENTRIES);

  REQUIRE(n == 3);
  CHECK(out[0].type == LIB_ENTRY_FOLDER);
  CHECK(out[0].depth == 0);
  CHECK(out[1].type == LIB_ENTRY_BOOK);
  CHECK(out[1].ref == 0);     // "a"
  CHECK(out[1].depth == 1);
  CHECK(out[2].type == LIB_ENTRY_BOOK);
  CHECK(out[2].ref == 1);     // "b"
  CHECK(out[2].depth == 1);
}

TEST_CASE("buildLibraryEntries: subfolders nest under parent at depth+1") {
  Catalog cat;
  addFolder(cat, "fiction");
  addFolder(cat, "fiction/short");
  addBook(cat, "novel.txt",  "/books/fiction/novel.txt",       "fiction");
  addBook(cat, "story.txt",  "/books/fiction/short/story.txt", "fiction/short");

  bool expanded[MAX_FOLDERS] = {false};
  expanded[0] = true; expanded[1] = true;
  LibEntry out[MAX_LIBRARY_ENTRIES];
  int n = buildLibraryEntries(cat, expanded, nullptr, 0, out, MAX_LIBRARY_ENTRIES);

  // fiction (d0), novel (d1), fiction/short (d1), story (d2)
  REQUIRE(n == 4);
  CHECK(out[0].type == LIB_ENTRY_FOLDER); CHECK(out[0].depth == 0);
  CHECK(out[1].type == LIB_ENTRY_BOOK);   CHECK(out[1].depth == 1);
  CHECK(out[2].type == LIB_ENTRY_FOLDER); CHECK(out[2].depth == 1);
  CHECK(out[3].type == LIB_ENTRY_BOOK);   CHECK(out[3].depth == 2);
}

TEST_CASE("buildLibraryEntries: collapsed parent hides expanded child") {
  Catalog cat;
  addFolder(cat, "fiction");
  addFolder(cat, "fiction/short");
  addBook(cat, "story.txt", "/books/fiction/short/story.txt", "fiction/short");

  bool expanded[MAX_FOLDERS] = {false};
  expanded[1] = true;     // child marked expanded but parent isn't
  LibEntry out[MAX_LIBRARY_ENTRIES];
  int n = buildLibraryEntries(cat, expanded, nullptr, 0, out, MAX_LIBRARY_ENTRIES);

  // Only the parent folder header. Child folder is hidden, so its
  // expanded flag is irrelevant.
  REQUIRE(n == 1);
  CHECK(out[0].type == LIB_ENTRY_FOLDER);
  CHECK(out[0].ref == 0);
}

TEST_CASE("buildLibraryEntries: system entries appended in given order") {
  Catalog cat;
  bool expanded[MAX_FOLDERS] = {false};
  LibraryEntryType sys[] = {
    LIB_ENTRY_BOOKMARKS, LIB_ENTRY_LIST, LIB_ENTRY_ABOUT, LIB_ENTRY_UPLOAD
  };
  LibEntry out[MAX_LIBRARY_ENTRIES];
  int n = buildLibraryEntries(cat, expanded, sys, 4, out, MAX_LIBRARY_ENTRIES);

  REQUIRE(n == 4);
  CHECK(out[0].type == LIB_ENTRY_BOOKMARKS);
  CHECK(out[1].type == LIB_ENTRY_LIST);
  CHECK(out[2].type == LIB_ENTRY_ABOUT);
  CHECK(out[3].type == LIB_ENTRY_UPLOAD);
  for (int i = 0; i < 4; i++) {
    CHECK(out[i].ref == -1);
    CHECK(out[i].depth == 0);
  }
}

TEST_CASE("buildLibraryEntries: respects outCap, never overflows the buffer") {
  Catalog cat;
  for (int i = 0; i < 5; i++) {
    char p[32]; std::snprintf(p, sizeof(p), "/books/b%d.txt", i);
    char n[16]; std::snprintf(n, sizeof(n), "b%d", i);
    addBook(cat, n, p, "");
  }
  bool expanded[MAX_FOLDERS] = {false};
  LibraryEntryType sys[] = { LIB_ENTRY_BOOKMARKS, LIB_ENTRY_ABOUT };

  // Cap at 3 — we'd have 5 books + 2 system = 7 otherwise.
  LibEntry out[3] = {};
  int n = buildLibraryEntries(cat, expanded, sys, 2, out, 3);

  CHECK_EQ(n, 3);
  // First three are root books; system entries dropped because we ran out.
  CHECK(out[0].type == LIB_ENTRY_BOOK);
  CHECK(out[1].type == LIB_ENTRY_BOOK);
  CHECK(out[2].type == LIB_ENTRY_BOOK);
}

TEST_CASE("buildLibraryEntries: books in catalog order under their folder") {
  Catalog cat;
  addFolder(cat, "f");
  addBook(cat, "z.txt", "/books/f/z.txt", "f");
  addBook(cat, "a.txt", "/books/f/a.txt", "f");

  bool expanded[MAX_FOLDERS] = {false};
  expanded[0] = true;
  LibEntry out[MAX_LIBRARY_ENTRIES];
  int n = buildLibraryEntries(cat, expanded, nullptr, 0, out, MAX_LIBRARY_ENTRIES);

  // The assembler doesn't sort — it walks catalog order. (Sorting is
  // loadBooks's job.) Verify the contract: ref order matches catalog.
  REQUIRE(n == 3);
  CHECK_EQ(out[1].ref, 0);   // z added first
  CHECK_EQ(out[2].ref, 1);   // a added second
}
