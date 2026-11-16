#pragma once
#include <map>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>
#include <Arduino.h>
#include "src/config.h"
#include "src/pure/arduino_compat.h"

struct File {
  bool directory=false;
  std::vector<std::string> entries;
  size_t entryIndex=0;
  bool isDirectory() const { return directory; }
  File openNextFile();
  void flush() {}
  std::shared_ptr<std::vector<uint8_t>> bytes;
  std::string name;
  size_t cursor = 0;
  bool* shortWrite = nullptr;
  explicit operator bool() const { return bool(bytes) || directory; }
  const char* path() const { return name.c_str(); }
  size_t size() const { return bytes ? bytes->size() : 0; }
  size_t position() const { return cursor; }
  size_t available() const { return size() > cursor ? size()-cursor : 0; }
  bool seek(uint32_t pos) { if (!bytes || pos>size()) return false; cursor=pos; return true; }
  size_t read(uint8_t* out, size_t n) {
    n=(std::min)(n,available());
    if(n) std::copy_n(bytes->data()+cursor,n,out);
    cursor+=n; return n;
  }
  int read() { uint8_t byte; return read(&byte,1)?int(byte):-1; }
  size_t write(const uint8_t* input, size_t n) {
    if(!bytes) return 0;
    if(shortWrite && *shortWrite && n) --n;
    if(cursor+n>size()) bytes->resize(cursor+n);
    std::copy_n(input,n,bytes->data()+cursor); cursor+=n; return n;
  }
  size_t print(const String& value) { return write(reinterpret_cast<const uint8_t*>(value.c_str()),value.length()); }
  void close() { bytes.reset(); }
};
struct MemoryFS {
  std::map<std::string,std::shared_ptr<std::vector<uint8_t>>> files;
  bool shortWrite=false, failRename=false;
  size_t freeBytes=4*1024*1024;
  File open(const String& path,const char* mode="r") {
    std::string key=path.c_str();
    if(key=="/books" && mode[0]!='w') {
      File dir; dir.directory=true; dir.name=key;
      for(const auto& entry:files) if(entry.first.rfind("/books/",0)==0) dir.entries.push_back(entry.first);
      return dir;
    }
    if(mode[0]=='w') files[key]=std::make_shared<std::vector<uint8_t>>();
    auto found=files.find(key);
    if(found==files.end())return File{};
    File file;file.bytes=found->second;file.name=key;file.shortWrite=&shortWrite;return file;
  }
  bool exists(const String& path) { return files.count(path.c_str())!=0; }
  bool remove(const String& path) { return files.erase(path.c_str())!=0; }
  bool rename(const String& from,const String& to) {
    if(failRename || !exists(from)) return false;
    files[to.c_str()]=files.at(from.c_str()); files.erase(from.c_str()); return true;
  }
};
extern MemoryFS FS;
inline File File::openNextFile() {
  return entryIndex<entries.size()?FS.open(entries[entryIndex++].c_str()):File{};
}
