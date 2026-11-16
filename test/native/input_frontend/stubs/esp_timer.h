#pragma once
#include "Arduino.h"
inline int64_t esp_timer_get_time() { return testClockMs * 1000ULL; }
