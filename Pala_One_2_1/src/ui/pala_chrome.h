#pragma once
#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <string.h>
#include "src/ui/language_fonts.h"
#include "src/ui/menu_geometry.h"

// Shared by the firmware and the native, pixel-exact preview renderer.
// Menu typography stays legible regardless of the chosen book font.
namespace PalaChrome {
constexpr int cardHeight = 26;
constexpr int cardStep = PalaMenuGeometry::cardStep;
constexpr int firstCardTop = PalaMenuGeometry::firstCardTop;
constexpr int visibleCards = PalaMenuGeometry::visibleCards;
constexpr int menuCardHeight = PalaMenuGeometry::cardHeight;
inline int firstVisible(int selected,int count){return PalaMenuGeometry::firstVisible(selected,count);}

inline void removeLastChar(char* text) {
  size_t n = strlen(text);
  if (!n) return;
  --n;
  while (n && (static_cast<unsigned char>(text[n]) & 0xC0) == 0x80) --n;
  text[n] = 0;
}

inline void fitted(U8G2_FOR_ADAFRUIT_GFX& font, const char* text,
                   int x, int baseline, int width) {
  char buf[256];
  size_t n = strlen(text);
  if (n >= sizeof(buf)) {
    n = sizeof(buf) - 1;
    while (n && (static_cast<unsigned char>(text[n]) & 0xC0) == 0x80) --n;
  }
  memcpy(buf, text, n); buf[n] = 0;
  bool shortened = n < strlen(text);
  if (font.getUTF8Width(buf) > width) shortened = true;
  if (shortened) {
    const int suffixWidth = font.getUTF8Width("...");
    while (*buf && font.getUTF8Width(buf) + suffixWidth > width) removeLastChar(buf);
    font.setCursor(x, baseline); font.print(buf); font.print("...");
  } else {
    font.setCursor(x, baseline); font.print(buf);
  }
}

inline int centered(U8G2_FOR_ADAFRUIT_GFX& font,const char* text,int baseline,int width=216,int center=125){
  char buf[256];size_t n=strlen(text);bool cut=n>=sizeof(buf)-3;
  if(cut){n=sizeof(buf)-4;while(n&&(static_cast<unsigned char>(text[n])&0xC0)==0x80)--n;}
  memcpy(buf,text,n);buf[n]=0;
  if(font.getUTF8Width(buf)>width)cut=true;
  if(cut){const int suffix=font.getUTF8Width("...");while(*buf&&font.getUTF8Width(buf)+suffix>width)PalaChrome::removeLastChar(buf);strcat(buf,"...");}
  const int w=font.getUTF8Width(buf);font.setCursor(center-w/2,baseline);font.print(buf);return w;
}
inline void header(U8G2_FOR_ADAFRUIT_GFX& font,const char* title,int baseline=18){
  font.setForegroundColor(1);font.setBackgroundColor(0);font.setFont(pala_font_sans_b12);
  if(font.getUTF8Width(title)>130)font.setFont(pala_font_sans_b10);
  if(font.getUTF8Width(title)>130)font.setFont(pala_font_sans_b08);
  font.setFontMode(1);centered(font,title,baseline,130);
}
inline void surface(Adafruit_GFX& gfx,int top,int height,bool selected){
  if(selected)gfx.fillRoundRect(5,top,240,height,12,1);else gfx.drawRoundRect(5,top,240,height,12,1);
}
// Compact return control for read-only detail pages.
inline void backHeader(Adafruit_GFX& gfx, U8G2_FOR_ADAFRUIT_GFX& font,
                       const char* title, bool focused = true) {
  if (focused) gfx.fillCircle(15,14,10,1);
  else gfx.drawCircle(15,14,10,1);
  for (int dy=-4;dy<=4;++dy) gfx.drawFastHLine(12+(dy<0?-dy:dy),14+dy,2,focused?0:1);
  font.setFont(pala_font_sans_b12);font.setFontMode(1);
  font.setForegroundColor(1);font.setBackgroundColor(0);
  header(font,title);
}

inline void labelCard(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* title,bool selected,int height,int indent=0){
  if(indent<0)indent=0;if(indent>40)indent=40;surface(gfx,top,height,selected);
  font.setFont(selected?pala_font_sans_b10:pala_font_sans_r10);
  if(font.getUTF8Width(title)>216-indent)font.setFont(selected?pala_font_sans_b08:pala_font_sans_r08);
  font.setFontMode(1);font.setForegroundColor(selected?0:1);font.setBackgroundColor(selected?1:0);
  centered(font,title,top+(height==menuCardHeight?20:18),216-indent,125+indent/2);
  font.setForegroundColor(1);font.setBackgroundColor(0);
}
inline void card(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* title,bool selected,int indent=0){labelCard(gfx,font,top,title,selected,cardHeight,indent);}
inline void menuCard(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* title,bool selected,int indent=0){labelCard(gfx,font,top,title,selected,menuCardHeight,indent);}
template<class Label> inline void menuList(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,const char* title,int count,int selected,Label label){
  header(font,title);const int first=firstVisible(selected,count);
  for(int row=0;row<visibleCards&&first+row<count;++row){const auto text=label(first+row);menuCard(gfx,font,firstCardTop+row*cardStep,text.c_str(),first+row==selected);}
}
inline void smallLine(U8G2_FOR_ADAFRUIT_GFX& font,const char* text,int baseline,bool strong=false) {
  font.setFont(strong?pala_font_sans_b08:pala_font_sans_r08);font.setFontMode(1);
  centered(font,text,baseline,216);
}
inline void pairingCode(U8G2_FOR_ADAFRUIT_GFX& font,const char* code) {
  font.setFont(pala_font_sans_b14);font.setFontMode(1);
  font.setCursor((250-font.getUTF8Width(code))/2,73);font.print(code);
}
// Shared geometry for the small reader: content ends at84, action starts at89.
// Three spacious rows: label and value have separate centered columns.
inline void settingCard(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* label,const char* value,bool selected){
  menuCard(gfx,font,top,"",selected);font.setForegroundColor(selected?0:1);font.setBackgroundColor(selected?1:0);
  font.setFont(pala_font_sans_r08);font.setFontMode(1);centered(font,label,top+19,132,78);
  font.setFont(pala_font_sans_b10);if(font.getUTF8Width(value)>82)font.setFont(pala_font_sans_b08);
  font.setFontMode(1);centered(font,value,top+20,82,194);font.setForegroundColor(1);font.setBackgroundColor(0);
}
// Shared title treatment for book detail/hero content. Wrap at a word when
// possible; truncate only the second line, on a complete UTF-8 boundary.
inline void titleBlock(U8G2_FOR_ADAFRUIT_GFX& font,const char* text,int firstBaseline=43,int secondBaseline=63){
  font.setFont(pala_font_sans_b12);font.setFontMode(1);font.setForegroundColor(1);font.setBackgroundColor(0);
  if(font.getUTF8Width(text)<=216){centered(font,text,(firstBaseline+secondBaseline)/2);return;}
  font.setFont(pala_font_sans_b10);
  if(font.getUTF8Width(text)<=216){centered(font,text,(firstBaseline+secondBaseline)/2);return;}
  char first[256];size_t n=strlen(text);if(n>=sizeof(first))n=sizeof(first)-1;
  while(n&&(static_cast<unsigned char>(text[n])&0xC0)==0x80)--n;
  memcpy(first,text,n);first[n]=0;
  while(*first&&font.getUTF8Width(first)>216)removeLastChar(first);
  n=strlen(first);size_t word=n;while(word&&first[word-1]!=' ')--word;
  if(word>n/2){n=word-1;first[n]=0;}
  size_t next=n;while(text[next]==' ')++next;
  centered(font,first,firstBaseline);centered(font,text+next,secondBaseline);
}
inline void menuChoice(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* label,bool selected,bool checked){
  menuCard(gfx,font,top,"",selected);font.setForegroundColor(selected?0:1);font.setBackgroundColor(selected?1:0);
  font.setFont(selected?pala_font_sans_b10:pala_font_sans_r10);if(font.getUTF8Width(label)>190)font.setFont(selected?pala_font_sans_b08:pala_font_sans_r08);
  font.setFontMode(1);centered(font,label,top+20,190);gfx.drawCircle(230,top+15,5,selected?0:1);if(checked)gfx.fillCircle(230,top+15,2,selected?0:1);
  font.setForegroundColor(1);font.setBackgroundColor(0);
}

inline void panel(Adafruit_GFX& gfx,int top=32,int height=52) {
  gfx.drawRoundRect(5,top,240,height,12,1);
}
// Focus navigation is identical on every menu. No decorative page dots.
inline void infoLine(U8G2_FOR_ADAFRUIT_GFX& font,const char* text,int baseline,bool strong=false) {
  font.setFont(strong?pala_font_sans_b10:pala_font_sans_r10);font.setFontMode(1);
  if(font.getUTF8Width(text)>216)font.setFont(strong?pala_font_sans_b08:pala_font_sans_r08);
  font.setFontMode(1);
  centered(font,text,baseline,216);
}
inline void message(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,const char* first,const char* second=nullptr) {
  panel(gfx);
  infoLine(font,first,second&&*second?49:63,true);
  if(second&&*second)infoLine(font,second,71);
}
inline void metric(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* label,const char* value) {
  panel(gfx,top,24);
  smallLine(font,label,top+10);
  font.setFont(pala_font_sans_b08);font.setFontMode(1);
  font.setCursor(233-font.getUTF8Width(value),top+21);font.print(value);
}
inline void choice(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* label,bool selected,bool checked) {
  card(gfx,font,top,"",selected);
  const uint16_t ink=selected?0:1;
  font.setForegroundColor(ink);font.setBackgroundColor(selected?1:0);
  font.setFont(selected?pala_font_sans_b10:pala_font_sans_r10);font.setFontMode(1);
  centered(font,label,top+18,190);
  gfx.drawCircle(230,top+13,5,ink);if(checked)gfx.fillCircle(230,top+13,2,ink);
  font.setForegroundColor(1);font.setBackgroundColor(0);
}
inline void taskLabelFont(U8G2_FOR_ADAFRUIT_GFX& font) {
  font.setFont(pala_font_sans_b08);font.setFontMode(1);
}
inline void taskCard(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int top,const char* first,const char* second,bool selected,bool done){
  surface(gfx,top,menuCardHeight,selected);const uint16_t ink=selected?0:1;
  gfx.drawCircle(19,top+15,4,ink);if(done)gfx.fillCircle(19,top+15,2,ink);
  font.setFont(selected?pala_font_sans_b08:pala_font_sans_r08);font.setFontMode(1);font.setForegroundColor(ink);font.setBackgroundColor(selected?1:0);
  auto line=[&](const char* text,int baseline){const int width=centered(font,text,baseline,200,134);if(done)gfx.drawFastHLine(134-width/2,baseline-3,width,ink);};
  line(first,top+(second&&*second?11:20));if(second&&*second)line(second,top+25);
  font.setForegroundColor(1);font.setBackgroundColor(0);
}
inline void splitAction(Adafruit_GFX& gfx,U8G2_FOR_ADAFRUIT_GFX& font,int x,const char* text,bool selected) {
  if(selected)gfx.fillRoundRect(x,89,116,26,12,1);
  else gfx.drawRoundRect(x,89,116,26,12,1);
  font.setForegroundColor(selected?0:1);font.setBackgroundColor(selected?1:0);
  font.setFont(pala_font_sans_b10);font.setFontMode(1);
  if(font.getUTF8Width(text)>92)font.setFont(pala_font_sans_b08);font.setFontMode(1);
  centered(font,text,107,92,x+58);
  font.setForegroundColor(1);font.setBackgroundColor(0);
}
inline void progress(Adafruit_GFX& gfx,int percent) {
  percent=percent<0?0:percent>100?100:percent;
  gfx.drawRoundRect(17,73,216,7,3,1);
  if(percent)gfx.fillRoundRect(19,75,(212*percent)/100,3,1,1);
}

} // namespace PalaChrome
