#pragma once
#include "src/lang/runtime_strings.h"

namespace Language {
struct PackedPool {
  const uint8_t *packed;
  size_t packedSize;
  size_t rawSize;
};
struct Catalog {
  const char *code;
  const PackedPool *pools;
};
const Catalog *catalogAt(size_t index);
using Inflate = bool (*)(const uint8_t *, size_t, uint8_t *, size_t);
using Persist = bool (*)(const char *);
// Prepare and validate before persisting or replacing the active pool.
// Allocation, decode or persistence failure leaves the old selection intact.
bool select(const char *code, Inflate inflate, Persist persist = nullptr);
void loadSettings();
bool setLanguage(const char *code);
} // namespace Language
