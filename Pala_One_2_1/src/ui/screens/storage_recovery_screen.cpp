#include "src/ui/screens/storage_recovery_screen.h"
#include "src/config.h"
#include "src/hal/display.h"
#include "src/hal/input.h"
#include "src/storage/fs_recovery.h"
#include "src/ui/font.h"
#include "src/ui/lock.h"
#include "src/ui/pala_chrome.h"
#include "src/ui/widgets.h"
#include <esp_system.h>

namespace {void restartDevice(){esp_restart();}}
StorageRecoveryScreen::StorageRecoveryScreen():actions_{fsRecoveryRetry,fsRecoveryReinitializeConfirmed,restartDevice}{}
void StorageRecoveryScreen::onEnter(){nextScreen=nullptr;page_=Page::Menu;selected_=helpPage_=0;restarting_=false;forceNextMenuFrameFull();draw();}
void StorageRecoveryScreen::menu(int selected){page_=Page::Menu;selected_=selected;resetInputFrontend();forceNextMenuFrameFull();}
void StorageRecoveryScreen::working(const char* text){prepareMenuFrame();drawSectionHeader(D_BOOT_STORAGE_ERROR);PalaChrome::message(gfx,u8g2,text);Font::useBody();display.update();}
void StorageRecoveryScreen::restart(){restarting_=true;working(D_RECOVERY_RESTART);actions_.restart();}
void StorageRecoveryScreen::draw(){
  if(restarting_)return;
  prepareMenuFrame();
  if(Lock::isLocked()){
    drawSectionHeader(D_BOOT_STORAGE_ERROR);PalaChrome::message(gfx,u8g2,D_SCREENSAVER_LOCKED);
    PalaChrome::card(gfx,u8g2,89,D_RECOVERY_UNLOCK,true);
  }else if(page_==Page::Menu){
    drawPalaList(D_BOOT_STORAGE_ERROR,3,selected_,[](int i)->String{return i==0?D_RECOVERY_RETRY:i==1?D_RECOVERY_HELP:D_RECOVERY_ERASE;});
  }else if(page_==Page::Help){
    drawBackSectionHeader(D_RECOVERY_HELP);
    PalaChrome::message(gfx,u8g2,helpPage_?D_RECOVERY_USB:D_RECOVERY_UNREADABLE,helpPage_?D_RECOVERY_SAVE_FIRST:D_RECOVERY_PRESERVED);
  }else if(page_==Page::Confirm||page_==Page::Final){
    drawSectionHeader(page_==Page::Confirm?D_RECOVERY_ERASE:D_RECOVERY_LAST_CONFIRM);
    PalaChrome::message(gfx,u8g2,D_RECOVERY_DELETE_FILES,page_==Page::Confirm?D_RECOVERY_KEEP_SETTINGS:D_RECOVERY_HOLD_ERASE);
    PalaChrome::splitAction(gfx,u8g2,5,D_RECOVERY_CANCEL,selected_==0);
    PalaChrome::splitAction(gfx,u8g2,129,page_==Page::Confirm?D_RECOVERY_CONTINUE:D_RECOVERY_ERASE_SHORT,selected_==1);
  }else{
    drawSectionHeader(D_BOOT_STORAGE_ERROR);
    PalaChrome::message(gfx,u8g2,page_==Page::RetryFailure?D_RECOVERY_UNREADABLE:D_RECOVERY_FAILED,page_==Page::RetryFailure?D_RECOVERY_PRESERVED:D_RECOVERY_RETRY);
    PalaChrome::card(gfx,u8g2,89,D_ABOUT_BACK,true);
  }
  Font::useBody();display.update();
}
void StorageRecoveryScreen::onButton(const ButtonEvent& event){
  if(!event.any()||restarting_)return;
  if(Lock::isLocked()){
    if(Lock::isUnlockGesture(event)){Lock::disengage();menu();draw();}return;
  }
  if(event.kind==ButtonEvent::Triple){menu();draw();return;}
  if(event.kind==ButtonEvent::Short){
    if(page_==Page::Menu)selected_=(selected_+1)%3;
    else if(page_==Page::Help)helpPage_=(helpPage_+1)%2;
    else if(page_==Page::Confirm||page_==Page::Final)selected_=1-selected_;
    else return;
    draw();return;
  }
  if(event.kind!=ButtonEvent::Long&&event.kind!=ButtonEvent::Double)return;
  if(page_==Page::Menu){
    if(selected_==0){working(D_RECOVERY_CHECKING);if(actions_.retry()){restart();return;}page_=Page::RetryFailure;resetInputFrontend();}
    else if(selected_==1){page_=Page::Help;helpPage_=0;resetInputFrontend();}
    else{page_=Page::Confirm;selected_=0;resetInputFrontend();}
  }else if(page_==Page::Help)menu(1);
  else if(page_==Page::RetryFailure||page_==Page::EraseFailure)menu();
  else if(selected_==0)menu(2);
  else if(page_==Page::Confirm){page_=Page::Final;selected_=0;resetInputFrontend();}
  else if(event.kind==ButtonEvent::Long){
    working(D_RECOVERY_WORKING);if(actions_.eraseConfirmed()){restart();return;}
    page_=Page::EraseFailure;resetInputFrontend();
  }else return; // Double can never erase, even when the final action has focus.
  forceNextMenuFrameFull();draw();
}
