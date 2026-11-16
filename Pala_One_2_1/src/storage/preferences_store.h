#ifndef PALA_STORAGE_PREFERENCES_STORE_H
#define PALA_STORAGE_PREFERENCES_STORE_H

#include <Preferences.h>

#include "src/storage/kv_store.h"

// `KeyValueStore` backed by an Arduino `Preferences` instance.
class PreferencesStore : public KeyValueStore {
public:
  explicit PreferencesStore(Preferences& p) : p_(p) {}

  int getInt(const char* key, int def) override { return p_.getInt(key, def); }
  void putInt(const char* key, int v) override { p_.putInt(key, v); }

  size_t getBytes(const char* key, void* buf, size_t maxLen) override {
    return p_.isKey(key) ? p_.getBytes(key, buf, maxLen) : 0;
  }
  void putBytes(const char* key, const void* buf, size_t len) override {
    p_.putBytes(key, buf, len);
  }

  String getString(const char* key, const String& def) override {
    return p_.isKey(key) ? p_.getString(key, def) : def;
  }
  void putString(const char* key, const String& v) override {
    p_.putString(key, v);
  }

  void remove(const char* key) override { if (p_.isKey(key)) p_.remove(key); }
  void clear() override { p_.clear(); }

private:
  Preferences& p_;
};

#endif  // PALA_STORAGE_PREFERENCES_STORE_H
