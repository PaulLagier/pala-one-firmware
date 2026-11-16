#pragma once
#include <cassert>
#include <cstring>
struct EInkDisplay {
  int calls=0,lastX=-1,lastY=-1,lastColor=-1;
  void drawPixel(int x,int y,int c){assert(x>=0&&x<250&&y>=0&&y<122);++calls;lastX=x;lastY=y;lastColor=c;}
  void clearMemory(){}void landscape(){}void fastmodeOff(){}void fastmodeOn(){}
};
struct Preferences {bool flipped=false;bool getBool(const char*,bool){return flipped;}void putBool(const char*,bool v){flipped=v;}};
inline EInkDisplay display;inline Preferences prefs;
