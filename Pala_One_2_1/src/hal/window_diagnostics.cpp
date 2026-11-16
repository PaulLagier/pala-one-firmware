#include "src/config.h"
#if PALA_DIAGNOSTICS && defined(PALA_COMPANION_TEST)
#include "src/hal/companion_ble.h"
#include "src/hal/cpu_burst.h"
#include "src/hal/input.h"
#include "src/storage/fs_util.h"
#include "src/storage/library.h"
#include "src/storage/statistics.h"
#include "src/storage/book_metadata.h"
#include "src/storage/preferences_store.h"
#include "src/ui/reader.h"
#include "src/ui/reader_index_feedback.h"
#include "src/ui/font.h"
#include "src/ui/text.h"
#include "src/ui/screen.h"
#include "src/ui/lock.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screens/bookmarks/bookmark_list_screen.h"
#include "src/ui/screens/bookmarks/book_select_screen.h"
#include "src/ui/screens/bookmarks/session.h"
#include "src/storage/page_cache.h"
#include <esp_heap_caps.h>
#include <memory>
#include <new>

// Explicit private cleanup after interrupted self-tests. Only reserved paths,
// bounded sizes and the exact generated text pattern are accepted, even for
// partial writes. Never format storage or touch any other book.
void readerTestCleanup(){
  if(g_currentScreen!=&g_libraryScreen||g_bookview.book.isOpen()||companionBleRunning()||Lock::isLocked()){
    Serial.println("[test-cleanup] busy");return;
  }
  auto clean=[](const char* path,uint32_t expected)->bool{
    if(!FS.exists(path))return true;
    File file=FS.open(path,"r");if(!file||file.isDirectory()||file.size()>expected){file.close();return false;}
    uint8_t bytes[128];size_t position=0;bool verified=true;
    while(file.available()&&verified){const size_t count=file.read(bytes,sizeof(bytes));if(!count){verified=false;break;}
      for(size_t i=0;i<count;++i)if(bytes[i]!=uint8_t("a\n\n"[(position+i)%3])){verified=false;break;}
      position+=count;delay(1);
    }
    verified=verified&&position==file.size();file.close();if(!verified)return false;
    const String key=prefKeyForBook(path);deleteBookMetadata(path);
    return FS.remove(path)&&!FS.exists(path)&&!FS.exists("/pc_"+key+".bin")
      &&!prefs.isKey((key+"_p").c_str())&&!prefs.isKey((key+"_off").c_str())&&!prefs.isKey((key+"_bm").c_str());
  };
  const bool index=clean("/books/PalaIndexCancelCheck.txt",300000);
  const bool window=clean("/books/PalaWindowCheck.txt",720000);
  loadBooks();g_libraryScreen.onEnter();
  Serial.printf("[test-cleanup] index=%d window=%d pass=%d\n",index,window,index&&window);
}

void readerWindowDeviceTest(){
  const char* path="/books/PalaWindowCheck.txt";const uint32_t size=720000;
  const String key=prefKeyForBook(path);
  if(g_currentScreen!=&g_libraryScreen||g_bookview.book.isOpen()||companionBleRunning()||Lock::isLocked()){
    Serial.println("[window-test] busy");return;
  }
  if(FS.exists(path)||FS.exists("/pc_"+key+".bin")||prefs.isKey((key+"_p").c_str())||prefs.isKey((key+"_off").c_str())
     ||prefs.isKey((key+"_bm").c_str())||g_library.bookCount>=MAX_BOOKS||fsFreeBytesSafe()<size+2*MAX_PAGES*4+16384){
    Serial.println("[window-test] unavailable");return;
  }
  const auto before=Statistics::snapshot();const uint32_t started=millis();CpuBurst cpu;bool ok=true;
  constexpr size_t blockBytes=768;
  std::unique_ptr<uint8_t[]> block(new(std::nothrow) uint8_t[blockBytes]);
  if(!block){Serial.println("[window-test] unavailable-memory");return;}
  for(size_t i=0;i<blockBytes;++i)block[i]="a\n\n"[i%3];
  File file=FS.open(path,"w");ok=bool(file);
  for(uint32_t written=0;ok&&written<size;){const size_t n=min(size_t(blockBytes),size_t(size-written));ok=file.write(block.get(),n)==n;written+=n;delay(1);}
  file.close();block.reset();loadBooks();Font::setDiagnosticLayout(14,Font::Family::Helvetica,0,false);
  const int index=bookIndexForPath(path);ok=ok&&index>=0&&openBookByIndex(index);
  int last=-1;bool reverse=false,bookmark=false,resumed=false;uint32_t resumeMs=0;
  // Independent sequential reference, with constant RAM and no reader table.
  // Font coverage can change line height; never hard-code bytes per page.
  const uint32_t targets[]={90000,180000,270000,360000,450000,540000,630000};
  int expectedPages[7]={};uint32_t expectedOffsets[7]={};
  uint32_t lastOffset=0,reverseOffset=kOffsetUnset;int referencePages=0;
  if(ok){
    File referenceFile=FS.open(path,"r");ok=bool(referenceFile);
    {
      std::unique_ptr<FileReadStream> reference(new(std::nothrow) FileReadStream(referenceFile));
      ok=ok&&bool(reference);uint32_t offset=0;
      while(ok&&offset<size){
        const uint32_t next=nextPageOffset(*reference,offset);if(next<=offset){ok=false;break;}
        if(referencePages==65536)reverseOffset=offset;
        for(int i=0;i<7;++i)if(targets[i]>=offset&&targets[i]<next){expectedPages[i]=referencePages;expectedOffsets[i]=offset;}
        lastOffset=offset;++referencePages;offset=next;if((referencePages&127)==0)delay(1);
      }
    }
    referenceFile.close();ok=ok&&referencePages>65536&&reverseOffset!=kOffsetUnset;
  }
  if(ok){
    // No real page-turn hooks: synthetic tests must not inflate statistics.
    for(int i=0;i<7;++i){
      const int page=findPageForOffset(targets[i]);ok=ok&&page==expectedPages[i]&&g_bookview.pages.offsetAt(page)==expectedOffsets[i];
      Serial.printf("[window-stage] page=%d first=%d count=%d elapsed_ms=%lu heap=%lu\n",page,g_bookview.pages.firstPage,g_bookview.pages.count,(unsigned long)(millis()-started),(unsigned long)ESP.getFreeHeap());delay(1);
    }
    last=findPageForOffset(size-1);ok=ok&&last==referencePages-1&&g_bookview.pages.endPage()==referencePages&&g_bookview.pages.eofReached;
    g_bookview.cursor.pageIndex=last;renderCurrentPage();ok=ok&&!advancePage();
    const int previous=findPageForOffset(reverseOffset);reverse=previous==65536&&g_bookview.pages.offsetAt(previous)==reverseOffset;
    g_bookview.cursor.pageIndex=previous;bookmark=addBookmarkForCurrentBook()!=nullptr;
    PreferencesStore store(prefs);const auto mark=loadBookmarks(store,key);bookmark=bookmark&&mark.count==1&&mark.pages[0]==65536&&mark.offsets[0]==reverseOffset;
    g_bookview.cursor.pageIndex=findPageForOffset(size-1);persistReaderState();resetBookView();
    const uint32_t resumeStart=millis();resumed=openBookByIndex(index);resumeMs=millis()-resumeStart;
    resumed=resumed&&g_bookview.cursor.pageIndex==last&&g_bookview.pages.firstPage>0&&g_bookview.pages.offsetAt(last)==lastOffset;
  }
  resetBookView();Font::loadSettings();deleteBookMetadata(path);const bool removed=FS.remove(path);loadBooks();
  const bool cleaned=removed&&!FS.exists(path)&&!FS.exists("/pc_"+key+".bin")&&!prefs.isKey((key+"_p").c_str())&&!prefs.isKey((key+"_off").c_str())&&!prefs.isKey((key+"_bm").c_str());
  const auto after=Statistics::snapshot();const bool stats=before.pagesRead==after.pagesRead&&before.buttonPresses==after.buttonPresses&&before.totalSessions==after.totalSessions&&before.currentStreak==after.currentStreak;
  resetInputFrontend();cpu.restore();g_libraryScreen.onEnter();
  Serial.printf("[window-test] pages=%d reverse=%d bookmark=%d resume=%d resume_ms=%lu cleaned=%d statistics=%d heap=%lu largest=%lu elapsed_ms=%lu pass=%d\n",
    last+1,reverse,bookmark,resumed,(unsigned long)resumeMs,cleaned,stats,(unsigned long)ESP.getFreeHeap(),(unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),(unsigned long)(millis()-started),ok&&reverse&&bookmark&&resumed&&cleaned&&stats);
}
void readerIndexCancelDeviceTest(){
  const char* path="/books/PalaIndexCancelCheck.txt";const uint32_t size=300000,target=250000;
  const String key=prefKeyForBook(path);
  if(g_currentScreen!=&g_libraryScreen||g_bookview.book.isOpen()||companionBleRunning()||Lock::isLocked()){
    Serial.println("[index-cancel-test] busy");return;
  }
  if(FS.exists(path)||FS.exists("/pc_"+key+".bin")||prefs.isKey((key+"_p").c_str())||prefs.isKey((key+"_off").c_str())
     ||prefs.isKey((key+"_bm").c_str())||g_library.bookCount>=MAX_BOOKS||fsFreeBytesSafe()<size+100000){
    Serial.println("[index-cancel-test] unavailable");return;
  }
  const auto before=Statistics::snapshot();bool ok=true;
  constexpr size_t blockBytes=768;
  std::unique_ptr<uint8_t[]> block(new(std::nothrow) uint8_t[blockBytes]);
  if(!block){Serial.println("[index-cancel-test] unavailable-memory");return;}
  for(size_t i=0;i<blockBytes;++i)block[i]="a\n\n"[i%3];
  File file=FS.open(path,"w");ok=bool(file);
  for(uint32_t written=0;ok&&written<size;){const size_t n=min(size_t(blockBytes),size_t(size-written));ok=file.write(block.get(),n)==n;written+=n;delay(1);}
  file.close();block.reset();loadBooks();Font::setDiagnosticLayout(14,Font::Family::Helvetica,0,false);
  PreferencesStore store(prefs);saveSavedOffset(store,key,target);saveSavedPage(store,key,1234);
  const int index=bookIndexForPath(path);const uint32_t clock=getCpuFrequencyMhz(),started=millis();
  bool cancelled=false,reopened=false,preserved=false,crossCancelled=false,crossResolved=false;
  uint32_t restoredOffset=kOffsetUnset,followingOffset=0;
  if(ok&&index>=0){
    readerIndexTestCancel(true);cancelled=!openBookByIndex(index)&&readerIndexWasCancelled()&&!g_bookview.book.isOpen();
    readerIndexTestCancel(false);preserved=loadSavedOffset(store,key)==target&&loadSavedPage(store,key)==1234&&getCpuFrequencyMhz()==clock;
    persistReaderState();preserved=preserved&&loadSavedOffset(store,key)==target&&loadSavedPage(store,key)==1234;
    resetBookView();reopened=openBookByIndex(index)&&!readerIndexWasCancelled();
    if(reopened&&g_bookview.pages.contains(g_bookview.cursor.pageIndex)){
      restoredOffset=g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);
      std::unique_ptr<FileReadStream> following(new(std::nothrow) FileReadStream(g_bookview.book.file()));
      if(following)followingOffset=nextPageOffset(*following,restoredOffset);
      reopened=restoredOffset<=target&&followingOffset>target;
    }else reopened=false;
    if(reopened)renderCurrentPage();
    resetBookView();deletePageCacheForBook(path);
    const uint32_t bookmarkPage=27777,unset=kOffsetUnset;
    saveBookmarksForKey(key,&bookmarkPage,&unset,1);
    g_bookmarkSession.bookIndex=index;g_bookmarkSession.selectedIndex=0;
    readerIndexTestCancel(true);g_bmListScreen.nextScreen=nullptr;g_bmListScreen.onEnter();
    crossCancelled=g_bmListScreen.nextScreen==&g_bmBookSelectScreen;
    readerIndexTestCancel(false);g_bmListScreen.nextScreen=nullptr;g_bmListScreen.onEnter();
    crossResolved=!g_bmListScreen.nextScreen;
    const auto bookmarks=loadBookmarks(store,key);
    preserved=preserved&&bookmarks.count==1&&bookmarks.pages[0]==bookmarkPage&&bookmarks.offsets[0]==kOffsetUnset
      &&loadSavedOffset(store,key)==target&&loadSavedPage(store,key)==1234&&getCpuFrequencyMhz()==clock;
  }
  readerIndexTestCancel(false);g_bmListScreen.nextScreen=nullptr;resetBookView();Font::loadSettings();deleteBookMetadata(path);
  const bool cleaned=FS.remove(path)&&!FS.exists("/pc_"+key+".bin")&&!prefs.isKey((key+"_off").c_str())&&!prefs.isKey((key+"_p").c_str())&&!prefs.isKey((key+"_bm").c_str());loadBooks();
  const auto after=Statistics::snapshot();const bool stats=before.pagesRead==after.pagesRead&&before.buttonPresses==after.buttonPresses&&before.totalSessions==after.totalSessions;
  resetInputFrontend();g_libraryScreen.nextScreen=nullptr;g_libraryScreen.onEnter();
  Serial.printf("[index-cancel-test] cancelled=%d cross_cancelled=%d cross_resolved=%d preserved=%d reopened=%d cleaned=%d statistics=%d restored_offset=%lu following_offset=%lu target=%lu elapsed_ms=%lu pass=%d\n",cancelled,crossCancelled,crossResolved,preserved,reopened,cleaned,stats,(unsigned long)restoredOffset,(unsigned long)followingOffset,(unsigned long)target,(unsigned long)(millis()-started),ok&&cancelled&&crossCancelled&&crossResolved&&preserved&&reopened&&cleaned&&stats);
}
#endif
