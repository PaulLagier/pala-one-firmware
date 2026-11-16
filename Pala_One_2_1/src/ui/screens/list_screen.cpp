#include "src/ui/pala_chrome.h"
#include "src/ui/screens/list_screen.h"

#include "src/hal/display.h"
#include "src/storage/list_items.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"

void ListScreen::onEnter() {
  if (g_list.selectedIndex < 0 || g_list.selectedIndex > g_list.count)
    g_list.selectedIndex = 0;
  draw();
}

void ListScreen::draw() {
  prepareMenuFrame();
  drawSectionHeader(D_LIST_HEADER);
  if(!listHasVisibleItems()) {
    PalaChrome::message(gfx,u8g2,D_LIST_NONE);
    PalaChrome::card(gfx,u8g2,89,D_READER_BACK_LIBRARY,true);
  } else {
    const int first=PalaChrome::firstVisible(g_list.selectedIndex,g_list.count+1);
    
    for(int row=0;row<PalaChrome::visibleCards&&first+row<=g_list.count;++row) {
      const int index=first+row,top=PalaChrome::firstCardTop+row*PalaChrome::cardStep;const bool selected=index==g_list.selectedIndex;
      if(index==g_list.count){PalaChrome::menuCard(gfx,u8g2,top,D_READER_BACK_LIBRARY,selected);continue;}
      const String fullLabel=g_list.items[index].text;String firstLine=fullLabel,secondLine;
      {PalaChrome::taskLabelFont(u8g2);splitListLabelForDisplay(fullLabel,200,firstLine,secondLine);}
      PalaChrome::taskCard(gfx,u8g2,top,firstLine.c_str(),secondLine.c_str(),selected,g_list.items[index].done);
    }
  }
  Font::useBody();display.update();
}

void ListScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;
  if (g_list.selectedIndex < 0 || g_list.selectedIndex > g_list.count)
    g_list.selectedIndex = g_list.count;
  if (!listHasVisibleItems()) {
    nextScreen = &g_libraryScreen;
    return;
  }

  switch (e.kind) {
    case ButtonEvent::Short:
      g_list.selectedIndex++;
      if (g_list.selectedIndex > g_list.count) g_list.selectedIndex = 0;
      draw();
      return;
    case ButtonEvent::Long:
      if (g_list.selectedIndex == g_list.count) {
        nextScreen = &g_libraryScreen;
        return;
      }
      if (g_list.selectedIndex >= 0 && g_list.selectedIndex < g_list.count) {
        g_list.items[g_list.selectedIndex].done = g_list.items[g_list.selectedIndex].done ? 0 : 1;
        saveListItems();
        draw();
      }
      return;
    case ButtonEvent::Double:
    case ButtonEvent::Triple:
      nextScreen = &g_libraryScreen;
      return;
    default:
      return;
  }
}
