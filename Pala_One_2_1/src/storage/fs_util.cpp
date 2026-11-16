#include "src/storage/fs_util.h"

bool fsBegin() {
  // First try to mount without formatting — protects existing data.
  if (FS.begin(false)) return true;

  // A failed mount does not distinguish a new volume from damaged user data.
  // Only the explicit recovery confirmation may initialize this partition.
  Serial.println("[FS] Mount failed; storage preserved. Explicit recovery required.");
  return false;
}

size_t fsTotalBytesSafe() { return FS.totalBytes(); }
size_t fsUsedBytesSafe()  { return FS.usedBytes(); }
size_t fsFreeBytesSafe() {
  size_t total = fsTotalBytesSafe();
  size_t used = fsUsedBytesSafe();
  return (total >= used) ? (total - used) : 0;
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
