#pragma once
#include "src/config.h"
#include <cstring>

// Report only languages selectable by this build. The runtime-catalog
// contribution replaces this compile-time adapter without changing the RPC.
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
