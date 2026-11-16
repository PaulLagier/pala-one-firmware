#pragma once
#include <cstdint>
#include "src/hal/display.h"
inline bool paintIndexFeedback=false;inline int indexFeedbackSteps=0;
inline int indexCancelAfter=-1;
class ReaderIndexFeedback {
public:
 explicit ReaderIndexFeedback(bool=true) {}
 bool step(uint32_t,uint32_t){++indexFeedbackSteps;if(paintIndexFeedback)gfx.fillRect(0,0,250,122,1);if(indexCancelAfter<0)return true;if(indexCancelAfter==0)return false;--indexCancelAfter;return true;}
};
