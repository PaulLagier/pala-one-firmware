#include "src/config.h"
#if PALA_DIAGNOSTICS && defined(PALA_COMPANION_TEST)
#include "src/hal/companion_rpc.h"
#include "src/hal/companion_ble.h"
#include "src/hal/file_stream.h"
#include "src/hal/packed_fixture.h"
#include "src/storage/library.h"
#include "src/storage/fs_util.h"
#include "src/storage/statistics.h"
#include "src/ui/reader.h"
#include "src/ui/screens/reader_screen.h"
#include "src/ui/sleep.h"
#include "src/ui/lock.h"
#include "src/storage/book_metadata.h"
#include "src/storage/preferences_store.h"
#include <esp_sleep.h>
#include "src/ui/screen.h"
#include "src/ui/screens/library_screen.h"
#include <ArduinoJson.h>
#include <mbedtls/sha256.h>
#include <algorithm>
#include <esp_heap_caps.h>
#include "src/lang/locale_core.h"
#include "src/ui/font.h"
#include "src/state.h"

namespace {
struct RpcProbe {
  JsonDocument request, reply;
  uint32_t sequence=0, transfer=0;
  unsigned checks=0,firstFailure=0; bool passed=true;
  void check(bool ok){++checks;if(!ok&&!firstFailure)firstFailure=checks;passed=passed&&ok;}
  void prepare(const char* op){request.clear();request["id"]=++sequence;request["op"]=op;}
  bool send(){String bytes;serializeJson(request,bytes);const String bytesOut=companionRpcControl((const uint8_t*)bytes.c_str(),bytes.length());reply.clear();
    return !deserializeJson(reply,bytesOut)&&(reply["id"]|0U)==sequence;}
  bool ok(){return send()&&(reply["ok"]|false);}
  bool error(const char* reason){return send()&&!(reply["ok"]|true)&&String(reply["error"]|"")==reason;}
  bool begin(){prepare("upload.begin");request["name"]="PalaConnectionCheck.pbt";request["size"]=sizeof(kPackedFixture);
    if(!ok())return false;transfer=reply["uploadId"]|0U;return transfer!=0;}
  bool chunks(bool corrupt=false,size_t limit=sizeof(kPackedFixture)){
    for(uint32_t offset=0;offset<limit;){
      const size_t n=std::min<size_t>(480,limit-offset);uint8_t frame[492];
      const uint32_t fields[]={++sequence,transfer,offset};
      for(int k=0;k<3;++k)for(int j=0;j<4;++j)frame[k*4+j]=fields[k]>>(8*j);
      memcpy(frame+12,kPackedFixture+offset,n);
      if(corrupt&&offset+n==sizeof(kPackedFixture))frame[12+n-1]^=0xff;
      const String bytes=companionRpcChunk(frame,n+12);reply.clear();
      if(deserializeJson(reply,bytes)||!(reply["ok"]|false)||(reply["offset"]|0U)!=offset+n)return false;
      offset+=n;yield();
    }return true;
  }
  void finish(const String& hash){prepare("upload.finish");request["uploadId"]=transfer;request["sha256"]=hash;}
  bool cancel(){prepare("upload.cancel");return ok();}
};
String fixtureSha(bool corrupt){
  mbedtls_sha256_context context;mbedtls_sha256_init(&context);uint8_t hash[32];
  bool ok=mbedtls_sha256_starts(&context,0)==0&&mbedtls_sha256_update(&context,kPackedFixture,sizeof(kPackedFixture)-1)==0;
  const uint8_t last=kPackedFixture[sizeof(kPackedFixture)-1]^(corrupt?0xff:0);
  ok=ok&&mbedtls_sha256_update(&context,&last,1)==0&&mbedtls_sha256_finish(&context,hash)==0;mbedtls_sha256_free(&context);
  String result;if(ok)for(uint8_t byte:hash){char digits[3];snprintf(digits,sizeof(digits),"%02x",byte);result+=digits;}return result;
}
}
void companionUploadDeviceTest(){
  const char* path="/books/PalaConnectionCheck.pbt";
  if(g_currentScreen!=&g_libraryScreen||g_bookview.book.isOpen()||companionBleRunning()){
    Serial.println("[upload-test] busy");return;
  }
  if(FS.exists(path)||g_library.bookCount>=MAX_BOOKS-1||fsFreeBytesSafe()<32768){Serial.println("[upload-test] unavailable");return;}
  const auto statistics=Statistics::snapshot();const int initialCount=g_library.bookCount;
  const String hash=fixtureSha(false),badBookHash=fixtureSha(true);RpcProbe test;
  companionRpcReset();test.check(hash.length()==64&&badBookHash.length()==64);
  // Interrupted transfers are invisible; reset must remove their reserved files.
  bool started=test.begin();test.check(started);
  String interrupted;
  if(started){interrupted="/books/.pala-ble-"+String(test.transfer,HEX)+".tmp";test.check(test.chunks(false,1));test.prepare("books.list");test.check(test.error("transfer-busy"));}
  test.check(test.cancel()&&!FS.exists(path)&&!FS.exists(interrupted)&&g_library.bookCount==initialCount);
  // A premature finish cancels the incomplete transaction.
  started=test.begin();test.check(started);
  if(started){test.finish(hash);test.check(test.error("incomplete-upload"));}
  test.check(!FS.exists(path));
  // Correct size but a mismatching checksum must never publish a book.
  started=test.begin();test.check(started);
  if(started){test.check(test.chunks());test.finish(hash);
    // Ensure the expected digest is wrong even when its first digit is zero.
    test.request["sha256"]=String(hash[0]=='0'?'1':'0')+hash.substring(1);
    test.check(test.error("checksum-mismatch"));}
  test.check(!FS.exists(path));
  // Correct transfer checksum is insufficient when a compressed block is corrupt.
  started=test.begin();test.check(started);
  if(started){test.check(test.chunks(true));test.finish(badBookHash);test.check(test.error("invalid-book"));}
  test.check(!FS.exists(path));
  // Exercise the same begin/chunk/finish transaction used by the native app.
  bool published=false;started=test.begin();test.check(started);
  if(started){
    // Out-of-order packets must not alter the expected offset or close the transfer.
    uint8_t invalid[13]={};const uint32_t fields[]={++test.sequence,test.transfer,1};
    for(int k=0;k<3;++k)for(int j=0;j<4;++j)invalid[k*4+j]=fields[k]>>(8*j);
    const String rejected=companionRpcChunk(invalid,sizeof(invalid));JsonDocument error;
    test.check(!deserializeJson(error,rejected)&&String(error["error"]|"")=="invalid-offset");
    test.check(test.chunks());test.finish(hash);published=test.ok();
    test.check(published&&String(test.reply["path"]|"")==path&&(test.reply["size"]|0U)==sizeof(kPackedFixture));
  }
  File book=FS.open(path,"r");test.check(published&&book&&book.size()==sizeof(kPackedFixture)&&bookLogicalSize(book)==kPackedRawSize);book.close();
  bool listed=false;
  for(int cursor=0;cursor<g_library.bookCount;++cursor){
    test.prepare("books.list");test.request["cursor"]=cursor;
    const bool ok=test.ok();test.check(ok&&(test.reply["total"]|-1)==initialCount+1);
    if(ok&&String(test.reply["books"][0]["path"]|"")==path){listed=true;test.check((test.reply["books"][0]["size"]|0U)==sizeof(kPackedFixture));}
  }
  test.check(listed&&g_library.bookCount==initialCount+1);
  const size_t free=fsFreeBytesSafe();test.prepare("ping");test.check(test.ok()&&(test.reply["freeBytes"]|0U)==free);
  // Existing destination must survive a second begin untouched.
  if(published){test.prepare("upload.begin");test.request["name"]="PalaConnectionCheck.pbt";test.request["size"]=sizeof(kPackedFixture);test.check(test.error("already-exists"));}
  test.cancel();
  if(published){test.prepare("books.delete");test.request["path"]=path;test.check(test.ok());}
  // Remove only our unique, preflight-absent fixture, including on a failed check.
  if(FS.exists(path))test.check(FS.remove(path));
  companionRpcReset();loadBooks();test.check(!FS.exists(path)&&g_library.bookCount==initialCount);
  const auto after=Statistics::snapshot();test.check(statistics.pagesRead==after.pagesRead&&statistics.buttonPresses==after.buttonPresses&&statistics.totalSessions==after.totalSessions);
  Serial.printf("[upload-test] checks=%u first_failure=%u stored=%u logical=%lu catalog=%d cleaned=%d pass=%d\n",test.checks,test.firstFailure,unsigned(sizeof(kPackedFixture)),(unsigned long)kPackedRawSize,listed,!FS.exists(path),test.passed);
}

namespace {
constexpr const char* kResumePath="/books/PalaResumeCheck.pbt";
constexpr const char* kResumeState="/.pala_resume_probe.json";
void discardResumeFixture(){
  if(g_bookview.book.isOpen()&&g_bookview.book.path()==kResumePath)resetBookView();
  FS.remove(kResumePath);deleteBookMetadata(kResumePath);FS.remove(kResumeState);loadBooks();
}
bool loadResumeProbe(JsonDocument& state){
  File file=FS.open(kResumeState,"r");const bool ok=file&&!deserializeJson(state,file);
  file.close();return ok&&String(state["kind"]|"")=="pala-resume-diagnostic-v1"&&String(state["firmware"]|"")==FW_VERSION;
}
}
void companionResumeTestBegin(){
  if(g_currentScreen!=&g_libraryScreen||g_bookview.book.isOpen()||companionBleRunning()||Lock::isLocked()||prefs.isKey("wake_path")){
    Serial.println("[resume-arm] busy");return;
  }
  const String key=prefKeyForBook(kResumePath);
  if(FS.exists(kResumePath)||FS.exists(kResumeState)||FS.exists("/pc_"+key+".bin")
      ||prefs.isKey((key+"_p").c_str())||prefs.isKey((key+"_off").c_str())||prefs.isKey((key+"_bm").c_str())
      ||g_library.bookCount>=MAX_BOOKS||fsFreeBytesSafe()<32768){Serial.println("[resume-arm] unavailable");return;}
  File file=FS.open(kResumePath,"w");const bool written=file&&file.write(kPackedFixture,sizeof(kPackedFixture))==sizeof(kPackedFixture);file.close();
  loadBooks();const int index=bookIndexForPath(kResumePath);
  if(!written||index<0||!openBookByIndex(index)){discardResumeFixture();Serial.println("[resume-arm] storage-error");return;}
  g_bookview.cursor.pageIndex=findPageForOffset(4600);
  const uint32_t offset=g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);
  JsonDocument state;state["kind"]="pala-resume-diagnostic-v1";state["firmware"]=FW_VERSION;
  state["page"]=g_bookview.cursor.pageIndex;state["offset"]=offset;state["pagesRead"]=Statistics::snapshot().pagesRead;
  state["noScreensaver"]=Sleep::noScreensaver();state["lockOnSleep"]=Sleep::lockOnSleep();state["sleepSeconds"]=Sleep::idleTimeoutSecs();
  File marker=FS.open(kResumeState,"w");const bool marked=marker&&serializeJson(state,marker)==measureJson(state);marker.close();
  if(!marked){discardResumeFixture();Serial.println("[resume-arm] storage-error");return;}
  g_currentScreen=&g_readerScreen;g_readerScreen.nextScreen=nullptr;forceNextRenderFull();renderCurrentPage();
  Serial.printf("[resume-arm] page=%d offset=%lu logical=%lu stored=%u armed=1\n",g_bookview.cursor.pageIndex,(unsigned long)offset,(unsigned long)g_bookview.book.size(),unsigned(sizeof(kPackedFixture)));
}
void companionResumeTestCheck(bool cancel){
  JsonDocument state;
  if(!loadResumeProbe(state)){Serial.println("[resume-check] no-owned-probe pass=0");return;}
  if(g_bookview.book.isOpen()&&g_bookview.book.path()!=kResumePath){Serial.println("[resume-check] busy pass=0");return;}
  const int page=state["page"]|-1;const uint32_t offset=state["offset"]|UINT32_MAX;
  const bool position=g_currentScreen==&g_readerScreen&&g_bookview.book.isOpen()&&g_bookview.book.path()==kResumePath
    &&g_bookview.book.size()==kPackedRawSize&&g_bookview.cursor.pageIndex==page&&g_bookview.pages.contains(page)&&g_bookview.pages.offsetAt(page)==offset;
  PreferencesStore store(prefs);const String key=prefKeyForBook(kResumePath);
  const bool saved=loadSavedOffset(store,key)==offset&&loadSavedPage(store,key)==page&&!prefs.isKey("wake_path");
  bool content=false;
  if(position){FileReadStream stream(g_bookview.book.file());content=stream.seek(offset)&&stream.read()==uint8_t(kPackedPattern[offset%(sizeof(kPackedPattern)-1)]);}
  const bool settings=(state["noScreensaver"]|false)==Sleep::noScreensaver()&&(state["lockOnSleep"]|false)==Sleep::lockOnSleep()
    &&(state["sleepSeconds"]|-1)==Sleep::idleTimeoutSecs()&&Lock::isLocked()==Sleep::lockOnSleep();
  const int wake=int(esp_sleep_get_wakeup_cause());const bool stats=(state["pagesRead"]|UINT64_MAX)==Statistics::snapshot().pagesRead;
  discardResumeFixture();if(Lock::isLocked())Lock::disengage();
  const bool cleaned=!FS.exists(kResumePath)&&!FS.exists(kResumeState)&&bookIndexForPath(kResumePath)<0
    &&!prefs.isKey((key+"_p").c_str())&&!prefs.isKey((key+"_off").c_str())&&!FS.exists("/pc_"+key+".bin");
  g_currentScreen=&g_libraryScreen;g_libraryScreen.onEnter();
  Serial.printf("[resume-check] position=%d saved=%d content=%d settings=%d wake=%d statistics=%d cleaned=%d cancelled=%d pass=%d\n",
    position,saved,content,settings,wake,stats,cleaned,cancel,cleaned&&(cancel||(position&&saved&&content&&settings&&wake==ESP_SLEEP_WAKEUP_EXT0&&stats)));
}
// Synthetic, diagnostic-only acceptance for all11 catalogs under actual BLE heap pressure.
void companionLanguageDeviceTest() {
  if(g_bookview.book.isOpen()||companionBleConnected()){Serial.println("[language-test] busy");return;}
  const String original=Language::currentCode();const bool saved=prefs.isKey("cfg_lang");
  const bool wasRunning=companionBleRunning();if(!wasRunning)companionBleBegin();
  RpcProbe probe;unsigned minimum=ESP.getFreeHeap(),largestMinimum=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  const auto layout=Font::layoutForCache();const uint64_t pages=Statistics::snapshot().pagesRead;
  for(size_t i=0;i<Language::count();++i) {
    const char* code=Language::codeAt(i);const auto started=millis();
    probe.prepare("settings.set");probe.request["key"]="language";probe.request["value"]=code;probe.check(probe.ok());
    probe.check(!strcmp(Language::currentCode(),code));probe.check(prefs.getString("cfg_lang","")==code);
    auto settings=probe.reply["settings"].as<JsonObject>();probe.check(String(settings["language"]|"")==code);
    probe.check(settings["availableLanguages"].as<JsonArray>().size()==11);
    // Re-read NVS through the actual boot loader, not an artificial dictionary fixture.
    Language::loadSettings();probe.check(!strcmp(Language::currentCode(),code));
    probe.prepare("settings.get");probe.check(probe.ok());
    const auto after=Font::layoutForCache();probe.check(after.bodySize==layout.bodySize&&after.lineGap==layout.lineGap&&after.family==layout.family&&after.bionic==layout.bionic);
    probe.check(Statistics::snapshot().pagesRead==pages);
    probe.check(companionBleRunning());
    unsigned heap=ESP.getFreeHeap(),largest=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    minimum=std::min(minimum,heap);largestMinimum=std::min(largestMinimum,largest);
    Serial.printf("[language-case] code=%s ms=%lu heap=%u largest=%u pass=%d\n",code,(unsigned long)(millis()-started),heap,largest,probe.passed);
    yield();
  }
  probe.prepare("settings.set");probe.request["key"]="language";probe.request["value"]="xx";probe.check(probe.error("invalid-setting"));
  const bool restored=Language::setLanguage(original.c_str());probe.check(restored);if(!saved)prefs.remove("cfg_lang");
  if(!wasRunning)companionBleEnd();
  g_libraryScreen.draw();
  Serial.printf("[language-test] checks=%u first_failure=%u min_heap=%u min_largest=%u restored=%d pass=%d\n",probe.checks,probe.firstFailure,minimum,largestMinimum,restored,probe.passed);
}

#endif
