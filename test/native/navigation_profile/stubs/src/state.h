#pragma once
#include <map>
#include <string>
struct ProfilePrefs {
  std::map<std::string,bool> flags;
  bool failWrites = false;
  bool getBool(const char* key, bool fallback) const {
    auto it=flags.find(key); return it==flags.end()?fallback:it->second;
  }
  size_t putBool(const char* key,bool value) {
    if(failWrites) return 0;
    flags[key]=value;return sizeof(bool);
  }
};
extern ProfilePrefs prefs;
