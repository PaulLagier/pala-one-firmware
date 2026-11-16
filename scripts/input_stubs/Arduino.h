#pragma once
#include <cstdint>
#include <cstddef>
#define IRAM_ATTR
#define HIGH 1
#define LOW 0
extern uint64_t testClockMs;
extern int testPin;
inline uint32_t millis() { return static_cast<uint32_t>(testClockMs); }
inline void delay(uint32_t ms) { testClockMs += ms; }
inline int digitalRead(int) { return testPin; }
inline void noInterrupts() {}
inline void interrupts() {}
