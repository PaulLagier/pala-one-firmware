#include "src/state.h"
#include "src/lang/locale_core.h"
#include "src/web/settings.h"
#include "src/web/chrome.h"
#include "src/web/web_assets.h"
#include "src/ui/font.h"
#include "src/ui/header_title.h"
#include "src/ui/reader.h"
#include "src/storage/book_metadata.h"
#include "src/storage/library.h"
#include <cassert>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <zlib.h>
BookView g_bookview;
Catalog g_library;
int progressWrites=0;
void saveProgress(){++progressWrites;}
int findPageForOffset(uint32_t){assert(false);return -1;}
void renderCurrentPage(){assert(false);}
void saveSavedPage(KeyValueStore&,const String&,int){assert(false);}
size_t fsTotalBytesSafe(){return 4521984;}
size_t fsUsedBytesSafe(){return 4096;}
size_t fsFreeBytesSafe(){return fsTotalBytesSafe()-fsUsedBytesSafe();}
#include "src/ui/statusbar.h"
#include "src/hal/input.h"
#include "src/hal/display.h"
#include "src/storage/wifi_creds.h"
bool contains(const String& text,const std::string& part){return std::string(text.c_str()).find(part)!=std::string::npos;}
std::string inflateAsset(){
 z_stream stream{};assert(inflateInit2(&stream,16+MAX_WBITS)==Z_OK);std::vector<uint8_t> raw(65536);
 stream.next_in=reinterpret_cast<Bytef*>(const_cast<char*>(server.body.c_str()));stream.avail_in=uInt(server.body.length());stream.next_out=raw.data();stream.avail_out=uInt(raw.size());
 assert(inflate(&stream,Z_FINISH)==Z_STREAM_END);assert(stream.total_in==server.body.length());const auto size=stream.total_out;inflateEnd(&stream);return std::string(reinterpret_cast<const char*>(raw.data()),size);
}
int main(int argc,char**argv){assert(argc==2);
  Font::loadSettings();HeaderTitle::set("Reader & friends");registerChromeRoutes();registerSettingsRoutes();
  u8g2.begin(gfx);
  server.request("/settings",HTTP_POST,{{"wifi_form","1"},{"wifi_ssid","Test <&' network"},{"wifi_password","secret123"}});
  assert(server.status==302&&WifiCreds::ssid()=="Test <&' network"&&WifiCreds::pass()=="secret123");
  server.request("/settings",HTTP_GET);assert(!contains(server.body,"secret123")&&contains(server.body,"Test &lt;&amp;&#39; network"));
  server.request("/settings",HTTP_POST,{{"wifi_form","1"},{"wifi_ssid","Test <&' network"},{"wifi_password",""}});assert(WifiCreds::pass()=="secret123");
  server.request("/settings",HTTP_POST,{{"wifi_form","1"},{"wifi_ssid","New open network"},{"wifi_password",""}});assert(server.status==302&&WifiCreds::pass()=="");
  server.request("/settings",HTTP_POST,{{"wifi_form","1"},{"wifi_ssid","Bad"},{"wifi_password","short"}});assert(server.status==400&&WifiCreds::ssid()=="New open network");
  server.request("/settings",HTTP_POST,{{"wifi_form","1"},{"wifi_clear","1"}});assert(!WifiCreds::has());
  auto times=std::map<std::string,String>{{"timing_form","1"},{"gap","200"},{"sequence","700"},{"hold","900"},{"veryLong","2200"},{"debounce","20"}};
  server.request("/settings",HTTP_POST,times);assert(server.status==302&&inputTiming().gap==200&&inputTiming().hold==900);
  times["hold"]="3000";server.request("/settings",HTTP_POST,times);assert(server.status==400&&inputTiming().hold==900);
  times["hold"]="-1";server.request("/settings",HTTP_POST,times);assert(server.status==400&&inputTiming().hold==900);
  const auto originalStamp=Font::layoutForCache();
  server.request("/settings",HTTP_POST,{{"reader_extra","1"},{"half_gaps","on"},{"flipped","on"},{"statusbar","2"}});
  assert(server.status==302&&Font::halfParagraphGaps()&&displayFlipped()&&Statusbar::mode()==Statusbar::Hidden&&Font::layoutForCache().halfGaps!=originalStamp.halfGaps);
  server.request("/settings",HTTP_POST,{{"reader_extra","1"},{"statusbar","oops"}});assert(server.status==400&&Font::halfParagraphGaps()&&displayFlipped());
  server.request("/settings",HTTP_POST,{{"reader_extra","1"},{"statusbar","0"}});assert(server.status==302&&!Font::halfParagraphGaps()&&!displayFlipped()&&Statusbar::mode()==Statusbar::Full);
  assert(progressWrites==0);std::cout<<"PASS actual settings routes: secret handling, escaping, Wi-Fi save/keep/open/forget/reject, timing atomic validation, rotation/paragraph/statusbar persistence owners\n";

  for(auto text:{std::string(29,'a')+"ü",std::string(30,'a')+"e\xcc\x81",std::string("Українська назва бібліотеки")}) {
    HeaderTitle::set(text.c_str());std::string actual=HeaderTitle::current();
    assert(actual.size()<=31);if(text[0]=='a')assert(actual== (text.size()==31?text:std::string(30,'a')));
    assert(actual.empty()||(static_cast<unsigned char>(actual.back())&0xc0)==0x80||static_cast<unsigned char>(actual.back())<0x80);
    HeaderTitle::loadSettings();assert(actual==HeaderTitle::current());
  }
  HeaderTitle::set("Reader & friends");
  const auto baseFont=Font::currentBodySize();
  const auto baseLayout=Font::layoutForCache();const auto baseMetrics=Font::bodyLayout();
  for(size_t i=0;i<Language::count();++i){
    const auto code=std::string(Language::codeAt(i));
    server.request("/settings",HTTP_POST,{{"language",code.c_str()},{"font","14"},{"hdr","Unexpected"}});
    assert(server.status==302&&server.headers.at("Location")=="/settings");
    assert(!strcmp(Language::currentCode(),code.c_str()));assert(prefs.getString("cfg_lang","")==code.c_str());
    assert(Font::currentBodySize()==baseFont&&!strcmp(HeaderTitle::current(),"Reader & friends")&&progressWrites==0);
    const auto layout=Font::layoutForCache();const auto metrics=Font::bodyLayout();
    assert(layout.bodySize==baseLayout.bodySize&&layout.lineGap==baseLayout.lineGap&&layout.family==baseLayout.family&&layout.bionic==baseLayout.bionic&&layout.statusbarReserve==baseLayout.statusbarReserve);
    assert(metrics.ascent==baseMetrics.ascent&&metrics.descent==baseMetrics.descent&&metrics.lineH==baseMetrics.lineH&&metrics.maxLines==baseMetrics.maxLines&&metrics.maxWidth==baseMetrics.maxWidth);
    server.request("/settings",HTTP_GET);
    assert(server.status==200&&server.mime=="text/html; charset=utf-8");
    assert(contains(server.body,"<html lang='"+code+"'>"));
    assert(contains(server.body,WebAssets::get(WebAssets::Id::CoreCss).url));
    assert(contains(server.body,"<label for='language'>"+std::string(D_LANGUAGE_LABEL)+"</label>"));
    assert(contains(server.body,"value='"+code+"' selected>"+Language::nameAt(i)));
    assert(contains(server.body,"id='palaLocale' hidden"));
    assert(contains(server.body,"data-no-matches='"+std::string(htmlEscape(D_WEB_JS_NO_MATCHES).c_str())+"'"));
    assert(contains(server.body,"title='"+std::string(htmlEscape(D_WEB_THEME_TITLE).c_str())+"'"));
    assert(contains(server.body,"<option value='4'"));
    assert(contains(server.body,D_WEB_SAVE_SETTINGS_BUTTON)&&contains(server.body,"Reader &amp; friends"));
    for(size_t j=0;j<Language::count();++j)assert(contains(server.body,"<option value='"+std::string(Language::codeAt(j))+"'"));
    const auto snapshot=prefs.values;
    for(const char* bad:{"unknown","de<script>","EN",""}){
      server.request("/settings",HTTP_POST,{{"language",bad},{"font","14"}});
      assert(server.status==400&&server.body==D_LANGUAGE_INVALID&&prefs.values==snapshot);
      assert(!strcmp(Language::currentCode(),code.c_str())&&Font::currentBodySize()==baseFont&&progressWrites==0);
    }
  }
  for(size_t i=0;i<static_cast<size_t>(WebAssets::Id::Count);++i){
    const auto& asset=WebAssets::get(static_cast<WebAssets::Id>(i));server.request(asset.url,HTTP_GET);
    assert(server.status==200&&server.mime==asset.mime&&server.body.length()==asset.gzipSize);
    assert(server.headers.at("Content-Encoding")=="gzip"&&server.headers.at("X-Content-Type-Options")=="nosniff");
    assert(server.headers.at("Cache-Control")=="public, max-age=31536000, immutable");
    const auto raw=inflateAsset();assert(raw.size()==asset.rawSize);
    std::ofstream file(std::string(argv[1])+"/asset-"+std::to_string(i),std::ios::binary);file.write(raw.data(),raw.size());
  }
  server.request("/style.css",HTTP_GET);assert(server.status==200&&inflateAsset().find(":root{")!=std::string::npos&&server.headers.at("Cache-Control")=="public, max-age=3600");
  std::cout<<"PASS actual web settings/chrome: "<<Language::count()<<" locales, persisted POST/redirect, selected native-name option, HTML lang/escaping, invalid rollback and unchanged reading settings/progress\n";
}
