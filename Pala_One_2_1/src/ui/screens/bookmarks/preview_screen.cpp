#include "src/ui/screens/bookmarks/preview_screen.h"
#include "src/ui/reader.h"
#include "src/ui/screens/bookmarks/bookmark_list_screen.h"
#include "src/ui/screens/reader_screen.h"
void BookmarkPreviewScreen::onEnter(){nextScreen=nullptr;readSelected_=true;draw();}
void BookmarkPreviewScreen::cancel(){resetBookView();nextScreen=&g_bmListScreen;}
void BookmarkPreviewScreen::draw(){if(!renderBookmarkPreview(readSelected_))cancel();}
void BookmarkPreviewScreen::onButton(const ButtonEvent& e){
  if(e.kind==ButtonEvent::Triple){cancel();return;}
  if(e.kind==ButtonEvent::Short){readSelected_=!readSelected_;draw();return;}
  if(e.kind==ButtonEvent::Long||e.kind==ButtonEvent::Double){
    if(!readSelected_){cancel();return;}
    persistReaderState();nextScreen=&g_readerScreen;
  }
}
void BookmarkPreviewScreen::onSleep(){resetBookView();}
