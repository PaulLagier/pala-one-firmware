#pragma once
#include "src/ui/screen.h"

class StorageRecoveryScreen:public Screen {
public:
  struct Actions {bool(*retry)();bool(*eraseConfirmed)();void(*restart)();};
  StorageRecoveryScreen();
  explicit StorageRecoveryScreen(Actions actions):actions_(actions){}
  void onEnter() override;
  void draw() override;
  void onButton(const ButtonEvent& event) override;
  bool allowSleep() const override{return false;}
private:
  enum class Page {Menu,Help,Confirm,Final,RetryFailure,EraseFailure};
  Actions actions_;Page page_=Page::Menu;int selected_=0,helpPage_=0;
  bool restarting_=false;
  void menu(int selected=0);
  void working(const char* text);
  void restart();
};
extern StorageRecoveryScreen g_storageRecoveryScreen;
