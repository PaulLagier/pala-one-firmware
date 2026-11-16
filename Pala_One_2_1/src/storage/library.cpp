#include "src/storage/library.h"

#include "src/pure/paths.h"
#include "src/state.h"                   // FS macro
#include "src/storage/fs_util.h"         // ensureBooksDir

// The library catalog. Populated by `loadBooks()` from on-disk `/books/**`.
// Navigation state (cursor, folder expansion, derived entry list) lives on
// the library screen — see `ui/screens/library_screen.cpp`.
Catalog g_library;

static void addFolderIfMissing(const String& folderRel) {
  if (folderRel.length() == 0) return;
  for (int i = 0; i < g_library.folderCount; i++) {
    if (strcmp(g_library.folders[i], folderRel.c_str()) == 0) return;
  }
  if (g_library.folderCount < MAX_FOLDERS) {
    strncpy(g_library.folders[g_library.folderCount], folderRel.c_str(), MAX_FOLDER_PATH);
    g_library.folders[g_library.folderCount][MAX_FOLDER_PATH] = '\0';
    g_library.folderCount++;
  }
}

static void sortFolders() {
  constexpr size_t kFolderBuf = MAX_FOLDER_PATH + 1;
  for (int i = 0; i < g_library.folderCount - 1; i++) {
    for (int j = i + 1; j < g_library.folderCount; j++) {
      if (strcmp(g_library.folders[j], g_library.folders[i]) < 0) {
        char tmp[kFolderBuf];
        memcpy(tmp, g_library.folders[i], kFolderBuf);
        memcpy(g_library.folders[i], g_library.folders[j], kFolderBuf);
        memcpy(g_library.folders[j], tmp, kFolderBuf);
      }
    }
  }
}

static void sortBooks() {
  for (int i = 0; i < g_library.bookCount - 1; i++) {
    for (int j = i + 1; j < g_library.bookCount; j++) {
      if (strcmp(g_library.books[j].name, g_library.books[i].name) < 0) {
        BookInfo tmp = g_library.books[i];
        g_library.books[i] = g_library.books[j];
        g_library.books[j] = tmp;
      }
    }
  }
}

static void scanBooksRecursive(const String& absDir, const String& relDir) {
  File dir = FS.open(absDir);
  if (!dir || !dir.isDirectory()) return;

  File f = dir.openNextFile();
  while (f) {
    String entryName = String(f.name());
    String absPath = entryName.startsWith("/") ? entryName : (absDir + "/" + entryName);
    String leaf = lastPathComponent(absPath);

    if (f.isDirectory()) {
      String childRel = relDir.length() ? (relDir + "/" + leaf) : leaf;
      addFolderIfMissing(childRel);
      scanBooksRecursive(absPath, childRel);
    } else if (g_library.bookCount < MAX_BOOKS && (absPath.endsWith(".txt") || absPath.endsWith(".pbt"))) {
      String relFile = relDir.length() ? (relDir + "/" + leaf) : leaf;
      BookInfo& b = g_library.books[g_library.bookCount];

      strncpy(b.path, absPath.c_str(), 95);
      b.path[95] = '\0';
      strncpy(b.folder, relDir.c_str(), MAX_FOLDER_PATH);
      b.folder[MAX_FOLDER_PATH] = '\0';

      String pretty = prettyRelativeLabel(relFile);
      strncpy(b.name, pretty.c_str(), 79);
      b.name[79] = '\0';
      b.size = f.size();
      g_library.bookCount++;
    }

    f.close();
    f = dir.openNextFile();
  }

  dir.close();
}

void loadBooks() {
  g_library.bookCount = 0;
  g_library.folderCount = 0;

  ensureBooksDir();
  scanBooksRecursive("/books", "");
  sortFolders();
  sortBooks();
}

const char* bookPath(int idx) {
  if (idx < 0 || idx >= g_library.bookCount) return nullptr;
  return g_library.books[idx].path;
}

int bookIndexForPath(const String& path) {
  for (int i = 0; i < g_library.bookCount; i++) {
    if (strcmp(g_library.books[i].path, path.c_str()) == 0) return i;
  }
  return -1;
}