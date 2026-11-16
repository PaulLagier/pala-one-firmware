#pragma once
#include <stdint.h>
struct InputTiming {
  uint32_t gap=175, sequence=550, hold=850, veryLong=2000, debounce=14;
};
// Validate a whole candidate before persisting; no partial updates.
inline bool validInputTiming(const InputTiming& t) {
  return t.gap>=75 && t.gap<=1000 && t.sequence>=t.gap && t.sequence<=5000
    && t.hold>=300 && t.hold<=5000 && t.veryLong>t.hold && t.veryLong<=10000
    && t.debounce<=50 && t.debounce<t.hold;
}

inline bool parseTimingNumber(const char* text, uint32_t& out) {
  if(!text || !*text)return false;
  uint32_t n=0;
  for(;*text;++text){
    if(*text<'0'||*text>'9')return false;
    const uint32_t digit=uint32_t(*text-'0');
    if(n>(UINT32_MAX-digit)/10)return false;
    n=n*10+digit;
  }
  out=n;return true;
}
