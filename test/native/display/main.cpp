#include "src/hal/display.h"
#include <cstdio>
#include <initializer_list>
int main(){
 loadDisplaySettings();assert(!displayFlipped());unsigned cases=0;
 for(bool flipped:{false,true}){
  setDisplayFlipped(flipped);assert(prefs.flipped==flipped);loadDisplaySettings();assert(displayFlipped()==flipped);
  for(int y=0;y<122;++y)for(int x=0;x<250;++x){gfx.drawPixel(x,y,(x+y)&1);assert(display.lastX==(flipped?x:249-x)&&display.lastY==(flipped?y:121-y));assert(display.lastColor==(((x+y)&1)?BLACK:WHITE));++cases;}
  const auto count=display.calls;gfx.drawPixel(-1,0,0);gfx.drawPixel(0,-1,0);gfx.drawPixel(250,0,0);gfx.drawPixel(0,122,0);assert(display.calls==count);
 }
 printf("PASS %u production display-adapter mappings, both rotations, colors, bounds and preference reload\n",cases);
}
