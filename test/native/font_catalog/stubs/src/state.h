#pragma once
#include <map>
#include <string>
struct FontPreferences {
  std::map<std::string, int> values;
  int getInt(const char *key, int fallback) {
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
  }
  bool getBool(const char* key, bool fallback) { return getInt(key, fallback ? 1 : 0) != 0; }
  size_t putInt(const char *key, int value) {
    values[key] = value;
    return sizeof(int);
  }
};
extern FontPreferences prefs;
