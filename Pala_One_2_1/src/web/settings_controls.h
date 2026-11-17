#pragma once
#include "src/pure/arduino_compat.h"
#include <initializer_list>
struct SettingsChoice {
  const char *value;
  const char *label;
};
// Fixed ASCII field names/values and trusted catalog labels, as in the existing
// settings forms. An explicit fallback label preserves valid custom app values.
void appendSettingsSelect(String &out, const char *name, const char *label, const char *hint,
                          std::initializer_list<SettingsChoice> choices, const String &current,
                          const String &currentLabel = String());
