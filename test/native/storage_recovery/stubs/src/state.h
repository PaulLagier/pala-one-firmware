#pragma once
#include "src/config.h"

struct File {
  bool valid = false;
  bool directory = false;
  explicit operator bool() const { return valid; }
  bool isDirectory() const { return directory; }
  File openNextFile() { return {}; }
  void close() {}
};
struct MountResults {
  bool values[8]{};
  unsigned count = 0, next = 0;
  MountResults& operator=(std::initializer_list<bool> results) {
    count = next = 0;
    for (bool value : results) values[count++] = value;
    return *this;
  }
  bool empty() const { return next == count; }
  bool front() const { return values[next]; }
  void pop_front() { ++next; }
};
struct FakeFS {
  MountResults mounts;
  unsigned begins = 0, ends = 0, formats = 0, mkdirs = 0;
  bool formatResult = true, mkdirResult = true, booksExists = false;
  bool booksReadable = true, booksDirectory = true;
  bool begin(bool formatOnFail) {
    if (formatOnFail) ++formats; // Makes accidental auto-format visible.
    ++begins;
    if (mounts.empty()) return false;
    bool result = mounts.front(); mounts.pop_front(); return result;
  }
  void end() { ++ends; }
  bool format() { ++formats; return formatResult; }
  bool exists(const String&) const { return booksExists; }
  bool mkdir(const String&) { ++mkdirs; return mkdirResult; }
  File open(const String&) { return {booksReadable, booksDirectory}; }
  size_t totalBytes() const { return 100; }
  size_t usedBytes() const { return 10; }
};
inline FakeFS FS;
struct SerialSink { void println(const char*) {} };
inline SerialSink Serial;
