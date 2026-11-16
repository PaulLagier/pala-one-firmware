#pragma once
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include "src/config.h"

// Deterministic adapter for the actual HAL stream, not a second buffer model.
struct File {
  std::string bytes;
  size_t cursor = 0;
  size_t shortLimit = FILE_STREAM_BUF_BYTES;
  unsigned reads = 0;
  unsigned seeks = 0;
  bool failSeek = false;
  bool failRead = false;
  explicit operator bool() const { return true; }
  size_t size() const { return bytes.size(); }
  size_t position() const { return cursor; }
  size_t available() const { return bytes.size() - cursor; }
  bool seek(uint32_t at) {
    ++seeks;
    if (failSeek || at > bytes.size()) return false;
    cursor = at;
    return true;
  }
  size_t read(uint8_t* out, size_t length) {
    ++reads;
    if (failRead) return 0;
    const size_t n = (std::min)({length, shortLimit, available()});
    std::memcpy(out, bytes.data() + cursor, n);
    cursor += n;
    return n;
  }
  int read() { uint8_t byte; return read(&byte, 1) ? byte : -1; }
};
