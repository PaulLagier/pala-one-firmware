#pragma once
inline unsigned readerErrors=0;
inline void drawCenter(const char*,const char*){++readerErrors;}

#include "src/hal/display.h"
#include "src/ui/pala_chrome.h"
inline void prepareMenuFrame(){beginPageCanvas();}
inline int drawSectionHeader(const char* text){PalaChrome::header(u8g2,text);return 42;}

#include <functional>
inline void drawPalaList(const char* title,int count,int selected,const std::function<String(int)>&label){PalaChrome::menuList(gfx,u8g2,title,count,selected,label);}

inline void drawBatteryBottomLeft() {}
