#pragma once
namespace Statusbar {enum Mode{Full,Minimal,Hidden};inline Mode nativeMode=Full;inline Mode mode(){return nativeMode;}inline int reserveH(){return nativeMode==Full?8:nativeMode==Minimal?1:0;}inline void setMode(Mode m){nativeMode=m;}}
