#pragma once
#include "src/pure/arduino_compat.h"

// Quoted JavaScript string, safe inside an HTML script element.
String webJsString(const char *text);
