#include "src/ui/centered_chrome.h"
#include "src/ui/screens/upload_screen.h"

#include "src/config.h"
#include "src/hal/companion_ble.h"
#include <WiFi.h>
#include "src/hal/display.h"
#include "src/hal/input.h"        // resetInputFrontend() — used on exit only
#include "src/hal/wifi.h"
#include "src/hal/wifi_provisioning.h"
#include "src/state.h"            // server
#include "src/storage/library.h"
#include "src/storage/wifi_creds.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/ui/pala_chrome.h"
#include "src/web/apps_upload.h"  // resetAppUpload()
#include "src/web/upload.h"       // resetBookUpload() / resetSleepUpload()

// How long to wait for an STA association before falling back to AP. Long
// enough for a typical 2.4 GHz home network (~1-3s); short enough that an
// unreachable network doesn't leave the user staring at "Connecting…".
static constexpr uint32_t kStaTimeoutMs = 5000;

// ---- Drawing --------------------------------------------------------------
static void drawConnecting(const String& ssid) {
  prepareMenuFrame();
  drawSectionHeader("");CenteredChrome::header(u8g2,D_UPLOAD_HEADER);
  CenteredChrome::message(gfx,u8g2,D_UPLOAD_CONNECTING,ssid.c_str());
  display.update();
}

void UploadScreen::draw() {
  if(phase_==Phase::Error){
    prepareMenuFrame();drawSectionHeader("");CenteredChrome::header(u8g2,D_UPLOAD_HEADER);Font::useMenu();
    CenteredChrome::message(gfx,u8g2,failure_==Failure::Bluetooth?D_CONNECT_BLE_UNAVAILABLE:D_CONNECT_WIFI_UNAVAILABLE);
    CenteredChrome::card(gfx,u8g2,89,D_READER_BACK_LIBRARY,true);display.update();return;
  }
#ifdef PALA_COMPANION_TEST
  if(phase_==Phase::Choose){
    prepareMenuFrame();drawSectionHeader("");CenteredChrome::header(u8g2,D_MENU_CONNECT);
    PalaChrome::menuList(gfx,u8g2,D_MENU_CONNECT,3,view_,[](int index)->String{return index==0?D_CONNECT_APP:index==1?D_CONNECT_BROWSER:D_READER_BACK_LIBRARY;});
    display.update();return;
  }
  if(phase_==Phase::StartingBluetooth || phase_==Phase::StartingWifi){
    prepareMenuFrame();drawSectionHeader("");CenteredChrome::header(u8g2,D_MENU_CONNECT);Font::useMenu();
    const char* message=phase_==Phase::StartingBluetooth?D_CONNECT_START_BLE:D_CONNECT_START_WIFI;
    CenteredChrome::message(gfx,u8g2,message);
    display.update();return;
  }
  if(phase_==Phase::Bluetooth){
    prepareMenuFrame();drawSectionHeader("");CenteredChrome::header(u8g2,"Pala App");Font::useMenu();
    PalaChrome::panel(gfx);
    Font::useToast();CenteredChrome::line(u8g2,companionBleConnected()?D_CONNECT_BLE_CONNECTED:D_CONNECT_PAIR_READY,47);
    PalaChrome::pairingCode(u8g2,companionPairingPin().c_str());
    CenteredChrome::card(gfx,u8g2,89,D_READER_BACK_LIBRARY,true);display.update();return;
  }
#endif
  prepareMenuFrame();
  drawSectionHeader("");CenteredChrome::header(u8g2,D_UPLOAD_HEADER);
  const int exitView = net_.fallbackUrl.length() ? 3 : 2;
  if (view_ < 0 || view_ > exitView) view_ = 0;
  Font::useToast();
  
  if (view_ == 0) {
    PalaChrome::panel(gfx);
    if (net_.mode == WifiMode::Station) {
      CenteredChrome::line(u8g2,D_UPLOAD_CONNECTED,45);
      CenteredChrome::line(u8g2,net_.staSsid.c_str(),65);
    } else {
      String network = String(D_UPLOAD_WIFI) + ": " + net_.apSsid;
      CenteredChrome::line(u8g2,network.c_str(),45);
      CenteredChrome::line(u8g2,D_UPLOAD_PASSWORD,59);
      Font::useMenuBold();
      CenteredChrome::line(u8g2,net_.apPass,79);
    }
    CenteredChrome::card(gfx, u8g2, 89, D_UPLOAD_OPEN, true);
  } else if (view_ < exitView) {
    PalaChrome::panel(gfx);
    CenteredChrome::line(u8g2,D_UPLOAD_OPEN,45);
    const String& url = view_ == 1 ? net_.primaryUrl : net_.fallbackUrl;
    Font::useMenuBold();
    CenteredChrome::line(u8g2,url.c_str(),69);
    CenteredChrome::card(gfx, u8g2, 89, D_WEB_READ_FIND_NEXT, true);
  } else {
    CenteredChrome::card(gfx, u8g2, 58, D_READER_BACK_LIBRARY, true);
  }
  display.update();
}

// ---- Lifecycle ------------------------------------------------------------
void UploadScreen::onEnter() {
#ifdef PALA_COMPANION_TEST
  phase_=Phase::Choose;view_=0;startedMs_=millis();draw();
#else
  beginSession();
#endif
}

void UploadScreen::beginBluetooth() {
#ifdef PALA_COMPANION_TEST
  resetBookUpload();resetSleepUpload();resetAppUpload();
  WifiProvisioning::notifyUploadSession(true);WiFi.mode(WIFI_OFF);
  setCpuFrequencyMhz(240);
  companionBleBegin();startedMs_=millis();
  if(companionBleRunning())phase_=Phase::Bluetooth;
  else {phase_=Phase::Error;failure_=Failure::Bluetooth;}
  draw();
#endif
}

void UploadScreen::beginSession() {

  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  // Tell WifiProvisioning to keep its hands off the radio for the duration
  // of the session — set BEFORE wifiStaBegin so a same-tick
  // WifiProvisioning::loop() can't race the WiFi.begin() we're about to fire.
  WifiProvisioning::notifyUploadSession(true);

  if (wifiStaBegin()) {
    phase_        = Phase::ConnectingSta;
    staStartedMs_ = millis();
    drawConnecting(WifiCreds::ssid());
  } else {
    // No stored creds — straight to AP, no point showing a splash for a
    // path we know will time out.
    net_ = wifiBeginAccessPoint();
    enterReady();
  }
}

void UploadScreen::enterReady() {
  if(!net_.ready){phase_=Phase::Error;failure_=Failure::Wifi;startedMs_=millis();draw();return;}
  phase_     = Phase::Ready;
  view_      = 0;
  startedMs_ = millis();
  server.begin();
  draw();
}

void UploadScreen::fallbackToAp() {
  wifiStaAbort();
  net_ = wifiBeginAccessPoint();
  enterReady();
}

void UploadScreen::stopSessionToLibrary() {
#ifdef PALA_COMPANION_TEST
  companionBleEnd();
#endif
  server.stop();
  wifiEnd();
  WifiProvisioning::notifyUploadSession(false);

  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  loadBooks();
  // Discard the exit-click so it doesn't leak into the library screen as a
  // menu selection.
  resetInputFrontend();
  nextScreen = &g_libraryScreen;
}

// ---- Per-iteration input + tick -------------------------------------------
void UploadScreen::onButton(const ButtonEvent& e) {
  if (!e.any()) return;
  if(phase_==Phase::Error){
    if(e.kind==ButtonEvent::Long || e.kind==ButtonEvent::Double || e.kind==ButtonEvent::Triple)stopSessionToLibrary();
    return;
  }
#ifdef PALA_COMPANION_TEST
  if(phase_==Phase::Choose){
    if(e.kind==ButtonEvent::Short){view_=(view_+1)%3;draw();}
    else if(e.kind==ButtonEvent::Triple)stopSessionToLibrary();
    else if(e.kind==ButtonEvent::Long || e.kind==ButtonEvent::Double){
      if(view_==2)stopSessionToLibrary();
      else {phase_=view_==0?Phase::StartingBluetooth:Phase::StartingWifi;startedMs_=millis();draw();}
    }
    return;
  }
  if(phase_==Phase::Bluetooth){
    if(e.kind==ButtonEvent::Long || e.kind==ButtonEvent::Double || e.kind==ButtonEvent::Triple)stopSessionToLibrary();
    return;
  }
#endif

  // Any tap during the connecting splash means "use the hotspot instead".
  if (phase_ == Phase::ConnectingSta) {
    fallbackToAp();
    return;
  }

  const int exitView = net_.fallbackUrl.length() ? 3 : 2;
  if (e.kind == ButtonEvent::Triple) { stopSessionToLibrary(); return; }
  if (e.kind == ButtonEvent::Short) {
    view_ = (view_ + 1) % (exitView + 1); draw(); return;
  }
  if (e.kind == ButtonEvent::Long || e.kind == ButtonEvent::Double) {
    if (view_ == exitView) stopSessionToLibrary();
    else { ++view_; draw(); }
  }
}

void UploadScreen::onIdleTick() {
  if(phase_==Phase::Error){if(uint32_t(millis()-startedMs_)>UPLOAD_AUTO_EXIT_MS)stopSessionToLibrary();return;}
#ifdef PALA_COMPANION_TEST
  if(phase_==Phase::StartingBluetooth){beginBluetooth();return;}
  if(phase_==Phase::StartingWifi){beginSession();return;}
  if(phase_==Phase::Choose){
    if(uint32_t(millis()-startedMs_)>UPLOAD_AUTO_EXIT_MS)stopSessionToLibrary();return;
  }
  if(phase_==Phase::Bluetooth){
    companionBleTick();if(companionBleNeedsRedraw())draw();
    if(companionBleConnected())startedMs_=millis();
    if(uint32_t(millis()-startedMs_)>UPLOAD_AUTO_EXIT_MS)stopSessionToLibrary();return;
  }
#endif
  if (phase_ == Phase::ConnectingSta) {
    WifiStaResult r = wifiStaPoll(net_);
    if (r == WifiStaResult::Connected) {
      enterReady();
      return;
    }
    if (r == WifiStaResult::Failed ||
        (uint32_t)(millis() - staStartedMs_) > kStaTimeoutMs) {
      fallbackToAp();
      return;
    }
    return;   // still associating — server isn't up yet
  }

  server.handleClient();
  if ((uint32_t)(millis() - startedMs_) > UPLOAD_AUTO_EXIT_MS) {
    stopSessionToLibrary();
  }
}
