#pragma once
#include "src/config.h"
namespace Diagnostics {
#if PALA_DIAGNOSTICS
bool poll();
bool isActive();
#else
inline bool poll() { return false; }
inline bool isActive() { return false; }
#endif
}
