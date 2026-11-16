#pragma once
#include "src/pure/arduino_compat.h"
#include <memory>
inline uint32_t millis(){return 10000;}
inline uint32_t cpuFrequency=80, cpuSwitches=0;
inline uint32_t getCpuFrequencyMhz(){return cpuFrequency;}
inline void setCpuFrequencyMhz(uint32_t value){cpuFrequency=value;++cpuSwitches;}

inline void delay(unsigned){}
