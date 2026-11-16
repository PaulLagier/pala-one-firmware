#pragma once
#include "src/config.h"
#include <cstring>

// Keep the companion protocol aligned with the selectable firmware catalogs.
#if defined(ARDUINO) || defined(PALA_RUNTIME_LANG)
#include "src/lang/locale_core.h"
namespace CompanionLanguage {
inline const char *currentCode() { return Language::currentCode(); }
inline size_t count() { return Language::count(); }
inline const char *codeAt(size_t index) { return Language::codeAt(index); }
inline bool setLanguage(const char *code) { return Language::setLanguage(code); }
} // namespace CompanionLanguage
#else
namespace CompanionLanguage {
inline const char *currentCode() {
#if defined(LANG_ES_LA)
  return "es";
#else
  return "en";
#endif
}
inline size_t count() { return 1; }
inline const char *codeAt(size_t index) { return index == 0 ? currentCode() : nullptr; }
inline bool setLanguage(const char *code) { return code && std::strcmp(code, currentCode()) == 0; }
} // namespace CompanionLanguage

#endif
