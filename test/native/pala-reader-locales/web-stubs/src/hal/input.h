#pragma once
#include "src/config.h"
struct ButtonEvent {enum Kind {None,Short,Double,Triple,Quad,Long,VeryLong,ClickHold};Kind kind=None;};
inline uint32_t nativeClickGap=175;
inline uint32_t inputClickGapMs(){return nativeClickGap;}
inline void setInputClickGapMs(uint32_t value){nativeClickGap=value;}

#include "src/pure/input_timing.h"
inline InputTiming nativeTiming;
inline InputTiming inputTiming(){return nativeTiming;}
inline bool setInputTiming(const InputTiming& t){if(!validInputTiming(t))return false;nativeTiming=t;return true;}
