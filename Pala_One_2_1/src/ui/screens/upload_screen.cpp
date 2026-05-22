#include "src/ui/screens/upload_screen.h"

#include "src/hal/display.h"
#include "src/hal/softap.h"
#include "src/state.h"            // server
#include "src/storage/library.h"
#include "src/ui/font.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/widgets.h"
#include "src/web/apps_upload.h"  // resetAppUpload()
#include "src/web/upload.h"       // resetBookUpload() / resetSleepUpload()

void UploadScreen::onEnter() {
  startSession();
  draw();
}

void UploadScreen::draw() {
  prepareMenuFrame();

  int y = drawSectionHeader(D_UPLOAD_HEADER);

  Font::useBold();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPLOAD_WIFI);
  y += 14;

  Font::useBody();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(net_.ssid);
  y += 16;

  Font::useBold();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPLOAD_PASSWORD);
  y += 14;

  Font::useBody();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(net_.pass);
  y += 16;

  Font::useBold();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(D_UPLOAD_OPEN);
  y += 14;

  Font::useBody();
  u8g2.setCursor(MARGIN_X, y);
  u8g2.print(net_.url.c_str());
  y += 18;

  display.update();
}

void UploadScreen::startSession() {
  startedMs_ = millis();
  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  net_ = softApBegin();

  server.begin();
}

void UploadScreen::stopSessionToLibrary() {
  server.stop();
  softApEnd();
  resetBookUpload();
  resetSleepUpload();
  resetAppUpload();

  loadBooks();
  resetInputFrontend();
  nextScreen = &g_libraryScreen;
}

void UploadScreen::onButton(const ButtonEvent& e) {
  if (e.kind == ButtonEvent::Short || e.kind == ButtonEvent::Triple) {
    stopSessionToLibrary();
  }
}

void UploadScreen::onIdleTick() {
  server.handleClient();
  if ((uint32_t)(millis() - startedMs_) > UPLOAD_AUTO_EXIT_MS) {
    stopSessionToLibrary();
  }
}
