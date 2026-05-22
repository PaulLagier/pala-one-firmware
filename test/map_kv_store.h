#pragma once

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "storage/kv_store.h"

// In-memory `KeyValueStore` for off-device tests. Bytes-backed for all types.
class MapKvStore : public KeyValueStore {
public:
  int getInt(const char* key, int def) override {
    auto it = data_.find(key);
    if (it == data_.end() || it->second.size() != sizeof(int)) return def;
    int v;
    std::memcpy(&v, it->second.data(), sizeof(v));
    return v;
  }
  void putInt(const char* key, int v) override {
    auto& buf = data_[key];
    buf.resize(sizeof(v));
    std::memcpy(buf.data(), &v, sizeof(v));
  }

  size_t getBytes(const char* key, void* buf, size_t maxLen) override {
    auto it = data_.find(key);
    if (it == data_.end()) return 0;
    size_t n = it->second.size();
    if (n > maxLen) n = maxLen;
    std::memcpy(buf, it->second.data(), n);
    return n;
  }
  void putBytes(const char* key, const void* buf, size_t len) override {
    auto& v = data_[key];
    v.assign((const uint8_t*)buf, (const uint8_t*)buf + len);
    putBytesCalls_++;
  }

  size_t putBytesCallCount() const { return putBytesCalls_; }

  String getString(const char* key, const String& def) override {
    auto it = strings_.find(key);
    if (it == strings_.end()) return def;
    return String(it->second.c_str());
  }
  void putString(const char* key, const String& v) override {
    strings_[key] = v.c_str();
  }

  void remove(const char* key) override {
    data_.erase(key);
    strings_.erase(key);
  }
  void clear() override {
    data_.clear();
    strings_.clear();
  }

  bool has(const char* key) const {
    return data_.count(key) > 0 || strings_.count(key) > 0;
  }
  size_t entryCount() const { return data_.size() + strings_.size(); }

private:
  std::map<std::string, std::vector<uint8_t>> data_;
  std::map<std::string, std::string> strings_;
  size_t putBytesCalls_ = 0;
};
