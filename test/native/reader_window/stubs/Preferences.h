#pragma once
#include "Arduino.h"
#include <map>
#include <variant>
#include <vector>
class Preferences {
public:
  bool failBytesWrite = false, failBytesRead = false;
  std::map<std::string, std::variant<int, String, std::vector<uint8_t>>> values;
  bool getBool(const char *key, bool fallback) { return getInt(key, fallback ? 1 : 0) != 0; }
  size_t putBool(const char *key, bool value) { return putInt(key, value ? 1 : 0); }
  bool isKey(const char *key) { return values.count(key); }
  int getInt(const char *key, int fallback) {
    auto i = values.find(key);
    if (i == values.end() || !std::holds_alternative<int>(i->second))
      return fallback;
    return std::get<int>(i->second);}
 size_t putInt(const char* key,int value){values[key]=value;return 4;}
 size_t getBytes(const char *key, void *output, size_t maximum) {
   if (failBytesRead)
     return 0;
   auto i = values.find(key);
   if (i == values.end() || !std::holds_alternative<std::vector<uint8_t>>(i->second))
     return 0;
   const auto &bytes = std::get<std::vector<uint8_t>>(i->second);
   const size_t n = (std::min)(maximum, bytes.size());
   memcpy(output, bytes.data(), n);
   return n;
 }
 size_t getBytesLength(const char *key) {
   auto i = values.find(key);
   return i == values.end() || !std::holds_alternative<std::vector<uint8_t>>(i->second)
              ? 0
              : std::get<std::vector<uint8_t>>(i->second).size();
 }
 size_t putBytes(const char *key, const void *input, size_t size) {
   if (failBytesWrite)
     return 0;
   const auto *bytes = static_cast<const uint8_t *>(input);
   values[key] = std::vector<uint8_t>(bytes, bytes + size);
   return size;
 }
 String getString(const char* key,const String& fallback){auto i=values.find(key);if(i==values.end()||!std::holds_alternative<String>(i->second))return fallback;return std::get<String>(i->second);}
 size_t putString(const char* key,const String& value){values[key]=value;return value.length();}
 bool remove(const char* key){return values.erase(key);}
 void clear(){values.clear();}
};
