#ifndef PALA_STORAGE_KV_STORE_H
#define PALA_STORAGE_KV_STORE_H

#include "src/pure/arduino_compat.h"

// Abstract key/value store. Mirrors the subset of Arduino `Preferences` the
// firmware uses. The firmware passes a `PreferencesStore` wrapping the real
// global; tests pass an in-memory `MapKvStore`.
class KeyValueStore {
public:
  virtual ~KeyValueStore() = default;

  virtual int    getInt(const char* key, int defaultValue) = 0;
  virtual void   putInt(const char* key, int value) = 0;

  virtual size_t getBytes(const char* key, void* buf, size_t maxLen) = 0;
  virtual void   putBytes(const char* key, const void* buf, size_t len) = 0;

  virtual String getString(const char* key, const String& defaultValue) = 0;
  virtual void   putString(const char* key, const String& value) = 0;

  virtual bool has(const char *key) const = 0;
  virtual size_t bytesSize(const char *key) const = 0;

  virtual void   remove(const char* key) = 0;
  virtual void   clear() = 0;
};

#endif  // PALA_STORAGE_KV_STORE_H
