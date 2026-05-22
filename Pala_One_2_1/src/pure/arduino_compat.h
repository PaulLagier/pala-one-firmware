// Compatibility shim so pure modules compile both on-device (real Arduino
// `String` and friends) and off-device (minimal `String` backed by
// std::string for host tests).
//
// All pure-module headers should `#include "arduino_compat.h"` instead of
// pulling in `<Arduino.h>` directly.
#ifndef PALA_PURE_ARDUINO_COMPAT_H
#define PALA_PURE_ARDUINO_COMPAT_H

#ifdef ARDUINO

#include <Arduino.h>

#else  // host build (off-device tests)

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>

// Minimal Arduino `String` shim. Only the methods used by the pure modules
// are implemented. Operations match the Arduino semantics (e.g. `indexOf`
// returns -1, `substring(from)` clamps, etc.).
class String {
public:
  String() = default;
  String(const char* s) : s_(s ? s : "") {}
  String(const std::string& s) : s_(s) {}
  String(const String&) = default;
  String(String&&) noexcept = default;
  String& operator=(const String&) = default;
  String& operator=(String&&) noexcept = default;

  // Numeric constructors mirror Arduino's behavior.
  String(int n)                { char b[24]; std::snprintf(b, sizeof(b), "%d", n);   s_ = b; }
  String(unsigned n)           { char b[24]; std::snprintf(b, sizeof(b), "%u", n);   s_ = b; }
  String(long n)               { char b[24]; std::snprintf(b, sizeof(b), "%ld", n);  s_ = b; }
  String(unsigned long n)      { char b[24]; std::snprintf(b, sizeof(b), "%lu", n);  s_ = b; }
  String(long long n)          { char b[32]; std::snprintf(b, sizeof(b), "%lld", n); s_ = b; }
  String(unsigned long long n) { char b[32]; std::snprintf(b, sizeof(b), "%llu", n); s_ = b; }
  String(float v, int prec = 2)  { char b[32]; std::snprintf(b, sizeof(b), "%.*f", prec, (double)v); s_ = b; }
  String(double v, int prec = 2) { char b[32]; std::snprintf(b, sizeof(b), "%.*f", prec, v);        s_ = b; }
  explicit String(char c) { s_.assign(1, c); }

  const char* c_str() const { return s_.c_str(); }
  unsigned    length() const { return (unsigned)s_.size(); }
  void        reserve(size_t n) { s_.reserve(n); }

  char operator[](unsigned i) const { return i < s_.size() ? s_[i] : 0; }
  char& operator[](unsigned i)      { return s_[i]; }

  String& operator+=(const String& o) { s_ += o.s_; return *this; }
  String& operator+=(const char* o)   { s_ += (o ? o : ""); return *this; }
  String& operator+=(char c)          { s_ += c; return *this; }
  String& operator+=(int n)           { s_ += String(n).s_; return *this; }
  String& operator+=(unsigned long n) { s_ += String(n).s_; return *this; }

  String operator+(const String& o) const { String r(*this); r += o; return r; }
  String operator+(const char* o) const   { String r(*this); r += o; return r; }
  String operator+(char c) const          { String r(*this); r += c; return r; }
  String operator+(int n) const           { String r(*this); r += n; return r; }

  bool operator==(const String& o) const { return s_ == o.s_; }
  bool operator==(const char* o) const   { return o && s_ == o; }
  bool operator!=(const String& o) const { return s_ != o.s_; }
  bool operator!=(const char* o) const   { return !o || s_ != o; }

  String substring(unsigned from) const {
    if (from >= s_.size()) return String();
    return String(s_.substr(from));
  }
  String substring(unsigned from, unsigned to) const {
    if (from >= s_.size() || to <= from) return String();
    if (to > s_.size()) to = s_.size();
    return String(s_.substr(from, to - from));
  }

  int indexOf(char c) const {
    auto p = s_.find(c); return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(char c, unsigned from) const {
    auto p = s_.find(c, from); return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(const char* needle, unsigned from = 0) const {
    auto p = s_.find(needle, from); return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(const String& needle, unsigned from = 0) const {
    return indexOf(needle.c_str(), from);
  }
  int lastIndexOf(char c) const {
    auto p = s_.rfind(c); return p == std::string::npos ? -1 : (int)p;
  }

  bool startsWith(const char* p) const {
    size_t pl = std::strlen(p);
    return s_.size() >= pl && std::memcmp(s_.data(), p, pl) == 0;
  }
  bool startsWith(const String& p) const { return startsWith(p.c_str()); }
  bool endsWith(const char* p) const {
    size_t pl = std::strlen(p);
    return s_.size() >= pl && std::memcmp(s_.data() + s_.size() - pl, p, pl) == 0;
  }
  bool endsWith(const String& p) const { return endsWith(p.c_str()); }

  void replace(char a, char b) {
    for (auto& c : s_) if (c == a) c = b;
  }
  void replace(const char* from, const char* to) {
    if (!from || !to || !*from) return;
    size_t fl = std::strlen(from);
    size_t tl = std::strlen(to);
    size_t p = 0;
    while ((p = s_.find(from, p, fl)) != std::string::npos) {
      s_.replace(p, fl, to, tl);
      p += tl;
    }
  }
  void replace(const String& from, const String& to) {
    replace(from.c_str(), to.c_str());
  }

  void trim() {
    size_t i = 0;
    while (i < s_.size() && (unsigned char)s_[i] <= ' ') ++i;
    size_t j = s_.size();
    while (j > i && (unsigned char)s_[j - 1] <= ' ') --j;
    s_ = s_.substr(i, j - i);
  }
  void remove(unsigned index) {
    if (index < s_.size()) s_.erase(index);
  }
  void remove(unsigned index, unsigned count) {
    if (index < s_.size()) s_.erase(index, count);
  }

  int toInt() const {
    if (s_.empty()) return 0;
    return std::atoi(s_.c_str());
  }

  // For std::ostream printing in test failure messages.
  friend std::ostream& operator<<(std::ostream& os, const String& s) {
    return os << s.s_;
  }

private:
  std::string s_;
};

inline String operator+(const char* a, const String& b) { return String(a) + b; }
inline bool operator==(const char* a, const String& b) { return b == a; }
inline bool operator!=(const char* a, const String& b) { return b != a; }

// Arduino min/max are macros; provide template versions in host build.
template <typename A, typename B>
inline auto _arduino_min(A a, B b) -> decltype(a < b ? a : b) { return a < b ? a : b; }
template <typename A, typename B>
inline auto _arduino_max(A a, B b) -> decltype(a > b ? a : b) { return a > b ? a : b; }
#define min(a, b) _arduino_min((a), (b))
#define max(a, b) _arduino_max((a), (b))

#endif  // ARDUINO

#endif  // PALA_PURE_ARDUINO_COMPAT_H
