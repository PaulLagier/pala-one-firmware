#ifndef PALA_UI_SCREENS_UPLOAD_SCREEN_H
#define PALA_UI_SCREENS_UPLOAD_SCREEN_H

#include "src/hal/wifi.h" // WifiSession (cached for draw())
#include "src/ui/screen.h"

class UploadScreen : public Screen {
public:
  void onEnter() override;
  void onButton(const ButtonEvent &e) override;
  void draw() override;
  void onIdleTick() override;

  // Active radio sessions inhibit sleep. The chooser and a failed Bluetooth
  // start may use normal idle sleep.
  bool allowSleep() const override {
    return phase_ == Phase::Choose || phase_ == Phase::BluetoothError;
  }

private:
  // Choose keeps radios off; Bluetooth starts on the next idle tick.
  // An active Bluetooth phase services the app until exit/timeout.
  // Wi-Fi retains its two existing phases:
  //   ConnectingSta — STA association in flight; onIdleTick polls, onButton
  //                   treats any tap as "fall back to AP".
  //   Ready         — session is live (STA or AP); onIdleTick services the
  //                   HTTP server, onButton exits to the library.
  enum class Phase { Choose, StartingBluetooth, Bluetooth, BluetoothError, ConnectingSta, Ready };

  Phase phase_ = Phase::Choose;
  uint8_t choice_ = 0;
  int view_ = 0;
  void drawReadingView();
  uint32_t startedMs_ = 0;    // for the auto-exit timer (set on entry to Ready)
  uint32_t staStartedMs_ = 0; // when wifiStaBegin() was called (for the 5s timeout)
  WifiSession net_;           // cached session info shown by draw()

  void beginSession();
  void beginBluetooth();
  void enterReady();   // server.begin + draw — used after STA success or AP setup
  void fallbackToAp(); // abort STA in flight, bring up AP, enter Ready
  void stopSessionToLibrary();
};

extern UploadScreen g_uploadScreen;

#endif // PALA_UI_SCREENS_UPLOAD_SCREEN_H
