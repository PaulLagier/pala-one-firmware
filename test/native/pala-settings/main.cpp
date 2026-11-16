#include "src/config.h"
#include "src/ui/screens/settings_screen.h"
#include "src/lang/locale_core.h"
#include <zlib.h>
#include <string>
bool inflatePreview(const uint8_t*in,size_t n,uint8_t*out,size_t cap){uLongf bytes=cap;return uncompress(out,&bytes,in,n)==Z_OK&&bytes==cap;}
#include "src/ui/screens/settings_screen.cpp"
SettingsScreen g_settingsScreen;
int main(int argc,char**argv){
 assert(argc==3&&Language::select(argv[2],inflatePreview));u8g2.begin(gfx);
 Screen* targets[]={&g_statsScreen,&g_updateScreen,&g_aboutScreen,&g_libraryScreen};
 for(auto activate:{ButtonEvent::Long,ButtonEvent::Double})for(int index=0;index<4;++index){
  SettingsScreen s;s.onEnter();for(int n=0;n<index;++n)s.onButton({ButtonEvent::Short});
  for(int y:{28,60,92})assert(gfx.pixels[y*250+125]);
  gfx.save((std::string(argv[1])+"/device-"+std::to_string(index)+".pbm").c_str());
  s.onButton({activate});assert(s.nextScreen==targets[index]);
 }
 SettingsScreen s;s.onEnter();s.onButton({ButtonEvent::Triple});assert(s.nextScreen==&g_libraryScreen&&settingWrites==0);
 puts("PASS production device hub: all routes, both activation gestures, three-row render, no setting writes");
}
