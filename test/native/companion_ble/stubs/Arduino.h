#pragma once
#include "../../companion_rpc/stubs/Arduino.h"
namespace TestCritical { inline int depth=0; }
using portMUX_TYPE=int;
constexpr int portMUX_INITIALIZER_UNLOCKED=0;
inline void portENTER_CRITICAL(portMUX_TYPE*){++TestCritical::depth;}
inline void portEXIT_CRITICAL(portMUX_TYPE*){--TestCritical::depth;}
