#pragma once
#include <Arduino.h>
#include "src/config.h"
#include "src/lang/lang.h"
#include "src/pure/library_nav.h"
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "src/ui/pala_chrome.h"
#include <cassert>
struct ButtonEvent{enum Kind{None,Short,Double,Triple,Quad,Long,VeryLong,ClickHold};Kind kind=None;bool any()const{return kind!=None;}};
class Screen{public:virtual ~Screen()=default;virtual void onEnter(){};virtual void onButton(const ButtonEvent&)=0;virtual void draw()=0;Screen* nextScreen=nullptr;};
inline Screen* g_currentScreen=nullptr;
struct TargetScreen:Screen{void draw()override{}void onButton(const ButtonEvent&)override{}};
inline TargetScreen g_aboutScreen,g_appsScreen,g_updateScreen,g_bmBookSelectScreen,g_listScreen,g_readerScreen,g_statsScreen,g_settingsScreen,g_uploadScreen;
inline Catalog g_library;
struct List{int selectedIndex=0;};inline List g_list;
inline bool listVisible=false,allowOpen=true,cancelOpen=false;
inline int opens=0,lastBook=-1,resets=0,bookmarkResets=0,fullRequests=0,frames=0,errors=0;
inline Adafruit_GFX gfx;inline U8G2_FOR_ADAFRUIT_GFX u8g2;
struct Display{void update(){++frames;}};inline Display display;
inline void forceNextMenuFrameFull(){++fullRequests;}
inline void prepareMenuFrame(){memset(gfx.pixels,0,sizeof(gfx.pixels));u8g2.setFontMode(1);u8g2.setForegroundColor(1);u8g2.setBackgroundColor(0);}
namespace Font{inline void useBody(){u8g2.setFont(pala_font_sans_r10);u8g2.setFontMode(1);}}
namespace HeaderTitle{inline const char* current(){return "Library";}}
inline void drawBatteryTopRight(){}
inline void resetBookView(){++resets;}
inline void resetBookmarkSession(){++bookmarkResets;}
inline bool listHasVisibleItems(){return listVisible;}
inline bool openBookByIndex(int index){assert(index>=0&&index<g_library.bookCount);++opens;lastBook=index;return allowOpen;}
inline bool readerIndexWasCancelled(){return cancelOpen;}
inline void drawCenter(const char*,const char*){++errors;}
struct Console{template<class T>void println(const T&){} };inline Console Serial;
