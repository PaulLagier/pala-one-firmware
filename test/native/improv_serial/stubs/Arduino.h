#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
inline uint32_t testMillis = 0;
inline uint32_t millis() { return testMillis; }
inline void delay(unsigned long value) { testMillis += value; }
