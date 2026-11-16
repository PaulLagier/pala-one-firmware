#pragma once
#include <string>
#define WL_CONNECTED 3
#define WIFI_AUTH_OPEN 0
#define WIFI_OFF 0
struct IPAddress {
  int operator[](int i) const { return i == 3 ? 1 : 127; }
};
struct TestWiFi {
  bool connected = false;
  bool failConnection = false;
  unsigned beginCalls = 0, radioCalls = 0, modeCalls = 0;
  int radioMode = 0;
  void mode(int value) { ++radioCalls; ++modeCalls; radioMode=value; }
  int status() {
    ++radioCalls;
    return connected ? WL_CONNECTED : 0;
  }
  IPAddress localIP() {
    ++radioCalls;
    return {};
  }
  void disconnect(bool off=false, bool =false) {
    ++radioCalls;
    connected = false;
    if(off) radioMode &= ~1;
  }
  void begin(const char *, const char *) {
    ++radioCalls;
    ++beginCalls;
    radioMode |= 1;
    connected = !failConnection;
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
