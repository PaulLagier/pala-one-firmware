#include "src/lang/locale_core.h"
#include <array>
#include <memory>
#include <new>
#include <string.h>
#if PALA_DIAGNOSTICS
#include <Arduino.h>
#include <esp_heap_caps.h>
#endif
namespace Language {
namespace {
using OwnedPools = std::array<std::unique_ptr<uint8_t[]>, poolCount>;
OwnedPools &ownedPools() {
  static OwnedPools pools;
  return pools;
}
Inflate currentInflater = nullptr;
bool reject(const char *reason, size_t pool, size_t bytes) {
#if PALA_DIAGNOSTICS
  Serial.printf("[locale-failure] reason=%s pool=%u bytes=%u heap=%u largest=%u\n", reason,
                unsigned(pool), unsigned(bytes), unsigned(ESP.getFreeHeap()),
                unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
#else
  (void)reason;
  (void)pool;
  (void)bytes;
#endif
  return false;
}
bool indexPool(uint8_t *pool, size_t bytes, size_t keys) {
  if (bytes < keys * 2 + 1)
    return false;
  size_t offset = keys * 2;
  for (size_t i = 0; i < keys; ++i) {
    if (offset >= bytes || !pool[offset])
      return false;
    const auto *end = static_cast<const uint8_t *>(memchr(pool + offset, 0, bytes - offset));
    if (!end)
      return false;
    pool[i * 2] = uint8_t(offset);
    pool[i * 2 + 1] = uint8_t(offset >> 8);
    offset = size_t(end - pool) + 1;
  }
  return offset == bytes;
}
} // namespace
bool select(const char *code, Inflate inflate, Persist persist) {
  if (!code)
    return false;
  const Catalog *next = nullptr;
  for (size_t i = 0; i < count(); ++i) {
    const auto *item = catalogAt(i);
    if (!strcmp(item->code, code)) {
      next = item;
      break;
    }
  }
  if (!next)
    return false;
  if (!strcmp(currentCode(), code))
    return !persist || persist(next->code);
  OwnedPools prepared;
  if (next->pools) {
    if (!inflate)
      return reject("decoder", 0, 0);
    std::array<size_t, poolCount> sizes{};
    for (size_t i = 0; i < poolCount; ++i) {
      const size_t prefix = keysInPool(i) * 2;
      if (next->pools[i].rawSize > 65535 - prefix)
        return reject("size", i, next->pools[i].rawSize);
      sizes[i] = prefix + next->pools[i].rawSize;
    }
    // Keep only interface strings during the atomic prepare phase. Browser
    // pools are all decoded and indexed here in one reusable scratch buffer;
    // successful first use caches each pool without evicting returned strings.
    size_t scratchSize = 0;
    for (size_t i = interfacePoolCount; i < poolCount; ++i)
      if (sizes[i] > scratchSize)
        scratchSize = sizes[i];
    std::unique_ptr<uint8_t[]> scratch;
    if (scratchSize) {
      scratch.reset(new (std::nothrow) uint8_t[scratchSize]);
      if (!scratch)
        return reject("scratch-allocation", interfacePoolCount, scratchSize);
    }
    for (size_t allocated = 0; allocated < interfacePoolCount; ++allocated) {
      size_t largest = interfacePoolCount;
      for (size_t i = 0; i < interfacePoolCount; ++i)
        if (!prepared[i] && (largest == interfacePoolCount || sizes[i] > sizes[largest]))
          largest = i;
      prepared[largest].reset(new (std::nothrow) uint8_t[sizes[largest]]);
      if (!prepared[largest])
        return reject("allocation", largest, sizes[largest]);
    }
    for (size_t i = 0; i < poolCount; ++i) {
      const auto &source = next->pools[i];
      const size_t keys = keysInPool(i), prefix = keys * 2;
      uint8_t *output = i < interfacePoolCount ? prepared[i].get() : scratch.get();
      if (!inflate(source.packed, source.packedSize, output + prefix, source.rawSize))
        return reject("inflate", i, sizes[i]);
      if (!indexPool(output, sizes[i], keys))
        return reject("index", i, sizes[i]);
    }
  }
  if (persist && !persist(next->code))
    return false;
  for (size_t i = 0; i < poolCount; ++i)
    activePools[i] = prepared[i] ? prepared[i].get() : next->pools ? nullptr : defaultPools[i];
  activePool = activePools[0];
  activeCode = next->code;
  ownedPools() = std::move(prepared);
  currentInflater = inflate;
  return true;
}
bool ensurePool(size_t index) {
  if (index >= poolCount)
    return false;
  if (activePools[index])
    return true;
  if (!currentInflater)
    return false;
  const Catalog *current = nullptr;
  for (size_t i = 0; i < count(); ++i)
    if (!strcmp(catalogAt(i)->code, currentCode())) {
      current = catalogAt(i);
      break;
    }
  if (!current || !current->pools)
    return false;
  const auto &source = current->pools[index];
  const size_t keys = keysInPool(index), prefix = keys * 2;
  if (source.rawSize > 65535 - prefix)
    return false;
  const size_t bytes = prefix + source.rawSize;
  std::unique_ptr<uint8_t[]> prepared(new (std::nothrow) uint8_t[bytes]);
  if (!prepared)
    return reject("cache-allocation", index, bytes);
  if (!currentInflater(source.packed, source.packedSize, prepared.get() + prefix, source.rawSize) ||
      !indexPool(prepared.get(), bytes, keys))
    return reject("cache-decode", index, bytes);
  activePools[index] = prepared.get();
  ownedPools()[index] = std::move(prepared);
  return true;
}
} // namespace Language
