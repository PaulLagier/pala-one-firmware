#include "src/hal/companion_rpc.h"
#include "src/lang/locale_core.h"
#include "src/hal/file_stream.h"
#ifdef PALA_COMPANION_TEST
#include <ArduinoJson.h>
#include "src/hal/companion_bookmarks.h"
#include "src/hal/companion_ble.h"
#include <esp_random.h>
#include <mbedtls/sha256.h>
#include "src/state.h"
#include "src/pure/paths.h"
#include "src/storage/library.h"
#include "src/storage/book_metadata.h"
#include "src/storage/fs_util.h"
#include "src/ui/font.h"
#include "src/hal/input.h"
#include "src/hal/display.h"
#include "src/ui/statusbar.h"
#include "src/ui/sleep.h"
#include "src/ui/header_title.h"
#include "src/ui/reader_actions.h"

namespace {
File upload;
String temporary, destination;
uint32_t uploadId = 0, expected = 0, received = 0;
mbedtls_sha256_context digest;
bool hashing = false;
uint32_t read32(const uint8_t* b) { return uint32_t(b[0]) | uint32_t(b[1])<<8 | uint32_t(b[2])<<16 | uint32_t(b[3])<<24; }
String response(JsonDocument& doc) {
  String out; serializeJson(doc, out);
  if (out.length() > 512) { const uint32_t id=doc["id"]|0U;doc.clear();doc["id"]=id;doc["ok"]=false;doc["error"]="response-too-large";out="";serializeJson(doc,out); }
  return out;
}
String fail(uint32_t id, const char* reason) { JsonDocument d; d["id"]=id; d["ok"]=false; d["error"]=reason; return response(d); }
void addSettings(JsonObject o) {
  o["language"]=Language::currentCode();
  auto languages=o["availableLanguages"].to<JsonArray>();
  for(size_t i=0;i<Language::count();++i)languages.add(Language::codeAt(i));
  o["title"]=HeaderTitle::current(); o["fontSize"]=Font::currentBodySize();
  o["fontFamily"]=Font::currentFamily()==Font::Family::OpenDyslexic?"dys":"helv";
  o["lineGap"]=Font::currentLineGap(); o["bionic"]=Font::bionicEnabled();
  o["sleepSeconds"]=Sleep::idleTimeoutSecs(); o["noScreensaver"]=Sleep::noScreensaver(); o["lockOnSleep"]=Sleep::lockOnSleep();
  o["longAction"]=int(Gestures::actionLong()); o["extraLongAction"]=int(Gestures::actionExtraLong()); o["clickHoldAction"]=int(Gestures::actionClickHold());
}
bool setSetting(const String& key, JsonVariant v) {
  if(key=="language" && v.is<const char*>()) return Language::setLanguage(v.as<const char*>());
  if (key=="title" && v.is<const char*>()) { String t=v.as<String>(); t.trim(); if(t.length()>31) return false; HeaderTitle::set(t.c_str()); return true; }
  if (key=="fontFamily" && v.is<const char*>()) { String f=v.as<String>(); if(f!="helv"&&f!="dys")return false; Font::setFamily(f=="dys"?Font::Family::OpenDyslexic:Font::Family::Helvetica); return true; }
  if (key=="halfParagraphGaps" && v.is<bool>()) { Font::setHalfParagraphGaps(v.as<bool>()); return true; }
  if (key=="flipped" && v.is<bool>()) { setDisplayFlipped(v.as<bool>()); return true; }
  if (key=="bionic" && v.is<bool>()) { Font::setBionic(v.as<bool>()); return true; }
  if (key=="noScreensaver" && v.is<bool>()) { Sleep::setNoScreensaver(v.as<bool>()); return true; }
  if (key=="lockOnSleep" && v.is<bool>()) { Sleep::setLockOnSleep(v.as<bool>()); return true; }
  if (!v.is<int>()) return false;
  const int n=v.as<int>();
  if (key=="clickSequenceMs"||key=="holdMs"||key=="veryLongMs"||key=="debounceMs"||key=="clickGapMs") {
    if(n<0)return false;
    InputTiming t=inputTiming();
    if(key=="clickGapMs")t.gap=n;else if(key=="clickSequenceMs")t.sequence=n;
    else if(key=="holdMs")t.hold=n;else if(key=="veryLongMs")t.veryLong=n;else t.debounce=n;
    return setInputTiming(t);
  }
  if (key=="statusbar" && n>=0 && n<=2) { Statusbar::setMode(Statusbar::Mode(n)); return true; }
  if (key=="fontSize" && (n==8||n==10||n==12||n==14)) { Font::setBodySize(n); return true; }
  if (key=="lineGap" && n>=0 && n<=4) { Font::setLineGap(n); return true; }
  if (key=="sleepSeconds" && (n>=10&&n<=3600)) { Sleep::setIdleTimeout(n); return true; }
  if (n>=0 && n<=3) {
    if(key=="longAction"){Gestures::setActionLong(ButtonAction(n));return true;}
    if(key=="extraLongAction"){Gestures::setActionExtraLong(ButtonAction(n));return true;}
    if(key=="clickHoldAction"){Gestures::setActionClickHold(ButtonAction(n));return true;}
  }
  return false;
}
}
void companionRpcReset() {
  companionBookmarksReset();
  if (upload) upload.close();
  // Reserved transfer files never enter the catalogue. Recover after power loss.
  static bool recovered = false;
  if (!recovered) {
    recovered = true;
    File dir = FS.open("/books");
    if (dir && dir.isDirectory()) {
      File entry = dir.openNextFile();
      while (entry) {
        String path = entry.path();
        const bool stale = !entry.isDirectory() && path.startsWith("/books/.pala-ble-") && path.endsWith(".tmp");
        entry.close();
        if (stale) FS.remove(path);
        entry = dir.openNextFile();
      }
      dir.close();
    }
  }
  if (temporary.length() && FS.exists(temporary)) FS.remove(temporary);
  if (hashing) { mbedtls_sha256_free(&digest); hashing=false; }
  temporary=""; destination=""; uploadId=0; expected=0; received=0;
}
String companionRpcControl(const uint8_t* bytes, size_t length) {
  JsonDocument in;
  if(deserializeJson(in,bytes,length))return fail(0,"invalid-json");
  const uint32_t id=in["id"]|0U; const String op=in["op"]|"";
  if(!id)return fail(0,"invalid-request");
  JsonDocument out; out["id"]=id; out["ok"]=true;
  if(op=="ping") { const size_t total=fsTotalBytesSafe(),used=fsUsedBytesSafe();out["totalBytes"]=total;out["usedBytes"]=used;out["freeBytes"]=used<=total?total-used:0; out["uploading"]=bool(upload); out["bookFormats"].to<JsonArray>().add("pbt1"); }
  else if(op=="books.list") {
    if(upload)return fail(id,"transfer-busy");
    int cursor=in["cursor"]|0;
    if(cursor<0 || cursor>g_library.bookCount)return fail(id,"invalid-cursor");
    auto books=out["books"].to<JsonArray>();
    // One bounded entry per page fits even the longest UTF-8 path.
    if(cursor<g_library.bookCount){const auto& b=g_library.books[cursor];auto item=books.add<JsonObject>();item["name"]=b.name;item["path"]=b.path;item["size"]=b.size;cursor++;}
    if(cursor<g_library.bookCount)out["next"]=cursor;else out["next"]=nullptr;
    out["total"]=g_library.bookCount;
  }
  else if(op=="books.delete") {
    if(upload)return fail(id,"transfer-busy");
    String path=in["path"]|"";
    int index=bookIndexForPath(path); if(index<0)return fail(id,"book-not-found");
    if(!FS.remove(path))return fail(id,"storage-error"); deleteBookMetadata(path); loadBooks();companionBleRefreshIdentity();
  }
  else if(op.startsWith("bookmarks.")) {
    if(upload)return fail(id,"transfer-busy");
    const char* error=companionBookmarksCommand(op,in,out);if(error)return fail(id,error);
  }
  else if(op=="settings.advanced.get") {
    auto o=out["settings"].to<JsonObject>();const auto t=inputTiming();
    o["halfParagraphGaps"]=Font::halfParagraphGaps();o["flipped"]=displayFlipped();
    o["statusbar"]=int(Statusbar::mode());o["clickGapMs"]=t.gap;o["clickSequenceMs"]=t.sequence;
    o["holdMs"]=t.hold;o["veryLongMs"]=t.veryLong;o["debounceMs"]=t.debounce;
  }
  else if(op=="settings.get") { addSettings(out["settings"].to<JsonObject>()); }
  else if(op=="settings.set") {
    if(!setSetting(in["key"]|"",in["value"]))return fail(id,"invalid-setting");
    companionBookmarksReset();companionBleRefreshIdentity();addSettings(out["settings"].to<JsonObject>());
  }
  else if(op=="upload.begin") {
    if(upload)return fail(id,"transfer-busy");
    if(g_library.bookCount>=MAX_BOOKS)return fail(id,"library-full");
    String name=in["name"]|""; String lower=name; lower.toLowerCase();
    uint32_t size=in["size"]|0U;
    if(!size || size>2*1024*1024 || fsFreeBytesSafe()<size+8192)return fail(id,"storage-full");
    if((!lower.endsWith(".txt") && !lower.endsWith(".pbt")) || name.length()>72 || name.indexOf('/')>=0 || name.indexOf('\\')>=0)return fail(id,"invalid-filename");
    name=name.substring(0,name.length()-4)+(lower.endsWith(".pbt")?".pbt":".txt");
    String clean=sanitizeUploadedFilename(name.substring(0,name.length()-4)+".txt");
    if(lower.endsWith(".pbt"))clean=clean.substring(0,clean.length()-4)+".pbt";
    if(!validUploadedBookName(clean))return fail(id,"invalid-filename");
    destination="/books/"+clean;
    if(FS.exists(destination)) { destination="";return fail(id,"already-exists"); }
    uploadId=esp_random(); if(!uploadId)uploadId=1;
    temporary="/books/.pala-ble-"+String(uploadId,HEX)+".tmp";
    upload=FS.open(temporary,"w");
    if(!upload){companionRpcReset();return fail(id,"storage-error");}
    expected=size;received=0;mbedtls_sha256_init(&digest);hashing=true;
    if(mbedtls_sha256_starts(&digest,0)){companionRpcReset();return fail(id,"hash-error");}
    out["uploadId"]=uploadId;out["offset"]=received;
  }
  else if(op=="upload.cancel") { companionRpcReset(); }
  else if(op=="upload.finish") {
    if(!upload || (in["uploadId"]|0U)!=uploadId)return fail(id,"invalid-upload");
    if(received!=expected){companionRpcReset();return fail(id,"incomplete-upload");}
    uint8_t hash[32]; if(mbedtls_sha256_finish(&digest,hash)){companionRpcReset();return fail(id,"hash-error");}
    String actual; for(uint8_t b:hash){char h[3];snprintf(h,sizeof(h),"%02x",b);actual+=h;}
    String wanted=in["sha256"]|"";wanted.toLowerCase();
    if(wanted.length()!=64 || actual!=wanted){companionRpcReset();return fail(id,"checksum-mismatch");}
    upload.flush();upload.close();
    if(destination.endsWith(".pbt")) {
      File candidate=FS.open(temporary,"r");
      // The transfer's temporary suffix is not a book type; validate explicitly.
      PalaPacked::Stream<File> book(candidate,inflateBookBlock);const size_t size=book.size();size_t actualBytes=0;
      while(book.read()>=0){++actualBytes;if((actualBytes&4095)==0)yield();}
      candidate.close();
      if(!size || actualBytes!=size){companionRpcReset();return fail(id,"invalid-book");}
    }
    if(FS.exists(destination)){companionRpcReset();return fail(id,"already-exists");}
    if(!FS.rename(temporary,destination)){companionRpcReset();return fail(id,"storage-error");}
    out["path"]=destination;out["size"]=received;out["sha256"]=actual;
    temporary="";companionRpcReset();loadBooks();companionBleRefreshIdentity();
  }
  else return fail(id,"unknown-command");
  return response(out);
}
String companionRpcChunk(const uint8_t* b,size_t length) {
  if(length<13)return fail(0,"invalid-chunk");
  uint32_t id=read32(b),session=read32(b+4),offset=read32(b+8);size_t count=length-12;
  if(!upload || session!=uploadId)return fail(id,"invalid-upload");
  if(offset!=received || count>expected-received)return fail(id,"invalid-offset");
  if(upload.write(b+12,count)!=count || mbedtls_sha256_update(&digest,b+12,count)){companionRpcReset();return fail(id,"storage-error");}
  received+=count;
  JsonDocument out;out["id"]=id;out["ok"]=true;out["offset"]=received;return response(out);
}
#else
void companionRpcReset() {}
String companionRpcControl(const uint8_t*,size_t){return "";}
String companionRpcChunk(const uint8_t*,size_t){return "";}
#endif
