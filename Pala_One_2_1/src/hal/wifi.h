#ifndef PALA_HAL_WIFI_H
#define PALA_HAL_WIFI_H

#include <Arduino.h>

// Upload-session network primitives. All non-blocking — the upload screen
// drives the STA-vs-AP decision itself with a small state machine so the
// main loop stays alive throughout (button events, Improv polling, screen
// redraws keep working through a slow STA association).
//
// Typical sequence:
//
//   if (wifiStaBegin()) {
//     // poll wifiStaPoll() each loop iteration; on Connected, take the
//     // session. On Failed (or after the caller's own timeout) call
//     // wifiStaAbort() and fall back to wifiBeginAccessPoint().
//   } else {
//     // no stored creds — straight to AP.
//     session = wifiBeginAccessPoint();
//   }
//   ... use session ...
//   wifiEnd();
//
// CPU clock: any of the begin functions bumps to 240 MHz (Wi-Fi needs it);
// wifiEnd() drops back to 80 MHz. wifiStaAbort() leaves the clock alone in
// case the caller is about to call wifiBeginAccessPoint().

enum class WifiMode { Station, AccessPoint };

struct WifiSession {
  WifiMode    mode = WifiMode::AccessPoint;
  String      primaryUrl;     // STA: http://pala-one.local  AP: http://192.168.4.1
  String      fallbackUrl;    // STA: http://<lan-ip>        AP: ""
  const char* apSsid = "";    // populated in AccessPoint mode only
  const char* apPass = "";    // populated in AccessPoint mode only
  String      staSsid;        // populated in Station mode only
};

enum class WifiStaResult {
  Connecting,   // still associating — keep polling
  Connected,    // associated; `out` is filled, stop polling
  Failed,       // hard error from the stack (no SSID, bad password, etc.)
};

// Kick off an STA association attempt against the stored credentials.
// Returns false if no creds are saved (caller should go straight to AP).
// Non-blocking; bumps CPU to 240 MHz.
bool          wifiStaBegin();

// Poll the in-flight STA attempt. On Connected, `out` is filled with the
// session and the caller takes ownership. On Failed, the caller should
// call wifiStaAbort() and fall back to AP. The caller decides when to
// give up on Connecting (typical: 5s timeout).
WifiStaResult wifiStaPoll(WifiSession& out);

// Tear down an in-flight STA attempt. Cheaper than wifiEnd() because no
// full session was ever brought up; leaves the CPU clock alone since the
// caller typically calls wifiBeginAccessPoint() next.
void          wifiStaAbort();

// Bring up SoftAP. Always succeeds. Bumps CPU to 240 MHz if not already.
WifiSession   wifiBeginAccessPoint();

// Tears down whichever mode is active and drops CPU back to 80 MHz.
void          wifiEnd();

// True while the radio has been brought up by one of the begin functions
// above and not yet torn down. This is a UI-facing indicator for the header
// status tray, not a link-state query — in Station mode it goes true at
// wifiStaBegin(), i.e. while still associating, not at WL_CONNECTED.
//
// Caveat: Sleep::enter() releases the radio directly rather than through
// wifiEnd(), so the flag stays true across that call. Harmless, because deep
// sleep is a reboot and statics re-initialize on wake.
bool          wifiActive();

#endif  // PALA_HAL_WIFI_H
