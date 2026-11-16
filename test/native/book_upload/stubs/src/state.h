#pragma once
#include "src/config.h"
#include "src/pure/arduino_compat.h"
#include <map>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

struct File {
  std::shared_ptr<std::vector<uint8_t>> bytes;
  std::string name;
  size_t cursor = 0;
  bool* shortWrite = nullptr;
  explicit operator bool() const { return bool(bytes); }
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
  File open(const String& path,const char* mode) {
    std::string key=path.c_str();
    if(mode[0]=='w') files[key]=std::make_shared<std::vector<uint8_t>>();
    auto found=files.find(key);
    return found==files.end()?File{}:File{found->second,key,0,&shortWrite};
  }
  bool exists(const String& path) { return files.count(path.c_str())!=0; }
  bool remove(const String& path) { return files.erase(path.c_str())!=0; }
  bool rename(const String& from,const String& to) {
    if(failRename || !exists(from)) return false;
    files[to.c_str()]=files.at(from.c_str()); files.erase(from.c_str()); return true;
  }
};
extern MemoryFS FS;
enum UploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
constexpr int HTTP_POST=1;
struct HTTPUpload {
  UploadStatus status=UPLOAD_FILE_START;
  String filename;
  uint8_t* buf=nullptr;
  size_t currentSize=0,totalSize=0;
};
struct TestServer {
  HTTPUpload incoming;
  int status=0;
  std::map<std::string,std::pair<void(*)(),void(*)()>> routes;
  HTTPUpload& upload() { return incoming; }
  void send(int code,const char*,const String&) { status=code; }
  void on(const char* path,int,void(*done)(),void(*stream)()) { routes[path]={done,stream}; }
};
extern TestServer server;
