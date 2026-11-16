#pragma once
#include "Arduino.h"
#include <functional>
#include <cstring>
#include <cassert>
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
struct ButtonEvent { enum Kind{None,Short,Double,Triple,Quad,Long,VeryLong,ClickHold};Kind kind=None;bool any()const{return kind!=None;} };
class Screen {public:virtual ~Screen()=default;virtual void onEnter(){};virtual void onButton(const ButtonEvent&)=0;virtual void draw()=0;Screen* nextScreen=nullptr;};
class StatisticsGraphics:public Adafruit_GFX {public:void drawRect(int x,int y,int w,int h,int c){drawFastHLine(x,y,w,c);drawFastHLine(x,y+h-1,w,c);drawFastVLine(x,y,h,c);drawFastVLine(x+w-1,y,h,c);}};
inline StatisticsGraphics gfx;
inline U8G2_FOR_ADAFRUIT_GFX u8g2;
struct PreviewDisplay {void update(){}};inline PreviewDisplay display;
#include "src/ui/language_fonts.h"
inline int settingWrites=0;
namespace Font {
enum class Family:uint8_t{Helvetica,OpenDyslexic};
inline Family family=Family::Helvetica;inline int size=10,gap=0;inline bool bionic=false;
inline Family currentFamily(){return family;}inline int currentBodySize(){return size;}inline int currentLineGap(){return gap;}inline bool bionicEnabled(){return bionic;}
inline void setFamily(Family v){family=v;++settingWrites;}inline void setBodySize(int v){size=v;++settingWrites;}inline void setLineGap(int v){gap=v;++settingWrites;}inline void setBionic(bool v){bionic=v;++settingWrites;}
inline void useMenu(){u8g2.setFont(pala_font_sans_r10);u8g2.setFontMode(1);}
inline void useMenuBold(){u8g2.setFont(pala_font_sans_b10);u8g2.setFontMode(1);}
inline void useToast(){u8g2.setFont(pala_font_sans_r08);u8g2.setFontMode(1);}
inline void useBody(){useMenu();}
}
namespace Sleep {
inline int timeout=120;inline bool locked=false,noImage=false;
inline int idleTimeoutSecs(){return timeout;}inline bool lockOnSleep(){return locked;}inline bool noScreensaver(){return noImage;}
inline void setIdleTimeout(int v){assert(v>=10&&v<=3600);timeout=v;++settingWrites;}
inline void setLockOnSleep(bool v){locked=v;++settingWrites;}inline void setNoScreensaver(bool v){noImage=v;++settingWrites;}
}
namespace Statusbar {enum Mode{Full,Minimal,Hidden};inline Mode current=Full;inline Mode mode(){return current;}inline void setMode(Mode v){current=v;++settingWrites;}}
inline uint32_t responseGap=175;
inline uint32_t inputClickGapMs(){return responseGap;}inline void setInputClickGapMs(uint32_t v){responseGap=v;++settingWrites;}
inline void prepareMenuFrame(){memset(gfx.pixels,0,sizeof(gfx.pixels));u8g2.setFontMode(1);u8g2.setForegroundColor(1);u8g2.setBackgroundColor(0);}
inline int drawSectionHeader(const char* title){PalaChrome::header(u8g2,title);Font::useMenu();return 42;}
inline void drawPalaList(const char* title,int count,int selected,const std::function<String(int)>& label){PalaChrome::menuList(gfx,u8g2,title,count,selected,label);}
class LibraryScreen:public Screen {public:void onButton(const ButtonEvent&)override{}void draw()override{}};inline LibraryScreen g_libraryScreen;

inline int drawBackSectionHeader(const char* title,bool focused=true){PalaChrome::backHeader(gfx,u8g2,title,focused);return 32;}

class UploadScreen:public Screen{public:void draw()override{}void onButton(const ButtonEvent&)override{}};inline UploadScreen g_uploadScreen;
#include "src/ui/reader_actions.h"
namespace Gestures {
inline ButtonAction longAction=ACTION_BOOKMARK,extraAction=ACTION_LOCK,chordAction=ACTION_MENU;
inline ButtonAction actionLong(){return longAction;}inline ButtonAction actionExtraLong(){return extraAction;}inline ButtonAction actionClickHold(){return chordAction;}
inline void setActionLong(ButtonAction v){longAction=v;++settingWrites;}inline void setActionExtraLong(ButtonAction v){extraAction=v;++settingWrites;}inline void setActionClickHold(ButtonAction v){chordAction=v;++settingWrites;}
}
