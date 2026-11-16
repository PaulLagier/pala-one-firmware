#pragma once
#include <string>
#define WL_CONNECTED 3
#define WIFI_AUTH_OPEN 0
struct IPAddress {
  int operator[](int i) const { return i == 3 ? 1 : 127; }
};
struct TestWiFi {
  bool connected = false;
  unsigned beginCalls = 0, radioCalls = 0;
  int status() {
    ++radioCalls;
    return connected ? WL_CONNECTED : 0;
  }
  IPAddress localIP() {
    ++radioCalls;
    return {};
  }
  void disconnect() {
    ++radioCalls;
    connected = false;
  }
  void begin(const char *, const char *) {
    ++radioCalls;
    ++beginCalls;
    connected = true;
  }
  int scanNetworks() {
    ++radioCalls;
    return 0;
  }
  std::string SSID(int) { return ""; }
  int RSSI(int) { return -50; }
  int encryptionType(int) { return 0; }
};
extern TestWiFi WiFi;
