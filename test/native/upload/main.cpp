#include "src/state.h"
#include "src/storage/library.h"
#include "src/storage/page_cache.h"
#include "src/web/chrome.h"
#include "src/web/upload.h"
#include "src/lang/locale_core.h"
#include <cassert>
#include <cstdio>
#include <zlib.h>
Catalog g_library;int invalidated=0;
void loadBooks(){}
void deletePageCacheForBook(const String&){++invalidated;}
size_t fsFreeBytesSafe(){return 4000000;}
String htmlEscape(const String&s){return s;}
String humanBytes(size_t n){return String(n);}
String storageCardHtml(const char*){return "";}
String successPage(const String&,const String&,const String&,const String& content){return content;}
bool inflateData(const uint8_t* in,size_t n,uint8_t* out,size_t size){uLongf count=size;return uncompress(out,&count,in,n)==Z_OK&&count==size;}
void chunk(int phase,const char* name,const char* content=""){
 auto& u=server.currentUpload;u.status=phase;u.filename=name;u.currentSize=strlen(content);u.totalSize=strlen(content);
 memcpy(u.buf,content,u.currentSize);server.uploadRoutes.at("/upload")();
}
void done(){server.request("/upload",HTTP_POST);}
std::string stored(const char* path){auto& b=FS.nodes.at(path)->bytes;return {b.begin(),b.end()};}
int main(){
 assert(Language::select("en",inflateData));registerUploadRoutes();
 FS.put("/books/existing.txt",{'O','L','D'});
 chunk(UPLOAD_FILE_START,"existing.txt");chunk(UPLOAD_FILE_WRITE,"existing.txt","NEW");chunk(UPLOAD_FILE_END,"existing.txt","NEW");done();
 assert(server.status==400&&stored("/books/existing.txt")=="OLD"&&!FS.exists("/books/existing.txt.tmp")&&invalidated==0);
 const std::string longName=std::string(85,'x')+".txt";
 chunk(UPLOAD_FILE_START,longName.c_str());done();assert(server.status==400&&FS.nodes.size()==1);
 chunk(UPLOAD_FILE_START,"new.txt");chunk(UPLOAD_FILE_WRITE,"new.txt","Müller reads.\n");chunk(UPLOAD_FILE_END,"new.txt","Müller reads.\n");done();
 assert(server.status==200&&stored("/books/new.txt")=="Müller reads."&&!FS.exists("/books/new.txt.tmp"));
 chunk(UPLOAD_FILE_START,"raced.txt");chunk(UPLOAD_FILE_WRITE,"raced.txt","NEW");FS.put("/books/raced.txt",{'O','L','D'});chunk(UPLOAD_FILE_END,"raced.txt","NEW");done();
 assert(server.status==400&&stored("/books/raced.txt")=="OLD"&&!FS.exists("/books/raced.txt.tmp"));
 chunk(UPLOAD_FILE_START,"short.txt");writeBudget=1;chunk(UPLOAD_FILE_WRITE,"short.txt","ABCDEFGHIJ");writeBudget=UINT32_MAX;chunk(UPLOAD_FILE_END,"short.txt","ABCDEFGHIJ");done();
 assert(server.status==400&&!FS.exists("/books/short.txt")&&!FS.exists("/books/short.txt.tmp"));
 chunk(UPLOAD_FILE_START,"cancel.txt");chunk(UPLOAD_FILE_WRITE,"cancel.txt","Some text");chunk(UPLOAD_FILE_ABORTED,"cancel.txt");done();
 assert(server.status==400&&!FS.exists("/books/cancel.txt")&&!FS.exists("/books/cancel.txt.tmp"));
 puts("PASS production multipart upload: valid UTF-8, existing-book and late-collision protection, long-name rejection, short-write and cancellation cleanup");
}
