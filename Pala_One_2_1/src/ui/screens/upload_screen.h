#ifndef PALA_UI_SCREENS_UPLOAD_SCREEN_H
#define PALA_UI_SCREENS_UPLOAD_SCREEN_H

#include "src/hal/wifi.h"    // WifiSession (cached for draw())
#include "src/ui/screen.h"

class UploadScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent& e) override;
  void draw() override;
  void onIdleTick() override;

  // The Wi-Fi session (AP or STA) can't keep running while the device
  // deep-sleeps.
  bool allowSleep() const override { return false; }
#if PALA_DIAGNOSTICS
  bool diagnosticChoosing() const { return phase_ == Phase::Choose; }
  bool diagnosticBrowserReady() const { return phase_ == Phase::Ready; }
#endif

private:
  // Two phases:
  //   ConnectingSta — STA association in flight; onIdleTick polls, onButton
  //                   treats any tap as "fall back to AP".
  //   Ready         — session is live (STA or AP); onIdleTick services the
  //                   HTTP server, onButton exits to the library.
  enum class Phase { Choose, StartingBluetooth, Bluetooth, StartingWifi, ConnectingSta, Ready, Error };

  Phase       phase_         = Phase::Ready;
  uint32_t    startedMs_     = 0;    // for the auto-exit timer (set on entry to Ready)
  uint32_t    staStartedMs_  = 0;    // when wifiStaBegin() was called (for the 5s timeout)
  int view_ = 0;                    // network, primary URL, optional fallback, exit
  WifiSession net_;                  // cached session info shown by draw()

  // Store the error kind, never a pointer into a replaceable locale pool.
  enum class Failure : uint8_t { None, Bluetooth, Wifi };
  Failure failure_ = Failure::None;
  void beginSession();
  void beginBluetooth();
  void enterReady();           // server.begin + draw — used after STA success or AP setup
  void fallbackToAp();         // abort STA in flight, bring up AP, enter Ready
  void stopSessionToLibrary();
};

extern UploadScreen g_uploadScreen;

#endif  // PALA_UI_SCREENS_UPLOAD_SCREEN_H
