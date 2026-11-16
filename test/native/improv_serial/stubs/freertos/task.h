#pragma once
#include <Arduino.h>
inline void vTaskDelay(unsigned long value) { testMillis += value; }
