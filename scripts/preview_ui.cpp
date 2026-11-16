#include "src/ui/pala_chrome.h"
#include <cassert>
int main(int argc,char**argv) {
  if(argc!=2)return 1;
  Adafruit_GFX gfx;U8G2_FOR_ADAFRUIT_GFX font;font.begin(gfx);font.setFontMode(1);
  PalaChrome::header(font,"Pala One");
  PalaChrome::card(gfx,font,29,"Der kleine Prinz",true);
  PalaChrome::card(gfx,font,57,"Die Verwandlung",false);
  PalaChrome::card(gfx,font,85,"Lesezeichen",false);
  // Exercise truncation at UTF-8 boundaries with the real font decoder.
  char word[]="caf\xc3\xa9";PalaChrome::removeLastChar(word);assert(strcmp(word,"caf")==0);
  gfx.save(argv[1]);
}
