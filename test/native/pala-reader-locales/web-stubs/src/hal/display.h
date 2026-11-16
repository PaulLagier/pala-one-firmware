#pragma once
#include "Adafruit_GFX.h"
#include "U8g2_for_Adafruit_GFX.h"
inline Adafruit_GFX gfx;inline U8G2_FOR_ADAFRUIT_GFX u8g2;
inline bool nativeFlipped=false;inline bool displayFlipped(){return nativeFlipped;}inline void setDisplayFlipped(bool v){nativeFlipped=v;}
