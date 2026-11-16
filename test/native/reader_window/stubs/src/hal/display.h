#pragma once
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "src/state.h"
class ReaderPreviewGraphics:public Adafruit_GFX {public:void drawRect(int x,int y,int w,int h,int color){drawFastHLine(x,y,w,color);drawFastHLine(x,y+h-1,w,color);drawFastVLine(x,y,h,color);drawFastVLine(x+w-1,y,h,color);}};
inline ReaderPreviewGraphics gfx;
inline U8G2_FOR_ADAFRUIT_GFX u8g2;
struct NullDisplay {void update(){} void fastmodeOn(){} void fastmodeOff(){}};inline NullDisplay display;
inline void setDisplayRefreshMode(bool){}
inline void beginPageCanvas(bool clear=true){if(clear)memset(gfx.pixels,0,sizeof(gfx.pixels));u8g2.setFontMode(1);u8g2.setForegroundColor(1);u8g2.setBackgroundColor(0);}
