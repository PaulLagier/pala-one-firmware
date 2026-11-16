#pragma once
#include <vector>
#include <string>
struct ResetFile {
  bool present, directory;
  explicit operator bool() const { return present; }
  bool isDirectory() const { return directory; }
  void close() {}
};
struct ResetFs {
  bool formatOk=true,mountOk=true,mkdirOk=true,validDirectory=true,existsNow=false;
  std::vector<std::string> calls;
  void end(){calls.push_back("end");}
  bool format(){calls.push_back("format");return formatOk;}
  bool begin(bool erase){calls.push_back(erase?"unsafe-mount":"mount");return mountOk;}
  bool exists(const char*){return existsNow;}
  bool mkdir(const char*){calls.push_back("mkdir");return mkdirOk;}
  ResetFile open(const char*,const char*){return {true,validDirectory};}
};
struct ResetPrefs {bool clearOk=true;unsigned clears=0;bool clear(){++clears;return clearOk;}};
inline ResetFs FS;
inline ResetPrefs prefs;
