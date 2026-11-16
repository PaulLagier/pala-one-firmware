#pragma once
#ifdef PALA_COMPANION
#include <ArduinoJson.h>
const char *companionBookmarksCommand(const String &op, JsonDocument &in, JsonDocument &out);
void companionBookmarksReset();
#endif
