#include "src/hal/display.h"
#include "src/hal/file_stream.h"
#include "src/ui/reader.h"
#include "src/ui/reader_index_feedback.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/font.h"
#include "src/ui/font_render.h"
#include "src/ui/language_fonts.h"

#include "src/ui/widgets.h"
#include "src/ui/text.h"



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
 const auto previous=getCpuFrequencyMhz();const int result=findPageForOffset(offset,true);
 assert(getCpuFrequencyMhz()==previous);return result;
}
void content(const std::vector<uint8_t>& raw,uint32_t offset){
 IReadStream& stream=g_bookview.book.stream();assert(stream.seek(offset));
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

void stagedPageReuse() {
 resetBookView();FS.nodes.clear();prefs.clear();
 const String path="/books/prefetch.txt";strcpy(g_library.books[0].path,path.c_str());g_library.bookCount=1;
 std::vector<uint8_t> bytes;const std::string words="alpha beta gamma delta epsilon. ";for(int i=0;i<100;++i)bytes.insert(bytes.end(),words.begin(),words.end());FS.put(path.c_str(),bytes);
 assert(openBookByIndex(0));auto& stream=g_bookview.book.stream();
 for(int size:{8,10,12,14})for(bool bionic:{false,true}){
  Font::setBodySize(size);Font::setBionic(bionic);clearPagePrefetch();
  prefetchPage(stream,0);const auto end=stream.position();beginPageCanvas();
  const auto next=drawPageAt(stream,0);assert(stream.position()==end);
  std::vector<uint8_t> cached(gfx.pixels,gfx.pixels+250*122);
  clearPagePrefetch();beginPageCanvas();assert(drawPageAt(stream,0)==next);
  assert(std::equal(cached.begin(),cached.end(),gfx.pixels));
 }
 resetBookView();
 std::cout<<"Idle prefetch reuse preserves stream position and exact rendered pixels for all sizes and Bionic states\n";
}
int main(int argc,char** argv) {
 assert(argc==2);u8g2.begin(gfx);Font::loadSettings();
 auto raw=load(std::string(argv[1])+"/5.raw");
 auto packed=load(std::string(argv[1])+"/5.pbt");
 exercise("large.txt",raw,raw);exercise("large.pbt",packed,raw);
 std::vector<uint8_t> poetry;for(int i=0;i<320000;++i){poetry.push_back('a');poetry.push_back('\n');poetry.push_back('\n');}
 exercise("poetry.txt",poetry,poetry);
 legacyCache();cancelledIndex();dirtyLayoutPersistence();stagedPageReuse();
 std::cout<<"Actual reader window, canonical progress, format/CRC failures and cancellation passed\n";
}
