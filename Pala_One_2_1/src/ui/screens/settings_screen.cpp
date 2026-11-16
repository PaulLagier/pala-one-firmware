#include "src/ui/screens/settings_screen.h"
#include "src/hal/display.h"
#include "src/ui/font.h"
#include "src/ui/widgets.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screens/statistics_screen.h"
#include "src/ui/screens/update_screen.h"
#include "src/ui/screens/about_screen.h"

void SettingsScreen::onEnter(){nextScreen=nullptr;selection_=0;draw();}
void SettingsScreen::draw(){
  prepareMenuFrame();
  drawPalaList(D_MENU_DEVICE,4,selection_,[](int index)->String{
    switch(index){case 0:return D_MENU_STATISTICS;case 1:return D_MENU_UPDATE;
      case 2:return D_ABOUT_FIRMWARE;default:return D_READER_BACK_LIBRARY;}
  });
  Font::useBody();display.update();
}
void SettingsScreen::onButton(const ButtonEvent& event){
  if(!event.any())return;
  if(event.kind==ButtonEvent::Triple){nextScreen=&g_libraryScreen;return;}
  if(event.kind==ButtonEvent::Short){selection_=(selection_+1)%4;draw();return;}
  if(event.kind!=ButtonEvent::Long&&event.kind!=ButtonEvent::Double)return;
  switch(selection_){case 0:nextScreen=&g_statsScreen;break;case 1:nextScreen=&g_updateScreen;break;
    case 2:nextScreen=&g_aboutScreen;break;default:nextScreen=&g_libraryScreen;break;}
}
