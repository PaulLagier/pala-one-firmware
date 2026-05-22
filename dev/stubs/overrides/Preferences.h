#pragma once

// Host-build stub for Arduino's <Preferences.h>.
// Provides the subset of the Preferences API used by PreferencesStore.

#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include "src/pure/arduino_compat.h"

class Preferences {
public:
  int getInt(const char* key, int def = 0) {
    auto it = bytes_.find(key);
    if (it == bytes_.end() || it->second.size() != sizeof(int)) {
      std::fprintf(stderr, "  [prefs] getInt  %s -> %d (default)\n", key, def);
      return def;
    }
    int v; std::memcpy(&v, it->second.data(), sizeof(v));
    std::fprintf(stderr, "  [prefs] getInt  %s -> %d\n", key, v);
    return v;
  }
  void putInt(const char* key, int v) {
    std::fprintf(stderr, "  [prefs] putInt  %s = %d\n", key, v);
    auto& b = bytes_[key]; b.resize(sizeof(v)); std::memcpy(b.data(), &v, sizeof(v));
  }

  size_t getBytes(const char* key, void* buf, size_t maxLen) {
    auto it = bytes_.find(key);
    if (it == bytes_.end()) return 0;
    size_t n = it->second.size() < maxLen ? it->second.size() : maxLen;
    std::memcpy(buf, it->second.data(), n);
    return n;
  }
  void putBytes(const char* key, const void* buf, size_t len) {
    auto& v = bytes_[key];
    v.assign((const uint8_t*)buf, (const uint8_t*)buf + len);
  }

  String getString(const char* key, const String& def = String("")) {
    auto it = strs_.find(key);
    return (it == strs_.end()) ? def : String(it->second.c_str());
  }
  void putString(const char* key, const String& v) { strs_[key] = v.c_str(); }

  void remove(const char* key) { bytes_.erase(key); strs_.erase(key); }
  void clear()                 { bytes_.clear();     strs_.clear();    }

private:
  std::map<std::string, std::vector<uint8_t>> bytes_;
  std::map<std::string, std::string>          strs_;
};
