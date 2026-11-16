#pragma once
#include <map>
#include <string>
struct LockPrefs {
  std::map<std::string, int> values;
  int getInt(const char *key, int fallback) {
    auto p = values.find(key);
    return p == values.end() ? fallback : p->second;
  }
  bool getBool(const char *key, bool fallback) { return getInt(key, fallback) != 0; }
  void putInt(const char *key, int value) { values[key] = value; }
  void putBool(const char *key, bool value) { putInt(key, value); }
};
extern LockPrefs prefs;
