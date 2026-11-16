#pragma once
#include "Arduino.h"
#include <cstring>
#include <string>
struct InputTestPrefs {
  uint32_t value = 175;
  bool wakePath=false;
  std::string getString(const char*,const char*){return wakePath?"/books/test.txt":"";}
  bool begin(const char*,bool){return true;}
  bool isKey(const char* key) { return wakePath&&!strcmp(key,"wake_path"); }
  size_t getBytes(const char*,void*,size_t) { return 0; }
  size_t putBytes(const char*,const void* data,size_t n) { value=static_cast<const uint32_t*>(data)[0]; return n; }
  uint32_t getUInt(const char*,uint32_t) { return value; }
  void putUInt(const char*,uint32_t v) { value = v; }
};
extern InputTestPrefs prefs;
