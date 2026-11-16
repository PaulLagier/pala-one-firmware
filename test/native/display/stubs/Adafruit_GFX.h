#pragma once
#include <cstdint>
class Adafruit_GFX {public:Adafruit_GFX(int,int){}virtual~Adafruit_GFX()=default;virtual void drawPixel(int16_t,int16_t,uint16_t)=0;};
