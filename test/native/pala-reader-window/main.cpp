#include "src/hal/display.h"
#include "src/hal/file_stream.h"
#include "src/ui/reader.h"
#include "src/ui/reader_index_feedback.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/font.h"
#include "src/ui/font_render.h"
#include "src/ui/language_fonts.h"
#include "src/lang/locale_core.h"
#include "src/ui/widgets.h"
#include "src/ui/text.h"
#include "src/ui/screens/bookmarks/bookmark_list_screen.h"
#include "src/ui/screens/bookmarks/book_select_screen.h"
#include "src/ui/screens/bookmarks/session.h"
#include "src/ui/statusbar.h"
#include "src/storage/library.h"
#include "src/storage/statistics.h"
#include "src/storage/page_cache.h"
#include "src/storage/book_metadata.h"
#include "src/storage/preferences_store.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>

Catalog g_library;
BookmarkSession g_bookmarkSession;
const char* bookPath(int index){return index==0?g_library.books[0].path:nullptr;}
int bookIndexForPath(const String& path){return path==g_library.books[0].path?0:-1;}
std::vector<uint8_t> load(const std::string& path){std::ifstream file(path,std::ios::binary);assert(file);return {std::istreambuf_iterator<char>(file),{}};}
std::vector<uint32_t> oracle(const std::vector<uint8_t>& bytes){
 StringReadStream stream{String(std::string(bytes.begin(),bytes.end()))};std::vector<uint32_t> offsets{0};
 while(offsets.back()<bytes.size()){
  const auto next=nextPageOffset(stream,offsets.back());assert(next>offsets.back());
  if(next>=bytes.size())break;offsets.push_back(next);
 }
 return offsets;
}
int findChecked(uint32_t offset){
 const auto previous=getCpuFrequencyMhz();const int result=findPageForOffset(offset);
 assert(getCpuFrequencyMhz()==previous);return result;
}
void content(const std::vector<uint8_t>& raw,uint32_t offset){
 FileReadStream stream(g_bookview.book.file());assert(stream.seek(offset));
 for(size_t n=0;n<32&&offset+n<raw.size();++n)assert(stream.read()==raw[offset+n]);
}
void cacheFaults(const String& path){
 const auto layout=Font::layoutForCache();const auto size=g_bookview.book.size();const String cache=String("/pc_")+prefKeyForBook(path)+".bin";
 persistReaderState();const auto valid=FS.nodes.at(cache.c_str())->bytes;assert(valid.size()<=40022);
 for(size_t location:{size_t(0),size_t(4),size_t(8),size_t(12),valid.size()-1}){
  auto damaged=valid;damaged[location]^=0xff;FS.put(cache.c_str(),damaged);PageOffsetTable table;
  assert(!loadPageOffsetCacheForBook(path,size,layout,table));assert(table.firstPage==0&&table.count==1&&table.offsetAt(0)==0);
 }
 auto truncated=valid;truncated.pop_back();FS.put(cache.c_str(),truncated);PageOffsetTable table;assert(!loadPageOffsetCacheForBook(path,size,layout,table));
 FS.put(cache.c_str(),valid);
 writeBudget=5;savePageOffsetCacheForBook(path,size,layout,g_bookview.pages);writeBudget=(std::numeric_limits<size_t>::max)();
 assert(FS.nodes.at(cache.c_str())->bytes==valid&&!FS.exists(cache+".tmp"));
 FS.failRename=true;savePageOffsetCacheForBook(path,size,layout,g_bookview.pages);FS.failRename=false;
 assert(FS.nodes.at(cache.c_str())->bytes==valid&&!FS.exists(cache+".tmp"));
 assert(loadPageOffsetCacheForBook(path,size,layout,table));assert(table.firstPage==g_bookview.pages.firstPage&&table.endPage()==g_bookview.pages.endPage());
 auto changed=layout;changed.lineGap=4;assert(!loadPageOffsetCacheForBook(path,size,changed,table));
 uint32_t untouched=42;assert(loadOffsetForPageFromDisk(path,size,layout,0,&untouched)==-1&&untouched==42);
 uint32_t last=0;assert(loadOffsetForPageFromDisk(path,size,layout,INT_MAX,&last)==g_bookview.pages.endPage()-1);assert(last==g_bookview.pages.offsets[g_bookview.pages.count-1]);
}
void exercise(const char* name,const std::vector<uint8_t>& stored,const std::vector<uint8_t>& raw){
 resetBookView();FS.nodes.clear();prefs.clear();Font::setFamily(Font::Family::Helvetica);Font::setBodySize(14);Font::setLineGap(0);Font::setBionic(false);Statusbar::setMode(Statusbar::Full);resetBookView();
 const String path=String("/books/")+name;strncpy(g_library.books[0].path,path.c_str(),sizeof(g_library.books[0].path));g_library.bookCount=1;FS.put(path.c_str(),stored);
 const auto offsets=oracle(raw);assert(offsets.size()>MAX_PAGES);assert(openBookByIndex(0));
 fileReads=0;const int scanned=findChecked(uint32_t(raw.size()-1));const auto scanReads=fileReads;
 assert(scanned==int(offsets.size()-1)&&scanReads<10000);
 std::cout<<name<<": full scan filesystem reads="<<scanReads<<" pages="<<offsets.size()<<"\n";
 const auto clocks=cpuSwitches;assert(findChecked(offsets.back())==scanned&&cpuSwitches==clocks);
 resetBookView();assert(openBookByIndex(0));renderCurrentPage();
 for(size_t page=0;page<offsets.size();++page){
  assert(g_bookview.cursor.pageIndex==int(page)&&g_bookview.pages.contains(int(page)));assert(g_bookview.pages.offsetAt(int(page))==offsets[page]);
  assert(g_bookview.pages.count<=MAX_PAGES);
  if(page==9999||page==10000||page+1==offsets.size()){renderCurrentPage();content(raw,offsets[page]);}
  assert(advancePage()==(page+1<offsets.size()));
 }
 assert(g_bookview.pages.eofReached&&g_bookview.pages.endPage()==int(offsets.size()));const int finalPage=g_bookview.cursor.pageIndex;
 // The window saved before the last roll makes the first reverse crossing fast.
 const int boundary=g_bookview.pages.firstPage;assert(boundary>0);g_bookview.cursor.pageIndex=boundary;fileReads=0;
 assert(retreatPage()&&g_bookview.cursor.pageIndex==boundary-1);assert(g_bookview.pages.offsetAt(boundary-1)==offsets[boundary-1]);assert(fileReads<400);
 assert(advancePage()&&g_bookview.cursor.pageIndex==boundary);assert(g_bookview.pages.offsetAt(boundary)==offsets[boundary]);
 g_bookview.cursor.pageIndex=findChecked(uint32_t(raw.size()-1));assert(g_bookview.cursor.pageIndex==finalPage);
 persistReaderState();armResumeOnWake();resetBookView();fileReads=0;
 assert(tryRestoreReadingSession()&&g_bookview.cursor.pageIndex==finalPage&&!prefs.isKey("wake_path"));assert(fileReads<500);
 assert(g_bookview.pages.offsetAt(finalPage)==offsets.back());content(raw,offsets.back());
 cacheFaults(path);
 // Backwards crossing when only the late cache survives must rebuild safely.
 const int first=g_bookview.pages.firstPage;g_bookview.cursor.pageIndex=first;cpuFrequency=160;const auto switchesBefore=cpuSwitches;assert(retreatPage());assert(g_bookview.cursor.pageIndex==first-1&&g_bookview.pages.offsetAt(first-1)==offsets[first-1]);assert(cpuFrequency==160&&cpuSwitches>switchesBefore);
 const int earlyPage=int(std::upper_bound(offsets.begin(),offsets.end(),7)-offsets.begin())-1;
 g_bookview.cursor.pageIndex=findChecked(7);assert(g_bookview.cursor.pageIndex==earlyPage&&g_bookview.pages.offsetAt(earlyPage)==offsets[earlyPage]);
 if(offsets.size()>70001){
  g_bookview.cursor.pageIndex=findChecked(offsets[70000]);assert(g_bookview.cursor.pageIndex==70000);assert(addBookmarkForCurrentBook());
  PreferencesStore store(prefs);const auto mark=loadBookmarks(store,g_bookview.book.key());assert(mark.count==1&&mark.pages[0]==70000&&mark.offsets[0]==offsets[70000]);
 }
 const auto anchor=offsets[offsets.size()/2];g_bookview.cursor.pageIndex=findChecked(anchor);Font::setBodySize(12);repaginateForLayoutChange();
 assert(cpuFrequency==160);cpuFrequency=80;
 const auto changed=oracle(raw);const auto wanted=int(std::upper_bound(changed.begin(),changed.end(),anchor)-changed.begin())-1;
 assert(g_bookview.cursor.pageIndex==wanted&&g_bookview.pages.offsetAt(wanted)==changed[wanted]);
 std::cout<<name<<": "<<raw.size()<<" logical bytes, "<<offsets.size()<<" pages, all offsets/EOF/reverse/cache/resume/layout passed\n";
}
void cancelledIndex(){
 resetBookView();FS.nodes.clear();prefs.clear();Font::setBodySize(14);Statusbar::setMode(Statusbar::Full);resetBookView();
 const String path="/books/cancel.txt";strcpy(g_library.books[0].path,path.c_str());g_library.bookCount=1;
 std::vector<uint8_t> bytes;for(int n=0;n<10000;++n)for(uint8_t c:{uint8_t('a'),uint8_t('\n'),uint8_t('\n')})bytes.push_back(c);FS.put(path.c_str(),bytes);
 PreferencesStore kv(prefs);const String key=prefKeyForBook(path);saveSavedOffset(kv,key,20000);saveSavedPage(kv,key,2000);
 navigatedToLibrary=false;indexCancelAfter=10;cpuFrequency=160;
 assert(!openBookByIndex(0)&&readerIndexWasCancelled()&&navigatedToLibrary&&!g_bookview.book.isOpen()&&cpuFrequency==160);
 assert(loadSavedOffset(kv,key)==20000&&loadSavedPage(kv,key)==2000);
 resetBookView();indexCancelAfter=-1;assert(openBookByIndex(0));
 const auto oldPage=g_bookview.cursor.pageIndex;const auto turns=recordedPageTurns;g_bookview.pages.reset();
 indexCancelAfter=10;navigatedToLibrary=false;assert(!retreatPage()&&readerIndexWasCancelled()&&navigatedToLibrary&&g_bookview.cursor.pageIndex==oldPage&&recordedPageTurns==turns);
 persistReaderState();assert(loadSavedOffset(kv,key)==20000&&loadSavedPage(kv,key)==2000);
 resetBookView();indexCancelAfter=-1;assert(openBookByIndex(0));saveProgress();const auto saved=loadSavedOffset(kv,key);
 Font::setBodySize(12);indexCancelAfter=10;navigatedToLibrary=false;repaginateForLayoutChange();
 assert(readerIndexWasCancelled()&&navigatedToLibrary&&g_bookview.cursor.pageIndex==-1);persistReaderState();assert(loadSavedOffset(kv,key)==saved);
 resetBookView();indexCancelAfter=-1;assert(openBookByIndex(0)&&!readerIndexWasCancelled());
 resetBookView();prefs.remove((key+"_off").c_str());saveSavedPage(kv,key,5000);assert(openBookByIndex(0));
 indexCancelAfter=10;navigatedToLibrary=false;const auto errors=readerErrors;renderCurrentPage();
 assert(readerIndexWasCancelled()&&navigatedToLibrary&&readerErrors==errors);persistReaderState();
 assert(loadSavedOffset(kv,key)==kOffsetUnset&&loadSavedPage(kv,key)==5000);
 resetBookView();indexCancelAfter=-1;cpuFrequency=80;
 std::cout<<"Cancelled opening, backwards scan and layout rebase preserve position, stats and caller clock; reopen works\n";
}
void crossBookLookup(const char* folder){
 resetBookView();FS.nodes.clear();prefs.clear();Font::setFamily(Font::Family::Helvetica);Font::setBodySize(14);Font::setLineGap(0);Font::setBionic(false);Statusbar::setMode(Statusbar::Full);resetBookView();
 std::vector<uint8_t> bytes;for(int i=0;i<20000;++i)for(uint8_t c:{uint8_t('a'),uint8_t('\n'),uint8_t('\n')})bytes.push_back(c);
 const String path="/books/cross.txt",active="/books/active.txt",key=prefKeyForBook(path);FS.put(path.c_str(),bytes);FS.put(active.c_str(),bytes);
 strcpy(g_library.books[0].path,active.c_str());g_library.bookCount=1;assert(openBookByIndex(0));g_bookview.cursor.pageIndex=findChecked(100);persistReaderState();
 const auto activePosition=g_bookview.book.file().position();const auto activePage=g_bookview.cursor.pageIndex;const auto values=prefs.values;const auto turns=recordedPageTurns;
 const auto offsets=oracle(bytes);assert(offsets.size()>3000);File file=FS.open(path,"r");
 struct Progress {int calls=0,limit=-1;uint32_t first=UINT32_MAX,last=0;};
 auto callback=[](uint32_t current,uint32_t target,void* ptr)->bool{auto& p=*static_cast<Progress*>(ptr);assert(current<target&&getCpuFrequencyMhz()==240);if(!p.calls)p.first=current;else assert(current==p.last+1);p.last=current;++p.calls;return p.limit<0||p.calls<=p.limit;};
 for(uint32_t clock:{80u,160u,240u}){cpuFrequency=clock;Progress progress;progress.limit=4;assert(pageOffsetForPage(file,path,3000,callback,&progress)==kOffsetUnset&&progress.calls==5&&progress.first==0&&cpuFrequency==clock);assert(resolveBookmarkOffset(path,3000,kOffsetUnset,callback,&(progress=Progress{0,0}))==kOffsetUnset&&cpuFrequency==clock);}
 Progress complete;assert(pageOffsetForPage(file,path,3000,callback,&complete)==offsets[3000]&&complete.calls==3000);
 const auto feedbackBefore=indexFeedbackSteps;indexCancelAfter=0;beginPageCanvas();std::vector<uint8_t> frame(gfx.pixels,gfx.pixels+250*122);
 assert(pageOffsetForPage(file,path,30)==offsets[30]&&resolveBookmarkOffset(path,30,kOffsetUnset)==offsets[30]);assert(indexFeedbackSteps==feedbackBefore&&indexCancelAfter==0&&std::equal(frame.begin(),frame.end(),gfx.pixels));
 Progress huge;huge.limit=0;assert(resolveBookmarkOffset(path,UINT32_MAX,kOffsetUnset,callback,&huge)==kOffsetUnset&&huge.calls==1);
 Progress canonical;canonical.limit=0;assert(resolveBookmarkOffset(path,3000,offsets[3000],callback,&canonical)==offsets[3000]&&canonical.calls==0);
 PageOffsetTable cache;cache.reset();cache.firstPage=1000;cache.count=10;for(int i=0;i<10;++i)cache.offsets[i]=offsets[1000+i];savePageOffsetCacheForBook(path,bytes.size(),Font::layoutForCache(),cache);
 Progress cached;cached.limit=0;assert(pageOffsetForPage(file,path,1005,callback,&cached)==offsets[1005]&&cached.calls==0);
 Progress partial;partial.limit=2;assert(pageOffsetForPage(file,path,1030,callback,&partial)==kOffsetUnset&&partial.first==1009);
 deletePageCacheForBook(path);assert(prefs.values==values&&recordedPageTurns==turns&&g_bookview.book.path()==active&&g_bookview.cursor.pageIndex==activePage&&g_bookview.book.file().position()==activePosition&&FS.nodes[path.c_str()]->bytes==bytes);
 resetBookView();strcpy(g_library.books[0].path,path.c_str());uint32_t pages[]={3000,3010,3020},unknown[]={kOffsetUnset,kOffsetUnset,kOffsetUnset};saveBookmarksForKey(key,pages,unknown,3);
 const auto bookmarkValues=prefs.values;g_bookmarkSession.bookIndex=0;g_bookmarkSession.selectedIndex=0;
 BookmarkListScreen list;paintIndexFeedback=true;indexCancelAfter=2;list.onEnter();assert(list.nextScreen==&g_bmBookSelectScreen&&prefs.values==bookmarkValues&&!g_bookview.book.isOpen());
 // Backfilled final windows can resolve preceding legacy marks: cancellation
 // must still leave the stored metadata and active reader untouched.
 g_bookmarkSession.selectedIndex=3;list.nextScreen=nullptr;indexCancelAfter=2;list.draw();
 assert(list.nextScreen==&g_bmBookSelectScreen&&prefs.values==bookmarkValues&&!g_bookview.book.isOpen());g_bookmarkSession.selectedIndex=0;
 // Successful foreground feedback must be erased before the final list is painted.
 list.nextScreen=nullptr;indexCancelAfter=-1;list.onEnter();assert(!list.nextScreen&&prefs.values==bookmarkValues);assert(gfx.pixels[121*250+249]==0);gfx.save((std::string(folder)+"/bookmarks-rebuilt.pbm").c_str());std::vector<uint8_t> rebuilt(gfx.pixels,gfx.pixels+250*122);
 uint32_t canonicalOffsets[]={offsets[3000],offsets[3010],offsets[3020]};saveBookmarksForKey(key,pages,canonicalOffsets,3);const int beforeFast=indexFeedbackSteps;paintIndexFeedback=false;list.draw();assert(indexFeedbackSteps==beforeFast&&std::equal(rebuilt.begin(),rebuilt.end(),gfx.pixels));
 list.onButton({ButtonEvent::Short});list.onButton({ButtonEvent::Short});list.onButton({ButtonEvent::Short});for(int y:{28,60,92})assert(gfx.pixels[y*250+125]);list.onButton({ButtonEvent::Long});assert(list.nextScreen==&g_bmBookSelectScreen);
 const String rawPath="/books/cross-raw.txt",packedPath="/books/cross-packed.pbt";
 const auto rawBytes=load(std::string(folder)+"/5.raw"),packedBytes=load(std::string(folder)+"/5.pbt");FS.put(rawPath.c_str(),rawBytes);FS.put(packedPath.c_str(),packedBytes);
 File rawFile=FS.open(rawPath,"r"),packedFile=FS.open(packedPath,"r");const auto clock=cpuFrequency;
 Progress packedCancel;packedCancel.limit=3;assert(pageOffsetForPage(packedFile,packedPath,80,callback,&packedCancel)==kOffsetUnset&&cpuFrequency==clock);
 assert(pageOffsetForPage(packedFile,packedPath,80)==pageOffsetForPage(rawFile,rawPath,80)&&FS.nodes[packedPath.c_str()]->bytes==packedBytes);
 assert(recordedPageTurns==turns&&FS.nodes[path.c_str()]->bytes==bytes);indexCancelAfter=-1;cpuFrequency=80;resetBookView();
 std::cout<<"Cross-book lookups: cancel sentinel, cached/uncached/stored offsets, caller clocks, background input/display isolation, active position/NVS preservation and actual bookmark-list frame/return passed\n";
}
void dirtyLayoutPersistence(){
 resetBookView();FS.nodes.clear();prefs.clear();Font::setFamily(Font::Family::Helvetica);Font::setBodySize(14);Font::setLineGap(0);Font::setBionic(false);Statusbar::setMode(Statusbar::Full);resetBookView();
 const String path="/books/dirty-layout.txt";strcpy(g_library.books[0].path,path.c_str());g_library.bookCount=1;
 std::vector<uint8_t> bytes;const std::string line="alpha beta gamma delta epsilon.\n\n";for(int n=0;n<3200;++n)bytes.insert(bytes.end(),line.begin(),line.end());FS.put(path.c_str(),bytes);
 assert(openBookByIndex(0));g_bookview.cursor.pageIndex=findChecked(uint32_t(bytes.size()/2));const auto anchor=g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);persistReaderState();
 Font::setBodySize(12);markPagesDirtyForLayoutChange();persistReaderState();PreferencesStore store(prefs);assert(loadSavedOffset(store,prefKeyForBook(path))==anchor);
 PageOffsetTable table;assert(!loadPageOffsetCacheForBook(path,bytes.size(),Font::layoutForCache(),table));
 resetBookView();assert(openBookByIndex(0));const auto expected=oracle(bytes);const int page=int(std::upper_bound(expected.begin(),expected.end(),anchor)-expected.begin())-1;
 assert(g_bookview.cursor.pageIndex==page&&g_bookview.pages.offsetAt(page)==expected[page]);persistReaderState();assert(loadPageOffsetCacheForBook(path,bytes.size(),Font::layoutForCache(),table));
 std::cout<<"Dirty-layout close/sleep persistence keeps canonical position and never stamps old page offsets as a new layout; reopen rebases correctly\n";
}
void legacyCache(){
 resetBookView();Font::setBodySize(14);Statusbar::setMode(Statusbar::Full);
 const String path="/books/legacy.txt",cache=String("/pc_")+prefKeyForBook(path)+".bin";
 // Original cache format with old font metrics must be rejected.
 const std::vector<uint8_t> bytes={0x4a,0x4f,0x43,0x50,14,0,0,8,9,0,0,0,3,0,0,0,0,0,3,0,0,0,6,0,0,0};
 FS.put(cache.c_str(),bytes);PageOffsetTable table;
 assert(!loadPageOffsetCacheForBook(path,9,Font::layoutForCache(),table));
 uint32_t untouched=99;assert(loadOffsetForPageFromDisk(path,9,Font::layoutForCache(),1,&untouched)==-1&&untouched==99);
 // The same prefix format remains supported when its font revision matches.
 auto revised=bytes;revised[4]|=128;revised[6]|=64;FS.put(cache.c_str(),revised);
 assert(loadPageOffsetCacheForBook(path,9,Font::layoutForCache(),table));assert(table.firstPage==0&&table.count==3&&table.offsetAt(2)==6);
 uint32_t offset=99;assert(loadOffsetForPageFromDisk(path,9,Font::layoutForCache(),1,&offset)==1&&offset==3);
 auto bad=revised;bad[18]=0;FS.put(cache.c_str(),bad);assert(!loadPageOffsetCacheForBook(path,9,Font::layoutForCache(),table));
 std::cout<<"Legacy prefix cache: old font rejection, matching revision load/lookup, nonmonotonic rejection passed\n";
}
void bookmarkPreviewReadOnly(const char* folder) {
 resetBookView();FS.nodes.clear();prefs.clear();Font::setFamily(Font::Family::Helvetica);Font::setBodySize(10);Font::setBionic(false);Statusbar::setMode(Statusbar::Full);
 const std::string path="/books/preview.txt";std::string words="Über den Fluss führt ein schmaler Weg. Grüße aus Köln. Zażółć gęślą jaźń. Читаємо українською мовою. ";std::string raw;
 for(int n=0;n<100;++n)raw+=words;
 FS.put(path,{raw.begin(),raw.end()});strncpy(g_library.books[0].path,path.c_str(),sizeof(g_library.books[0].path));g_library.bookCount=1;
 assert(openBookByIndex(0));persistReaderState();g_bookview.cursor.pageIndex=findPageForOffset(1500);
 const auto anchor=g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);const auto index=g_bookview.cursor.pageIndex;
 const auto values=prefs.values;const auto layout=Font::layoutForCache();const auto turns=recordedPageTurns;
 g_bookview.book.file().seek(42);const auto filePosition=g_bookview.book.file().position();
 assert(renderBookmarkPreview(true));gfx.save((std::string(folder)+"/preview-read.pbm").c_str());
 assert(prefs.values==values&&recordedPageTurns==turns&&g_bookview.cursor.pageIndex==index&&g_bookview.pages.offsetAt(index)==anchor&&g_bookview.book.file().position()==filePosition);
 assert(renderBookmarkPreview(false));gfx.save((std::string(folder)+"/preview-back.pbm").c_str());assert(prefs.values==values&&recordedPageTurns==turns);
 const auto after=Font::layoutForCache();assert(layout.family==after.family&&layout.bodySize==after.bodySize&&layout.lineGap==after.lineGap&&layout.bionic==after.bionic);
 for(size_t language=0;language<Language::count();++language){
   assert(Language::select(Language::codeAt(language),inflateBookBlock));
   for(bool read:{true,false}){assert(renderBookmarkPreview(read));gfx.save((std::string(folder)+"/preview-"+Language::currentCode()+(read?"-read.pbm":"-back.pbm")).c_str());assert(prefs.values==values&&recordedPageTurns==turns&&g_bookview.pages.offsetAt(index)==anchor);}
 }
 assert(Language::select(Language::defaultCode,inflateBookBlock));
 std::cout<<"PASS bookmark excerpt: real file/paginator/fonts/UI, both actions, unchanged canonical offset/cursor/file position/font/settings/progress/counters\n";
}

uint32_t paginateReference(IReadStream&,uint32_t,const LayoutMetrics&,const MeasureFn&,const LineCallback&);
void paragraphAndPrefetch() {
  resetBookView();FS.nodes.clear();prefs.clear();
  std::string text;
  for(int n=0;n<60;++n)text+="Über den Fluss. café, Größe, é. Читаємо українською.\n\n";
  FS.put("/books/layout.txt",{text.begin(),text.end()});File f=FS.open("/books/layout.txt","r");
  unsigned cases=0;
  for(auto family:{Font::Family::Helvetica,Font::Family::OpenDyslexic})for(int size:{8,10,12,14})for(bool bionic:{false,true})for(bool half:{false,true}){
    Font::setFamily(family);Font::setBodySize(size);Font::setBionic(bionic);Font::setHalfParagraphGaps(half);
    Statusbar::setMode(Statusbar::Full);const auto layout=Font::layoutForCache();
    assert(bool(layout.halfGaps)==half);
    uint32_t pos=0;
    for(int page=0;page<3;++page){
      clearPagePrefetch();beginPageCanvas();const auto next=drawPageAt(f,pos);
      std::vector<uint8_t> direct(gfx.pixels,gfx.pixels+sizeof(gfx.pixels));
      prefetchPage(f,pos);beginPageCanvas();assert(drawPageAt(f,pos)==next);
      assert(std::equal(direct.begin(),direct.end(),gfx.pixels));
      FileReadStream stream(f);assert(nextPageOffset(stream,pos)==next);
      if(!half){StringReadStream original(text.c_str());std::vector<std::string> expected,actual;
        const auto oldNext=paginateReference(original,pos,Font::bodyLayout(),Font::measureBionicLine,[&](const char* s,size_t n){expected.emplace_back(s,n);});
        const auto newNext=paginatePage(stream,pos,Font::bodyLayout(),Font::measureBionicLine,[&](const char* s,size_t n){actual.emplace_back(s,n);});
        assert(oldNext==newNext&&expected==actual);
      }
      assert(next>pos);pos=next;++cases;
    }
  }
  Font::setHalfParagraphGaps(false);Font::setBodySize(10);Font::setBionic(false);
  std::cout<<"PASS "<<cases<<" actual-font paragraph/prefetch/direct pixel cases and original/new Bionic page equivalence\n";
}

void spacingRegression(const char* folder) {
  resetBookView();FS.nodes.clear();prefs.clear();Font::setBodySize(10);Font::setFamily(Font::Family::Helvetica);Font::setBionic(true);Font::setLineGap(0);Font::setHalfParagraphGaps(false);Statusbar::setMode(Statusbar::Full);
  const std::string sample="consumption measurement pattern\n6-2-2: The deep sleep power is the consumed power when the panel controller is in deep sleep mode.\n6-2-3: The listed electrical/optical parameters are measured under normal operating conditions.\n";
  FS.put("/books/spacing.txt",{sample.begin(),sample.end()});File f=FS.open("/books/spacing.txt","r");
  beginPageCanvas();LayoutMetrics old=Font::bodyLayout();const auto full=PalaFonts::bounds(false,10);old.ascent=full.ascent;old.descent=full.descent;old.lineH=old.ascent-old.descent;old.maxLines=(SCREEN_H-TOP_PAD-BOT_PAD-Statusbar::reserveH())/old.lineH;old.paragraphGapH=old.lineH;
  int y=old.ascent,oldLines=0;FileReadStream original(f);const auto oldNext=paginatePage(original,0,old,Font::measureBionicLine,[&](const char* line,size_t){Font::drawBionicLine(MARGIN_X,y,line);y+=old.lineH;++oldLines;});
  gfx.save((std::string(folder)+"/spacing-before.pbm").c_str());
  clearPagePrefetch();beginPageCanvas();const auto newNext=drawPageAt(f,0);gfx.save((std::string(folder)+"/spacing-after.pbm").c_str());String text;assert(extractPageText(f,0,text)==newNext);int newLines=0;for(size_t n=0;n<text.length();++n)if(text[n]=='\n')++newLines;
  assert(Font::bodyLayout().lineH==16&&old.lineH==22&&newNext>oldNext&&newLines>oldLines);
  std::cout<<"PASS photo-text spacing regression: 10pt gap0 pitch22->16, lines "<<oldLines<<"->"<<newLines<<", same glyphs and typography settings\n";
  const char* accented="ǺǗǜ ĄĘŁŹŻ český příběh. Її Йй Ёё. é ë í\n\n";
  std::string rare;for(int n=0;n<50;++n)rare+=accented;FS.put("/books/accents.txt",{rare.begin(),rare.end()});File special=FS.open("/books/accents.txt","r");
  for(auto family:{Font::Family::Helvetica,Font::Family::OpenDyslexic})for(int size:{8,10,12,14})for(bool bionic:{false,true})for(bool half:{false,true}){
    Font::setFamily(family);Font::setBodySize(size);Font::setBionic(bionic);Font::setHalfParagraphGaps(half);
    const auto m=Font::bodyLayout();
    for(bool bold:{false,true}){if(bold)Font::useBold();else Font::useBody();for(unsigned cp=32;cp<127;++cp){int a=0,d=0;Font::glyphVerticalBounds(u8g2,cp,a,d);assert(a<=m.ascent&&d>=m.descent);}}
    uint32_t pos=0;for(int page=0;page<4;++page){clearPagePrefetch();beginPageCanvas();const auto next=drawPageAt(special,pos);FileReadStream stream(special);assert(next==nextPageOffset(stream,pos));String extracted;assert(extractPageText(special,pos,extracted)==next);assert(next>pos);
      for(int row=SCREEN_H-BOT_PAD-Statusbar::reserveH();row<SCREEN_H;++row)for(int x=0;x<SCREEN_W;++x)assert(!gfx.pixels[row*250+x]);pos=next;}
  }
  Font::setHalfParagraphGaps(false);Font::setBionic(false);Font::setFamily(Font::Family::Helvetica);Font::setBodySize(14);Font::setLineGap(0);resetBookView();
  std::cout<<"PASS adaptive accents: both families, all sizes, Bionic/half paragraphs; ASCII compact bounds, draw/extract/index offsets and bottom safety\n";
}

int main(int argc,char**argv){assert(argc==2);u8g2.begin(gfx);u8g2.setFontMode(1);const std::string root=argv[1];
 spacingRegression(argv[1]);paragraphAndPrefetch();const auto raw=load(root+"/5.raw"),packed=load(root+"/5.pbt");exercise("large.txt",raw,raw);exercise("large.pbt",packed,raw);
 std::vector<uint8_t> poetry;for(int i=0;i<240000;++i)for(uint8_t c:{uint8_t('a'),uint8_t('\n'),uint8_t('\n')})poetry.push_back(c);
 exercise("poetry.txt",poetry,poetry);bookmarkPreviewReadOnly(argv[1]);legacyCache();cancelledIndex();crossBookLookup(argv[1]);dirtyLayoutPersistence();std::cout<<"PASS: actual reader, text, fonts, cache, progress and bookmarks with bounded RAM\n";
}
