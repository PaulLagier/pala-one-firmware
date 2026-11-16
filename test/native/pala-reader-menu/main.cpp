#include "src/config.h"
#include "src/ui/reader_menu.h"
#include "src/ui/screens/reader_screen.h"
#include "runtime.h"
#include <iostream>
int main(){ReaderScreen s;currentScreen=&s;
 auto open=[&](){s.nextScreen=nullptr;g_bookview=BookView{};ReaderMenu::close();toast.clear();ReaderMenu::open();};
 open();assert(focus==D_READER_MENU_CONTINUE&&labels.size()==4);s.onButton({ButtonEvent::Long});assert(!ReaderMenu::isActive()&&bookmarks==0&&!s.nextScreen);
 open();s.onButton({ButtonEvent::Short});assert(focus==D_READER_MENU_BOOKMARK);s.onButton({ButtonEvent::Long});assert(bookmarks==1&&!ReaderMenu::isActive()&&toast=="Saved");
 open();s.onButton({ButtonEvent::Short});s.onButton({ButtonEvent::Short});assert(focus==D_READER_BACK_LIBRARY);s.onButton({ButtonEvent::Long});assert(s.nextScreen==&g_libraryScreen&&!ReaderMenu::isActive()&&persists>0);
 open();for(int i=0;i<3;++i)s.onButton({ButtonEvent::Short});assert(focus==D_READER_MENU_CONTINUE);s.onButton({ButtonEvent::Double});assert(!ReaderMenu::isActive()&&!s.nextScreen);
 open();s.onButton({ButtonEvent::Short});s.onButton({ButtonEvent::Double});assert(bookmarks==2&&!ReaderMenu::isActive());
 open();s.onButton({ButtonEvent::Short});s.onButton({ButtonEvent::Short});s.onButton({ButtonEvent::Double});assert(s.nextScreen==&g_libraryScreen&&!ReaderMenu::isActive());
 open();s.onIdleTick();assert(prefetches==0);assert(dirtyMarks==0&&Font::size==10&&Font::gap==0&&!Font::bionic);
 g_bookview.book.opened=false;ReaderMenu::draw();assert(!ReaderMenu::isActive()&&s.nextScreen==&g_libraryScreen);
 puts("PASS production ReaderMenu/ReaderScreen: continue, bookmark, persist/return, wrapping, dismiss, no setting mutations, no prefetch while open");
}
