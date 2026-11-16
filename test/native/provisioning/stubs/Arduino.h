#pragma once
#include "../../improv_serial/stubs/Arduino.h"
class TestSerial;
extern TestSerial Serial;
struct HWCDC {
  inline static bool plugged = true;
  static bool isPlugged() { return plugged; }
};
