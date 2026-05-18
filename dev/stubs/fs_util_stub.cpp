// Host-build stub for src/storage/fs_util.h.
// Uses the LittleFS stub directly so create/delete operations affect dev/fs/.

#include "src/storage/fs_util.h"

bool fsBegin() { return true; }

size_t fsTotalBytesSafe() { return LittleFS.totalBytes(); }
size_t fsUsedBytesSafe()  { return LittleFS.usedBytes();  }
size_t fsFreeBytesSafe()  {
  size_t t = fsTotalBytesSafe(), u = fsUsedBytesSafe();
  return (t >= u) ? (t - u) : 0;
}

void ensureBooksDir() {
  if (!FS.exists("/books")) FS.mkdir("/books");
}

bool ensureDirRecursive(const String& path) {
  if (path.length() == 0 || path == "/") return true;
  if (FS.exists(path)) return true;
  int slash = path.lastIndexOf('/');
  if (slash > 0) {
    String parent = path.substring(0, slash);
    if (parent.length() > 0 && !FS.exists(parent)) {
      if (!ensureDirRecursive(parent)) return false;
    }
  }
  return FS.mkdir(path);
}

bool isDirEmpty(const String& path) {
  File dir = FS.open(path);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return false;
  }
  File f = dir.openNextFile();
  bool empty = !f;
  if (f) f.close();
  dir.close();
  return empty;
}
