#pragma once
#include "Arduino.h"
#include "src/ui/menu_geometry.h"
#include <vector>
#include <string>
#include <functional>
#include <cassert>
struct ButtonEvent{enum Kind{None,Short,Double,Triple,Quad,Long,VeryLong,ClickHold};Kind kind=None;bool any()const{return kind!=None;}};
class Screen{public:virtual~Screen()=default;virtual void onEnter(){};virtual void onButton(const ButtonEvent&)=0;virtual void draw()=0;virtual void onSleep(){};virtual void onIdleTick(){};Screen*nextScreen=nullptr;};
inline Screen* currentScreen=nullptr;
inline std::vector<std::string> labels;inline std::string focus;
inline int frames=0,saves=0,persists=0,bookmarks=0,renders=0,prefetches=0,dirtyMarks=0,fullMenus=0,locks=0,sleeps=0,arms=0;
inline uint32_t savedOffset=0;inline bool dirty=false,locked=false;inline std::string toast,armedPath;
struct MockBook {bool opened=true;String name="/books/Pala.pbt";size_t bytes=10000;bool isOpen()const{return opened;}const String&path()const{return name;}size_t size()const{return bytes;}};
struct MockPages {int firstPage=0,count=2;bool eofReached=false;uint32_t offsets[2]={0,1000};bool contains(int n)const{return n>=firstPage&&n<firstPage+count;}uint32_t offsetAt(int n)const{assert(contains(n));return offsets[n-firstPage];}int endPage()const{return firstPage+count;}};
struct MockCursor{int pageIndex=0,pageTurnsSinceFull=0;};struct BookView{MockBook book;MockPages pages;MockCursor cursor;};inline BookView g_bookview;
#ifdef PALA_MENU_RENDER
#include <Adafruit_GFX.h>
#include "src/ui/language_fonts.h"
#include <U8g2_for_Adafruit_GFX.h>
inline U8G2_FOR_ADAFRUIT_GFX u8g2;inline Adafruit_GFX gfx;
#else
struct MockFont{};inline MockFont u8g2;struct Graphics{};inline Graphics gfx;
namespace PalaChrome{inline void fitted(MockFont&,const char*text,int,int,int){labels.emplace_back(text);}inline void card(Graphics&,MockFont&,int,const char*text,bool selected){labels.emplace_back(text);if(selected)focus=text;}}
#ifndef PALA_MENU_RENDER

namespace PalaChrome{
inline void menuCard(Graphics&g,MockFont&f,int y,const char*t,bool selected){card(g,f,y,t,selected);}
inline void settingCard(Graphics&,MockFont&,int,const char*label,const char*value,bool selected){labels.emplace_back(label);labels.emplace_back(value);if(selected)focus=std::string(label)+": "+value;}
inline constexpr int visibleCards=PalaMenuGeometry::visibleCards,firstCardTop=PalaMenuGeometry::firstCardTop,cardStep=PalaMenuGeometry::cardStep;
inline int firstVisible(int selected,int count){return PalaMenuGeometry::firstVisible(selected,count);}
inline void splitAction(Graphics&,MockFont&,int,const char*t,bool selected){labels.emplace_back(t);if(selected)focus=t;}
inline void panel(Graphics&,int=32,int=52){}
inline void pager(Graphics&,int,int){}
inline void smallLine(MockFont&,const char*t,int,bool=false){labels.emplace_back(t);}
inline void titleBlock(MockFont&,const char*t,int=45,int=63){labels.emplace_back(t);}
inline void pairingCode(MockFont&,const char*t){labels.emplace_back(t);}
inline void message(Graphics&,MockFont&,const char*a,const char*b=nullptr){labels.emplace_back(a);if(b)labels.emplace_back(b);}
inline void metric(Graphics&,MockFont&,int,const char*a,const char*b){labels.emplace_back(a);labels.emplace_back(b);}
}
#endif

#endif
struct Display{void update(){++frames;}};inline Display display;
inline void forceNextMenuFrameFull(){++fullMenus;}
#ifdef PALA_MENU_RENDER
inline void prepareMenuFrame(){memset(gfx.pixels,0,sizeof(gfx.pixels));u8g2.setFontMode(1);u8g2.setForegroundColor(1);u8g2.setBackgroundColor(0);}
inline int drawSectionHeader(const char*text){PalaChrome::header(u8g2,text);return 42;}
inline void drawPalaList(const char*title,int count,int selected,const std::function<String(int)>&label){PalaChrome::menuList(gfx,u8g2,title,count,selected,label);}
#else
inline void prepareMenuFrame(){labels.clear();focus.clear();}
inline int drawSectionHeader(const char*text){labels.emplace_back(text);return 42;}
inline void drawPalaList(const char*title,int count,int selected,const std::function<String(int)>&label){assert(selected>=0&&selected<count);labels.emplace_back(title);for(int n=0;n<count;++n)labels.emplace_back(label(n).c_str());focus=label(selected).c_str();}
#endif
inline void markPagesDirtyForLayoutChange(){dirty=true;++dirtyMarks;}
namespace Font{
enum class Family:uint8_t{Helvetica,OpenDyslexic};inline Family family=Family::Helvetica;inline int size=10,gap=0;inline bool bionic=false;
inline Family currentFamily(){return family;}inline int currentBodySize(){return size;}inline int currentLineGap(){return gap;}inline bool bionicEnabled(){return bionic;}
inline void setFamily(Family v){family=v;}inline void setBodySize(int v){size=v;}inline void setLineGap(int v){gap=v;}inline void setBionic(bool v){bionic=v;}
#ifdef PALA_MENU_RENDER
inline void useMenu(){u8g2.setFont(pala_font_sans_r10);u8g2.setFontMode(1);}
inline void useMenuBold(){u8g2.setFont(pala_font_sans_b10);u8g2.setFontMode(1);}
inline void useBody(){useMenu();}
inline void useToast(){u8g2.setFont(pala_font_sans_r08);u8g2.setFontMode(1);}
#else
inline void useMenu(){}inline void useMenuBold(){}inline void useBody(){}inline void useToast(){}
#endif
}
namespace Statusbar{enum Mode{Full,Minimal,Hidden};inline Mode value=Full;inline Mode mode(){return value;}inline void cycleMode(){value=Mode((value+1)%3);markPagesDirtyForLayoutChange();}}
inline void saveProgress(){assert(g_bookview.pages.contains(g_bookview.cursor.pageIndex));++saves;savedOffset=g_bookview.pages.offsetAt(g_bookview.cursor.pageIndex);}
inline void saveProgressThrottled(){saveProgress();}inline void persistReaderState(){++persists;saveProgress();}
inline const char*addBookmarkForCurrentBook(){++bookmarks;return "Saved";}
inline void renderCurrentPage(){++renders;}inline void prefetchNextReaderPage(){++prefetches;}
inline bool advancePage(){++g_bookview.cursor.pageIndex;return true;}inline bool retreatPage(){if(g_bookview.cursor.pageIndex<=0)return false;--g_bookview.cursor.pageIndex;return true;}
inline void armResumeOnWake(){++arms;armedPath=g_bookview.book.path().c_str();}inline void resetBookView(){g_bookview.book.opened=false;dirty=false;}
class LibraryScreen:public Screen{public:void onButton(const ButtonEvent&)override{}void draw()override{}};inline LibraryScreen g_libraryScreen;
inline void navigateToLibraryRoot(){if(currentScreen)currentScreen->nextScreen=&g_libraryScreen;}
namespace Toast{inline void show(const String&s){toast=s.c_str();}inline bool isActive(){return !toast.empty();}}
namespace Lock{inline void engage(){locked=true;++locks;}}
struct SleepSignal{};namespace Sleep{inline void enter(){++sleeps;if(currentScreen)currentScreen->onSleep();throw SleepSignal{};}}
struct Buttons{bool isPressed()const{return false;}bool hasPendingClicks()const{return false;}};inline Buttons g_btns;
inline bool buttonQueueNonEmpty(){return false;}inline uint32_t userIdleMs(){return 100;}
#include "src/ui/reader_actions.h"
namespace Gestures{inline ButtonAction longAction=ACTION_BOOKMARK,veryLongAction=ACTION_LOCK,chordAction=ACTION_MENU;inline ButtonAction actionFor(ButtonEvent::Kind k){return k==ButtonEvent::Long?longAction:k==ButtonEvent::VeryLong?veryLongAction:k==ButtonEvent::ClickHold?chordAction:ACTION_NONE;}}

inline bool previewReady=true,previewReadFocus=true;inline int previewDraws=0;
inline bool renderBookmarkPreview(bool selected){previewReadFocus=selected;++previewDraws;return previewReady;}
