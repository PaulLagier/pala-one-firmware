#pragma once
#include <WiFi.h>
#include "src/state.h"
#include "src/hal/file_stream.h"
// Keep browser reading/search/downloads in logical UTF-8, including PBT1 books.
inline void streamBookText(File& file,const String& attachment=String()){
  FileReadStream stream(file);
  if(!stream.size()){server.send(500,"text/plain; charset=utf-8","Invalid book");return;}
  server.setContentLength(stream.size());
  if(attachment.length())server.sendHeader("Content-Disposition","attachment; filename=\""+attachment+"\"");
  server.send(200,"text/plain; charset=utf-8","");
  WiFiClient client=server.client();uint8_t bytes[512];
  while(stream.available() && client.connected()){
    size_t count=0;while(count<sizeof(bytes)){const int value=stream.read();if(value<0)break;bytes[count++]=uint8_t(value);}
    if(!count)break;
    size_t sent=0;while(sent<count && client.connected()){const size_t n=client.write(bytes+sent,count-sent);if(!n)return;sent+=n;}
    yield();
  }
}
