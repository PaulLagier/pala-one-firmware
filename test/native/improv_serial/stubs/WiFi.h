#pragma once
#include <string>
#define WL_CONNECTED 3
#define WIFI_AUTH_OPEN 0
struct IPAddress { int operator[](int i) const { return i==3?1:127; } };
struct TestWiFi {
  bool connected=false;unsigned beginCalls=0;
  int status() { return connected?WL_CONNECTED:0; }
  IPAddress localIP() { return {}; }
  void disconnect() { connected=false; }
  void begin(const char*,const char*) { ++beginCalls;connected=true; }
  int scanNetworks() { return 0; }
  std::string SSID(int) { return ""; }
  int RSSI(int) { return -50; }
  int encryptionType(int) { return 0; }
};
extern TestWiFi WiFi;
