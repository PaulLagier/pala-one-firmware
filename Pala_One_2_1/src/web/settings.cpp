#include "src/web/settings.h"

#include <WiFi.h>

#include "src/config.h"
#include "src/state.h"
#include "src/ui/font.h"
#include "src/ui/sleep.h"
#include "src/ui/sleep_slots.h"
#include "src/web/chrome.h"

static void handleSettings() {
  int curFont = Font::currentBodySize();
  String sel8  = (curFont == 8)  ? " selected" : "";
  String sel10 = (curFont == 10) ? " selected" : "";
  String sel12 = (curFont == 12) ? " selected" : "";
  String sel14 = (curFont == 14) ? " selected" : "";

  int curSleep = Sleep::idleTimeoutSecs();
  String ss30   = (curSleep == 30)   ? " selected" : "";
  String ss60   = (curSleep == 60)   ? " selected" : "";
  String ss120  = (curSleep == 120)  ? " selected" : "";
  String ss300  = (curSleep == 300)  ? " selected" : "";
  String ss600  = (curSleep == 600)  ? " selected" : "";
  String ss1800 = (curSleep == 1800) ? " selected" : "";

  int curGap = Font::currentLineGap();
  String lg0 = (curGap == 0) ? " selected" : "";
  String lg1 = (curGap == 1) ? " selected" : "";
  String lg2 = (curGap == 2) ? " selected" : "";
  String lg3 = (curGap == 3) ? " selected" : "";

  bool hasSleepImg = FS.exists("/sleep.bin");
  bool multiScreensaver = (prefs.getInt("cfg_ss_multi", 0) == 1);
  int sleepMode = prefs.getInt("cfg_ss_mode", 0);
  if (sleepMode != 1) sleepMode = 0;
  String ssmCycle = (sleepMode == 0) ? " checked" : "";
  String ssmShuffle = (sleepMode == 1) ? " checked" : "";

  int slotIds[MAX_MULTI_SLEEP_SLOTS];
  int slotCount = collectSleepSlots(slotIds);
  int nextSlot = findNextFreeSleepSlot();

  String out = webPageStart(
    D_WEB_SETTINGS_TITLE,
    D_WEB_SETTINGS_SUBTITLE_PREFIX FW_BUILD D_WEB_SETTINGS_SUBTITLE_SUFFIX,
    "<a href='/'>" D_WEB_SETTINGS_BACK_NAV "</a>"
  );
  out.reserve(out.length() + 8000);

  out +=
    "<div class='card'><h2>" D_WEB_READING_HEADING "</h2>"
    "<form method='POST' action='/settings' accept-charset='UTF-8'>"
    "<div class='grid cols-2'>"
    "<div><label for='font'>" D_WEB_FONT_SIZE_LABEL "</label><select id='font' name='font'>"
    "<option value='8'";  out += sel8;  out += ">" D_WEB_FONT_SIZE_8  "</option>"
    "<option value='10'"; out += sel10; out += ">" D_WEB_FONT_SIZE_10 "</option>"
    "<option value='12'"; out += sel12; out += ">" D_WEB_FONT_SIZE_12 "</option>"
    "<option value='14'"; out += sel14; out += ">" D_WEB_FONT_SIZE_14 "</option>"
    "</select><div class='hint'>" D_WEB_FONT_SIZE_HINT "</div></div>"
    "<div><label for='sleep'>" D_WEB_SLEEP_AFTER_LABEL "</label><select id='sleep' name='sleep'>"
    "<option value='30'";   out += ss30;   out += ">" D_WEB_SLEEP_30S "</option>"
    "<option value='60'";   out += ss60;   out += ">" D_WEB_SLEEP_1M  "</option>"
    "<option value='120'";  out += ss120;  out += ">" D_WEB_SLEEP_2M  "</option>"
    "<option value='300'";  out += ss300;  out += ">" D_WEB_SLEEP_5M  "</option>"
    "<option value='600'";  out += ss600;  out += ">" D_WEB_SLEEP_10M "</option>"
    "<option value='1800'"; out += ss1800; out += ">" D_WEB_SLEEP_30M "</option>"
    "</select><div class='hint'>" D_WEB_SLEEP_HINT "</div></div>"
    "<div><label for='lgap'>" D_WEB_LINE_SPACING_LABEL "</label><select id='lgap' name='lgap'>"
    "<option value='0'"; out += lg0; out += ">" D_WEB_LINE_SPACING_0 "</option>"
    "<option value='1'"; out += lg1; out += ">" D_WEB_LINE_SPACING_1 "</option>"
    "<option value='2'"; out += lg2; out += ">" D_WEB_LINE_SPACING_2 "</option>"
    "<option value='3'"; out += lg3; out += ">" D_WEB_LINE_SPACING_3 "</option>"
    "</select><div class='hint'>" D_WEB_LINE_SPACING_HINT "</div></div>"
    "</div>"
    "<div class='actions' style='margin-top:24px'><button type='submit'>" D_WEB_SAVE_SETTINGS_BUTTON "</button><span class='muted'>" D_WEB_SETTINGS_NO_EXTRAS "</span></div>"
    "</form></div>";

  if (multiScreensaver) {
    out += "<div class='card'><h2>" D_WEB_SCREENSAVER_HEADING "</h2>"
      "<p>" D_WEB_SCREENSAVER_SPECS "</p>"
      "<p class='muted'>" D_WEB_SCREENSAVER_TIP "</p>"
      "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:8px'>"
      "<input type='hidden' name='sscfg' value='1'>"
      "<input type='hidden' name='ssmulti' value='1'>"
      "<div><label>" D_WEB_SS_ORDER_LABEL "</label>"
      "<label style='display:flex;gap:10px;align-items:flex-start;margin-top:8px'><input type='radio' name='ssmode' value='cycle'";
    out += ssmCycle;
    out += "><span>" D_WEB_SS_CYCLE "</span></label>"
      "<label style='display:flex;gap:10px;align-items:flex-start;margin-top:8px'><input type='radio' name='ssmode' value='shuffle'";
    out += ssmShuffle;
    out += "><span>" D_WEB_SS_SHUFFLE "</span></label>"
      "</div>"
      "<div class='actions'><button type='submit'>" D_WEB_SAVE_SETTINGS_BUTTON "</button></div>"
      "</form>"
      "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:10px'>"
      "<input type='hidden' name='sscfg' value='1'>"
      "<div class='actions'><button type='submit'>" D_WEB_SS_USE_SINGLE "</button></div>"
      "</form></div>";

    out += "<div class='card'><h2>" D_WEB_SS_FILES_HEADING "</h2>";

    for (int i = 0; i < slotCount; i++) {
      int slot = slotIds[i];
      out += "<div style='display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 0;border-bottom:1px solid var(--line-soft)'>";
      out += "<img src='/sleep-thumb?slot=";
      out += String(slot);
      out += "' alt='slot ";
      out += String(slot);
      out += "' style='width:180px;height:auto;border:1px solid var(--line);border-radius:10px;background:var(--card)'>";
      out += "<form method='POST' action='/sleep-slot-remove'><input type='hidden' name='slot' value='";
      out += String(slot);
      out += "'><button type='submit' class='btn secondary' style='background:transparent;border:0;padding:0;color:var(--link)'>" D_WEB_SS_REMOVE "</button></form>";
      out += "</div>";
    }

    String slotDisabled = (nextSlot < 0) ? " disabled" : "";
    out += "<h2 style='margin-top:14px'>" D_WEB_SS_ADD_ROTATION "</h2>"
      "<form method='POST' action='/upload-sleep-slot' enctype='multipart/form-data' onsubmit=\"var inp=this.elements['file'];if(!inp||!inp.files||!inp.files.length||!inp.files[0].size){alert('" D_WEB_SLEEP_ERR_EMPTY_CHOOSE "');return false;}return true;\">"
      "<div class='grid'><div><input type='file' name='file' accept='.bin,.BIN,application/octet-stream,*/*'";
    out += slotDisabled;
    out += "></div></div>"
      "<div class='actions'><button type='submit'";
    out += slotDisabled;
    out += ">" D_WEB_SCREENSAVER_UPLOAD_BUTTON "</button></div></form>"
      "</div>";
  } else {
    out += "<div class='card'><h2>" D_WEB_SCREENSAVER_HEADING "</h2>"
      "<p>" D_WEB_SCREENSAVER_SPECS "</p>"
      "<p class='muted'>" D_WEB_SCREENSAVER_TIP "</p>";
    if (hasSleepImg) {
      out += "<div style='display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 0;border-bottom:1px solid var(--line-soft)'>";
      out += "<img src='/sleep-thumb?single=1' alt='single screensaver' style='width:180px;height:auto;border:1px solid var(--line);border-radius:10px;background:var(--card)'>";
      out += "<form method='POST' action='/del-sleep' style='display:inline'><button type='submit' class='btn secondary' style='background:transparent;border:0;padding:0;color:var(--link)' onclick=\"return confirm('" D_WEB_CONFIRM_DEL_SCREENSAVER "')\">" D_WEB_SS_REMOVE "</button></form>";
      out += "</div>";
    } else {
      out += "<div class='status idle'>" D_WEB_SCREENSAVER_DEFAULT "</div>";
    }
    out +=
      "<form method='POST' action='/upload-sleep' enctype='multipart/form-data' style='margin-top:14px' onsubmit=\"var inp=this.elements['file'];if(!inp||!inp.files||!inp.files.length||!inp.files[0].size){alert('" D_WEB_SLEEP_ERR_EMPTY_CHOOSE "');return false;}return true;\">"
      "<div class='grid'><div><label for='file'>" D_WEB_SS_SINGLE_LABEL "</label><input id='file' type='file' name='file' accept='.bin'></div></div>"
      "<div class='actions'><button type='submit'>" D_WEB_SCREENSAVER_UPLOAD_BUTTON "</button></div>"
      "</form>"
      "<form method='POST' action='/settings' accept-charset='UTF-8' style='margin-top:10px'>"
      "<input type='hidden' name='sscfg' value='1'>"
      "<input type='hidden' name='ssmulti' value='1'>"
      "<input type='hidden' name='ssmode' value='cycle'>"
      "<div class='actions'><button type='submit'>" D_WEB_SS_USE_MULTI "</button></div>"
      "</form></div>";
  }

  out += webPageEnd();
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.send(200, "text/html; charset=utf-8", out);
}

static void handleSettingsPost() {
  // Font::setBodySize, Font::setLineGap, Sleep::setIdleTimeout all clamp /
  // validate internally — no inline guards needed here.
  if (server.hasArg("font")) {
    int fs = server.arg("font").toInt();
    if (fs != Font::currentBodySize()) Font::setBodySize(fs);
  }
  if (server.hasArg("sleep")) {
    int ss = server.arg("sleep").toInt();
    if (ss != Sleep::idleTimeoutSecs()) Sleep::setIdleTimeout(ss);
  }
  if (server.hasArg("lgap")) {
    int lg = server.arg("lgap").toInt();
    if (lg != Font::currentLineGap()) Font::setLineGap(lg);
  }
  if (server.hasArg("sscfg")) {
    bool multiEnabled = server.hasArg("ssmulti");
    prefs.putInt("cfg_ss_multi", multiEnabled ? 1 : 0);

    String m = server.arg("ssmode");
    int newMode = (m == "shuffle" || m == "rnd") ? 1 : 0;
    prefs.putInt("cfg_ss_mode", newMode);
  }
  // On-disk page caches are layout-stamped and self-invalidate on load, so
  // a font/lineGap change needs no cross-cutting cleanup here.
  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

static void handleDeleteSleepImg() {
  if (FS.exists("/sleep.bin")) FS.remove("/sleep.bin");
  resetSleepRotationState();
  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

static void handleSleepThumb() {
  String p;
  if (server.hasArg("single")) {
    p = "/sleep.bin";
  } else {
    int slot = parseSleepSlotArg(server.arg("slot"));
    if (slot < 0) {
      server.send(400, "text/plain; charset=utf-8", D_WEB_SLEEP_ERR_INVALID_SLOT);
      return;
    }
    p = sleepSlotPath(slot);
  }

  File f = FS.open(p, "r");
  if (!f || f.size() != SLEEP_FRAME_BYTES) {
    if (f) f.close();
    server.send(404, "text/plain; charset=utf-8", "Thumbnail not found");
    return;
  }

  static uint8_t buf[SLEEP_FRAME_BYTES];
  if (f.read(buf, SLEEP_FRAME_BYTES) != SLEEP_FRAME_BYTES) {
    f.close();
    server.send(500, "text/plain; charset=utf-8", "Read failed");
    return;
  }
  f.close();

  const int rowBytes = 32;
  const int bmpHeader = 14 + 40 + 8;
  const int imgBytes = rowBytes * SCREEN_H;
  const int total = bmpHeader + imgBytes;

  uint8_t fileHeader[14] = {
    0x42, 0x4D,
    (uint8_t)(total & 0xFF), (uint8_t)((total >> 8) & 0xFF),
    (uint8_t)((total >> 16) & 0xFF), (uint8_t)((total >> 24) & 0xFF),
    0, 0, 0, 0,
    (uint8_t)(bmpHeader & 0xFF), (uint8_t)((bmpHeader >> 8) & 0xFF),
    (uint8_t)((bmpHeader >> 16) & 0xFF), (uint8_t)((bmpHeader >> 24) & 0xFF)
  };
  uint8_t infoHeader[40] = {
    40, 0, 0, 0,
    (uint8_t)(SCREEN_W & 0xFF), (uint8_t)((SCREEN_W >> 8) & 0xFF), 0, 0,
    (uint8_t)(SCREEN_H & 0xFF), (uint8_t)((SCREEN_H >> 8) & 0xFF), 0, 0,
    1, 0,
    1, 0,
    0, 0, 0, 0,
    (uint8_t)(imgBytes & 0xFF), (uint8_t)((imgBytes >> 8) & 0xFF),
    (uint8_t)((imgBytes >> 16) & 0xFF), (uint8_t)((imgBytes >> 24) & 0xFF),
    0x13, 0x0B, 0, 0,
    0x13, 0x0B, 0, 0,
    2, 0, 0, 0,
    0, 0, 0, 0
  };
  uint8_t palette[8] = {0, 0, 0, 0, 255, 255, 255, 0};

  server.setContentLength(total);
  server.send(200, "image/bmp", "");
  WiFiClient client = server.client();
  client.write(fileHeader, sizeof(fileHeader));
  client.write(infoHeader, sizeof(infoHeader));
  client.write(palette, sizeof(palette));

  uint8_t row[rowBytes];
  for (int y = SCREEN_H - 1; y >= 0; y--) {
    const uint8_t* src = &buf[y * rowBytes];
    for (int i = 0; i < rowBytes; i++) row[i] = reverseBits8(src[i]);
    client.write(row, rowBytes);
  }
}

static void handleSleepSlotRemove() {
  int slot = parseSleepSlotArg(server.arg("slot"));
  if (slot >= 0) {
    String p = sleepSlotPath(slot);
    if (FS.exists(p)) FS.remove(p);
  }
  resetSleepRotationState();
  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

void registerSettingsRoutes() {
  server.on("/settings",         HTTP_GET,  handleSettings);
  server.on("/settings",         HTTP_POST, handleSettingsPost);
  server.on("/del-sleep",        HTTP_POST, handleDeleteSleepImg);
  server.on("/sleep-thumb",      HTTP_GET,  handleSleepThumb);
  server.on("/sleep-slot-remove", HTTP_POST, handleSleepSlotRemove);
}
