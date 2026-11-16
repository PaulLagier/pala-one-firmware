#include "src/config.h"
#include "src/ui/reader_menu.h"
#include "src/lang/locale_core.h"
#include "runtime.h"
#include <string>
#include <zlib.h>
#include "src/ui/reader_menu.cpp"
bool inflatePreview(const uint8_t* in,size_t n,uint8_t* out,size_t size){uLongf count=size;return uncompress(out,&count,in,n)==Z_OK&&count==size;}
int main(int argc,char**argv){
 assert(argc==3&&Language::select(argv[2],inflatePreview));u8g2.begin(gfx);
 ReaderMenu::open();
 for(int selection=0;selection<3;++selection){
  for(int y:{28,60,92})assert(gfx.pixels[y*250+125]);
  gfx.save((std::string(argv[1])+"/reader-"+std::to_string(selection)+".pbm").c_str());
  ReaderMenu::onButton({ButtonEvent::Short});
 }
 Font::useMenuBold();for(const char* text:{D_READER_MENU_CONTINUE,D_READER_MENU_BOOKMARK,D_READER_BACK_LIBRARY})assert(u8g2.getUTF8Width(text)<=216);
 puts("PASS actual ReaderMenu three-action rendering and label widths");
}
