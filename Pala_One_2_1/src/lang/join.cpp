#include "src/lang/join.h"
#include <string.h>

namespace Language {
String join(std::initializer_list<const char*> parts){
  size_t bytes=0;
  for(const auto* part:parts)if(part)bytes+=strlen(part);
  String out;out.reserve(bytes);
  for(const auto* part:parts)if(part)out+=part;
  return out;
}
}
