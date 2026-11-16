#pragma once
#include <vector>
#include <string>
#include <functional>
#include "src/pure/arduino_compat.h"
inline constexpr int HIGH = 1, LOW = 0;
inline int physicalButton = HIGH;
inline int digitalRead(int) { return physicalButton; }
inline uint32_t mockMillis = 0;
inline uint32_t millis() { return mockMillis; }
inline unsigned delays = 0;
inline unsigned lastCpuMHz = 0;
inline void delay(unsigned ms) { mockMillis += ms; if (++delays > 500) throw "recovery boot loop did not finish"; }
inline bool setCpuFrequencyMhz(unsigned mhz) { lastCpuMHz = mhz; return true; }
