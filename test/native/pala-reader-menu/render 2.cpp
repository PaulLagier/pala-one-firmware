#include "src/config.h"
#include "src/ui/reader_menu.h"
#include "src/lang/locale_core.h"
#include "runtime.h"
#include <string>
#include <zlib.h>
#include "src/ui/reader_menu.cpp"
bool inflatePreview(const uint8_t*in,size_t inputSize,uint8_t*out,size_t outputSize){uLongf bytes=outputSize;return uncompress(out,&bytes,in,inputSize)==Z_OK&&bytes==outputSize;}
int main(int argc,char**argv){if(argc!=3)return 1;if(!Language::select(argv[2],inflatePreview))return 2;const std::string path=argv[1];u8g2.begin(gfx);u8g2.setFontMode(1);
 auto save=[&](const char*name){for(int y=25;y<28;++y)for(int x=0;x<250;++x)assert(!gfx.pixels[y*250+x]);for(int y=0;y<25;++y)for(int x=195;x<250;++x)assert(!gfx.pixels[y*250+x]);gfx.save((path+"/"+name+".pbm").c_str());};auto next=[](int n){for(int i=0;i<n;++i)ReaderMenu::onButton({ButtonEvent::Short});};auto activate=[](){ReaderMenu::onButton({ButtonEvent::Long});};
 ReaderMenu::open();save("menu-1");next(3);save("menu-2");next(3);save("menu-3");
 ReaderMenu::open();next(1);activate();save("font-1");next(3);save("font-2");next(1);save("font-3");
 ReaderMenu::open();next(2);activate();save("status");
 g_bookview.book.bytes=2097152;g_bookview.pages.firstPage=2097150;g_bookview.pages.offsets[1]=2097151;g_bookview.cursor.pageIndex=2097151;g_bookview.pages.eofReached=true;ReaderMenu::draw();save("status-max");
 ReaderMenu::open();next(6);activate();save("overview-max");
 g_bookview.book.name="/books/Der kleine Prinz.pbt";g_bookview.book.bytes=10000;g_bookview.pages.firstPage=83;g_bookview.cursor.pageIndex=83;g_bookview.pages.offsets[0]=4200;g_bookview.pages.eofReached=false;ReaderMenu::draw();save("overview");
 g_bookview.book.name="/books/Коли довгі історії знаходять свій дім у нашій бібліотеці.pbt";ReaderMenu::draw();save("overview-long");
 g_bookview.book.name="/books/";for(int i=0;i<60;++i)g_bookview.book.name+="Щ";g_bookview.book.name+=".pbt";ReaderMenu::draw();save("overview-long-word");
 next(1);save("overview-menu");
 g_bookview.book.name="/books/";for(int i=0;i<300;++i)g_bookview.book.name+="Щ";g_bookview.book.name+=".pbt";ReaderMenu::draw();save("overview-very-long-word");
 for(const char* title:{"A quiet place to read","Дуже довгі історії знаходять свій дім у нашій бібліотеці",g_bookview.book.name.c_str()}){prepareMenuFrame();PalaChrome::titleBlock(u8g2,title);for(int y=0;y<122;++y)for(int x=0;x<250;++x)if(gfx.pixels[y*250+x])assert(x>=15&&x<=235&&y>=28&&y<=68);}
 bool fits=true;auto check=[&](const String&text,int width){if(u8g2.getUTF8Width(text.c_str())>width){fprintf(stderr,"Menu text too wide: %s\n",text.c_str());fits=false;}};
 const char*rows[]={D_READER_MENU_BOOKMARK,D_READER_MENU_FONT,D_READER_MENU_STATUS,D_READER_MENU_CLOSE,D_READER_MENU_LOCK,D_READER_MENU_CONTINUE,D_READER_MENU_FULL,D_READER_MENU_MINIMAL,D_READER_MENU_HIDDEN,D_ABOUT_BACK};
 Font::useMenuBold();for(const char*row:rows)check(row,216);
 check(String(D_READER_MENU_FAMILY)+": Pala Dys",216);check(String(D_READER_MENU_SIZE)+": 14",216);check(String(D_READER_MENU_GAP)+": 4",216);check(String(D_READER_MENU_BIONIC)+": "+D_READER_MENU_ON,216);check(String(D_READER_MENU_BIONIC)+": "+D_READER_MENU_OFF,216);
 Font::useMenu();char text[96];snprintf(text,sizeof(text),D_READER_MENU_PAGE_TOTAL,2097152,2097152,100u);check(text,236);snprintf(text,sizeof(text),D_READER_MENU_PAGE,2097152,100u);check(text,236);
 u8g2.setFont(pala_font_sans_b12);check(String(D_READER_MENU_TITLE)+"",183);check(String(D_READER_MENU_FONT)+"",183);check(D_READER_MENU_STATUS,183);
 return fits?0:2;
}
