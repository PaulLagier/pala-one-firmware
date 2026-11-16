#include "src/ui/reader_menu.h"
#include "src/hal/display.h"
#include "src/ui/reader.h"
#include "src/ui/font.h"
#include "src/ui/widgets.h"
#include "src/ui/toast.h"
#include "src/ui/screens/library_screen.h"

namespace ReaderMenu {
namespace {bool active=false;int selection=0;}
bool isActive(){return active;}
void close(){active=false;}
void open(){
  if(!g_bookview.book.isOpen()||!g_bookview.pages.contains(g_bookview.cursor.pageIndex)){
    close();navigateToLibraryRoot();return;
  }
  saveProgress();active=true;selection=0;forceNextMenuFrameFull();draw();
}
void draw(){
  if(!active)return;
  if(!g_bookview.book.isOpen()||!g_bookview.pages.contains(g_bookview.cursor.pageIndex)){
    close();navigateToLibraryRoot();return;
  }
  prepareMenuFrame();
  drawPalaList(D_READER_MENU_TITLE,3,selection,[](int index)->String{
    return index==0?D_READER_MENU_CONTINUE:index==1?D_READER_MENU_BOOKMARK:D_READER_BACK_LIBRARY;
  });
  Font::useBody();display.update();
}
bool onButton(const ButtonEvent& event){
  if(!active||!event.any())return false;
  if(event.kind==ButtonEvent::Short){selection=(selection+1)%3;draw();return true;}
  if(event.kind!=ButtonEvent::Long&&event.kind!=ButtonEvent::Double){close();return true;}
  if(selection==1){const char* message=addBookmarkForCurrentBook();close();if(message)Toast::show(message);}
  else if(selection==2){persistReaderState();close();navigateToLibraryRoot();}
  else close();
  return true;
}
} // namespace ReaderMenu
