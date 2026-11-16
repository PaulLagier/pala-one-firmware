#pragma once
#include "src/pure/arduino_compat.h"
extern unsigned cacheDeletes;
inline void deletePageCacheForBook(const String&) { ++cacheDeletes; }
