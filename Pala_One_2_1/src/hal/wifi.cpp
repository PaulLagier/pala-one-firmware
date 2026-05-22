#include "src/hal/wifi.h"

#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_wifi.h>

#include "src/state.h"               // AP_SSID, AP_PASS
#include "src/storage/wifi_creds.h"

static constexpr const char* kMdnsHost = "pala-one";

// Cached STA SSID so wifiStaPoll() can fill the WifiSession without going
// back to NVS. Cleared by wifiStaAbort() / wifiEnd().
static String s_staSsid;

bool wifiStaBegin() {
  if (!WifiCreds::has()) return false;

  setCpuFrequencyMhz(240);

  s_staSsid = WifiCreds::ssid();
  const String pass = WifiCreds::pass();

  WiFi.mode(WIFI_STA);
  WiFi.begin(s_staSsid.c_str(), pass.c_str());
  return true;
}

WifiStaResult wifiStaPoll(WifiSession& out) {
  switch (WiFi.status()) {
    case WL_CONNECTED: {
      IPAddress ip = WiFi.localIP();
      MDNS.begin(kMdnsHost);
      MDNS.addService("http", "tcp", 80);

      out.mode        = WifiMode::Station;
      out.staSsid     = s_staSsid;
      out.primaryUrl  = String("http://") + kMdnsHost + ".local";
      out.fallbackUrl = String("http://") + ip.toString();
      return WifiStaResult::Connected;
    }
    case WL_NO_SSID_AVAIL:
    case WL_CONNECT_FAILED:
    case WL_CONNECTION_LOST:
      return WifiStaResult::Failed;
    default:
      return WifiStaResult::Connecting;
  }
}

void wifiStaAbort() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  s_staSsid = "";
  // CPU clock stays at 240 MHz — the typical next call is
  // wifiBeginAccessPoint() which needs it anyway. wifiEnd() drops it.
}

WifiSession wifiBeginAccessPoint() {
  setCpuFrequencyMhz(240);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();

  WifiSession s;
  s.mode       = WifiMode::AccessPoint;
  s.apSsid     = AP_SSID;
  s.apPass     = AP_PASS;
  s.primaryUrl = String("http://") + ip.toString();
  return s;
}

void wifiEnd() {
  MDNS.end();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  esp_wifi_stop();
  btStop();
  s_staSsid = "";
  setCpuFrequencyMhz(80);  // back to low-power idle
}
