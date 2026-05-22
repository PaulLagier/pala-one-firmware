#include "src/storage/wifi_creds.h"

#include "src/state.h"  // prefs

namespace WifiCreds {

static constexpr const char* kKeySsid = "wifi_ssid";
static constexpr const char* kKeyPass = "wifi_pass";

bool has() {
  return prefs.getString(kKeySsid, "").length() > 0;
}

String ssid() { return prefs.getString(kKeySsid, ""); }
String pass() { return prefs.getString(kKeyPass, ""); }

void save(const String& ssid, const String& pass) {
  prefs.putString(kKeySsid, ssid);
  prefs.putString(kKeyPass, pass);
}

void clear() {
  prefs.remove(kKeySsid);
  prefs.remove(kKeyPass);
}

}  // namespace WifiCreds
