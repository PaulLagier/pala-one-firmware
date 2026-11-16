#include <U8g2_for_Adafruit_GFX.h>
#include <string>
Adafruit_GFX g; U8G2_FOR_ADAFRUIT_GFX f;
void text(int x,int y,const char*s,bool bold=false,bool inverse=false){f.setFont(bold?u8g2_font_helvB10_te:u8g2_font_helvR10_te);f.setForegroundColor(inverse?0:1);f.setBackgroundColor(inverse?1:0);f.setCursor(x,y);f.print(s);}
void title(int x,int y,const char*s){f.setFont(u8g2_font_helvB12_te);f.setForegroundColor(1);f.setBackgroundColor(0);f.setCursor(x,y);f.print(s);}
void pill(int x,int y,int w,const char*s,bool selected){g.fillRoundRect(x,y,w,25,12,selected?1:0);if(!selected)g.drawRoundRect(x,y,w,25,12,1);text(x+10,y+18,s,true,selected);}
void reset(){memset(g.pixels,0,sizeof(g.pixels));}
int main(int argc,char**argv){if(argc!=2)return 1;f.begin(g);f.setFontMode(1);std::string p=argv[1];
 title(7,21,"Bibliothek");text(206,20,"2/8");
 g.drawRoundRect(3,32,244,87,13,1);
 g.fillRoundRect(11,40,55,71,7,1);
 g.fillRoundRect(25,53,27,27,13,0);
 g.fillRoundRect(31,50,27,27,13,1);
 g.drawFastHLine(19,91,39,0);g.drawFastHLine(24,97,29,0);
 g.drawFastHLine(18,49,7,0);g.drawFastVLine(21,46,7,0);
 title(77,54,"Der kleine");title(77,74,"Prinz");
 pill(76,84,160,"Weiter · 42%",true);
 g.save((p+"/cover.pbm").c_str());
}
