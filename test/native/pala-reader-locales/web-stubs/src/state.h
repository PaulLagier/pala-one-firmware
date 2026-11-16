#pragma once
#include "src/config.h"
#include "Preferences.h"
#include "memory_fs.h"
#include <map>
#include <functional>
#include <string>
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef FPSTR
#define FPSTR(value) (value)
#endif
constexpr int HTTP_GET=0,HTTP_POST=1;
enum {UPLOAD_FILE_START,UPLOAD_FILE_WRITE,UPLOAD_FILE_END,UPLOAD_FILE_ABORTED};
struct HTTPUpload {int status=0;String filename;size_t currentSize=0,totalSize=0;uint8_t buf[4096]={};};
struct NativeWebServer {
  HTTPUpload currentUpload;
  std::map<std::string,std::function<void()>> uploadRoutes;
  HTTPUpload& upload(){return currentUpload;}
  void on(const char* path,int method,std::function<void()> done,std::function<void()> stream){on(path,method,done);uploadRoutes[path]=stream;}

  std::map<std::pair<std::string,int>,std::function<void()>> routes;
  std::map<std::string,String> arguments,headers;
  int status=0;String body,mime;
  void on(const char* path,int method,std::function<void()> handler){routes[{path,method}]=handler;}
  bool hasArg(const char* key){return arguments.count(key);}
  String arg(const char* key){auto it=arguments.find(key);return it==arguments.end()?String(""):it->second;}
  void sendHeader(const char* key,const char* value){headers[key]=value;}
  void send(int code,const char* type,const String& content){status=code;mime=type;body=content;}
  void send_P(int code,const char* type,const char* content){send(code,type,content);}
  void send_P(int code,const char* type,const char* content,size_t bytes){send(code,type,String(std::string(content,bytes)));}
  void request(const char* path,int method,std::map<std::string,String> args={}){arguments=std::move(args);headers.clear();body="";status=0;routes.at({path,method})();}
};
inline NativeWebServer server;
inline MemoryFS FS;
inline Preferences prefs;
inline void* g_currentScreen=nullptr;
