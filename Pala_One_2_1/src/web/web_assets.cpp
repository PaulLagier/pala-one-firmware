#include "src/web/web_assets.h"
#include "src/state.h"

namespace WebAssets {
void serve(Id id,bool immutable){
  const auto& item=get(id);
  server.sendHeader("Content-Encoding","gzip");
  server.sendHeader("Cache-Control",immutable?"public, max-age=31536000, immutable":"public, max-age=3600");
  server.sendHeader("X-Content-Type-Options","nosniff");
  server.send_P(200,item.mime,reinterpret_cast<const char*>(item.gzip),item.gzipSize);
}
void appendStyle(String& out,Id id){out+="<link rel='stylesheet' href='";out+=get(id).url;out+="'>";}
void appendScript(String& out,Id id){out+="<script src='";out+=get(id).url;out+="'></script>";}
void registerRoutes(){
  for(size_t i=0;i<static_cast<size_t>(Id::Count);++i){
    const auto id=static_cast<Id>(i);
    server.on(get(id).url,HTTP_GET,[id](){serve(id);});
  }
}
}
