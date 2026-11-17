#pragma once
#include "src/config.h"
#include "Preferences.h"
#include "memory_fs.h"
inline MemoryFS FS;
inline Preferences prefs;

#include "web_server.h"
uint8_t loadBookmarksForKey(const String &, uint32_t *, uint32_t *);
