#ifndef PALA_STORAGE_WIFI_CREDS_H
#define PALA_STORAGE_WIFI_CREDS_H

#include <Arduino.h>

// Persisted home-network Wi-Fi credentials, configurable in two ways:
//  - The Web settings UI (web/settings.cpp)
//  - written by the Improv Serial flow (hal/wifi_provisioning.cpp)
// Read by the upload screen when deciding STA vs SoftAP. Stored alongside
// the project's other settings in the shared "ereader" NVS namespace
// (`prefs`), under per-module keys following the existing convention.
namespace WifiCreds {

bool   has();                                   // true iff a non-empty SSID is stored
String ssid();                                  // "" if not set
String pass();                                  // "" if not set
void   save(const String& ssid, const String& pass);
void   clear();

}  // namespace WifiCreds

#endif  // PALA_STORAGE_WIFI_CREDS_H
