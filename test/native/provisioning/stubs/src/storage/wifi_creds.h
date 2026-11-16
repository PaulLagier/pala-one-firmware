#pragma once
#include <string>
namespace WifiCreds {
inline std::string savedSsid = "old", savedPassword = "keep";
inline unsigned saves = 0;
inline void save(const char *ssid, const char *password) {
  savedSsid = ssid;
  savedPassword = password;
  ++saves;
}
} // namespace WifiCreds
