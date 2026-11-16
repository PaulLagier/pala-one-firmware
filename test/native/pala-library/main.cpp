#include "runtime.h"
#include "src/ui/screens/library_screen.h"
#include "src/lang/locale_core.h"
#include <zlib.h>
#include <string>
#include "src/ui/screens/library_screen.cpp"
LibraryScreen g_libraryScreen;
bool inflateData(const uint8_t* in,size_t n,uint8_t* out,size_t size){uLongf count=size;return uncompress(out,&count,in,n)==Z_OK&&count==size;}
void send(ButtonEvent::Kind kind){g_libraryScreen.onButton({kind});}
void next(int count){while(count--)send(ButtonEvent::Short);}
void enter(){g_libraryScreen.nextScreen=nullptr;g_libraryScreen.onEnter();}
void book(const char* folder=""){g_library.bookCount=1;snprintf(g_library.books[0].name,80,"Story");snprintf(g_library.books[0].path,96,"/books/%s%sStory.pbt",folder,*folder?"/":"");snprintf(g_library.books[0].folder,MAX_FOLDER_PATH+1,"%s",folder);}
void three(){for(int y:{28,60,92})assert(gfx.pixels[y*250+125]);}
int main(int argc,char** argv){
 assert(argc==3&&Language::select(argv[2],inflateData));u8g2.begin(gfx);
 for(int count=0;count<130;++count)for(int selected=-1;selected<=count;++selected){
  int first=PalaMenuGeometry::firstVisible(selected,count);assert(first>=0);
  if(count<3)assert(first==0);else assert(first+3<=count);
  if(selected>=0&&selected<count)assert(selected>=first&&selected<first+3);
 }
 assert(PalaMenuGeometry::firstVisible(3,4)==1&&PalaMenuGeometry::firstVisible(4,5)==2);
 auto save=[&](const char* name){three();gfx.save((std::string(argv[1])+"/"+name+".pbm").c_str());};
 g_library=Catalog{};enter();save("empty-add");assert(opens==0);
 send(ButtonEvent::Long);assert(g_libraryScreen.nextScreen==&g_uploadScreen&&opens==0);
 enter();next(1);save("empty-settings");send(ButtonEvent::Double);assert(g_libraryScreen.nextScreen==&g_settingsScreen);
 enter();next(2);save("empty-bookmarks");send(ButtonEvent::Long);assert(g_libraryScreen.nextScreen==&g_bmBookSelectScreen&&opens==0);
 // New books replace the empty landing without hiding the first new book.
 enter();book();g_libraryScreen.draw();save("book-arrived");send(ButtonEvent::Long);assert(g_libraryScreen.nextScreen==&g_readerScreen&&lastBook==0);
 Screen* targets[]={&g_readerScreen,&g_bmBookSelectScreen,&g_uploadScreen,&g_settingsScreen};
 for(int i=0;i<4;++i){enter();next(i);three();send(ButtonEvent::Double);assert(g_libraryScreen.nextScreen==targets[i]);}
 // Existing apps/tasks never add entries to the reading-focused library.
 listVisible=true;enter();next(4);send(ButtonEvent::Long);assert(g_libraryScreen.nextScreen==&g_readerScreen);listVisible=false;
 allowOpen=false;cancelOpen=true;enter();send(ButtonEvent::Long);assert(!g_libraryScreen.nextScreen&&errors==0);
 cancelOpen=false;enter();send(ButtonEvent::Long);assert(!g_libraryScreen.nextScreen&&errors==1);allowOpen=true;
 g_library=Catalog{};g_library.folderCount=1;strcpy(g_library.folders[0],"fiction");book("fiction");resetLibraryNav();enter();save("folder-collapsed");
 int before=opens;send(ButtonEvent::Long);assert(!g_libraryScreen.nextScreen&&opens==before);save("folder-expanded");next(1);send(ButtonEvent::Long);assert(g_libraryScreen.nextScreen==&g_readerScreen&&lastBook==0);
 enter();next(1);send(ButtonEvent::Long);assert(g_libraryScreen.nextScreen==&g_readerScreen); // expansion retained by name
 g_currentScreen=&g_readerScreen;navigateToLibraryRoot();assert(g_readerScreen.nextScreen==&g_libraryScreen);
 puts("PASS actual Library: three-row windows, empty transfer/device/bookmark routes, simplified system actions, book arrival/open/cancellation, folder expansion and root returns");
}
