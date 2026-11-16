#include <U8g2_for_Adafruit_GFX.h>
#include "src/ui/language_fonts.h"
#include "src/ui/font_render.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main(){Adafruit_GFX gfx;U8G2_FOR_ADAFRUIT_GFX f;f.begin(gfx);f.setForegroundColor(1);f.setBackgroundColor(0);
const uint8_t* faces[]={pala_font_sans_r08,pala_font_sans_b08,pala_font_sans_r10,pala_font_sans_b10,pala_font_sans_r12,pala_font_sans_b12,pala_font_sans_r14,pala_font_sans_b14,pala_font_dys_r08,pala_font_dys_b08,pala_font_dys_r10,pala_font_dys_b10,pala_font_dys_r12,pala_font_dys_b12,pala_font_dys_r14,pala_font_dys_b14};unsigned count=0;
for(auto face:faces){Font::useTransparentFace(f,face);for(unsigned cp=32;cp<0x10000;++cp){if(!u8g2_IsGlyph(&f.u8g2,cp))continue;int a=0,d=0;Font::glyphVerticalBounds(f,cp,a,d);memset(gfx.pixels,0,sizeof(gfx.pixels));f.drawGlyph(60,60,cp);for(int y=0;y<122;++y)for(int x=25;x<110;++x)if(gfx.pixels[y*250+x])assert(y>=60-a&&y<60-d);++count;}}
printf("PASS %u actual glyph metadata/pixel bounds, all16 font faces\n",count);}
