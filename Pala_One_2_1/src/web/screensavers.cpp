#include "src/web/screensavers.h"

#include <WiFi.h>   // WiFiClient

#include "src/config.h"
#include "src/state.h"
#include "src/ui/screensavers.h"

// ============================================================================
//  Binary screensaver endpoints used by the SPA at /#/screensavers:
//
//    GET  /screensavers/thumb     -> image/bmp  (?single=1 | ?slot=N)
//    GET  /screensavers/download  -> octet-stream attachment (single | slot)
//    POST /screensavers/upload    -> multipart .bin   (single | slot | auto)
//
//  State changes (mode picker + delete) live in src/web/api_screensavers.cpp
//  as JSON endpoints; this file only handles raw bytes in/out. The SPA's
//  slot grid `<img src>`s come straight from here.
// ============================================================================

namespace {

// Per-session upload state. `slotTarget == -1` and `legacy == true` means
// the upload is destined for /sleep.bin (single image); otherwise it goes
// into a Screensavers slot.
struct SlotUpload {
  File   tmpFile;
  String tmpPath;
  int    slotTarget = -1;
  bool   legacy     = false;
  bool   ok         = false;
  String error;
};

SlotUpload s_up;

}  // namespace

// Reverse the bit order of one byte. The XBM/XBitmap format is LSB-first;
// BMP's monochrome row data is MSB-first. One reverse per byte at thumbnail
// generation time is plenty cheap for 3904 bytes.
static uint8_t reverseBits8(uint8_t b) {
  b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
  b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
  b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
  return b;
}

// ============================================================================
//  GET /screensavers/thumb — render the requested image as a 250x122 BMP.
// ============================================================================
static void handleSleepThumb() {
  uint8_t buf[Screensavers::SCREENSAVER_BYTES];
  bool gotBytes = false;

  if (server.hasArg("single")) {
    File f = FS.open("/sleep.bin", "r");
    if (f && f.size() >= (size_t)Screensavers::SCREENSAVER_BYTES) {
      gotBytes = (f.read(buf, Screensavers::SCREENSAVER_BYTES) ==
                  (size_t)Screensavers::SCREENSAVER_BYTES);
    }
    if (f) f.close();
  } else if (server.hasArg("slot")) {
    int slot = server.arg("slot").toInt();
    gotBytes = Screensavers::readSlot(slot, buf);
  }

  if (!gotBytes) {
    server.send(404, "text/plain; charset=utf-8", "Thumbnail not found");
    return;
  }

  // Minimal 1-bit BMP — 14-byte file header, 40-byte info header, 8-byte
  // 2-color palette, then bottom-up row data (32 bytes per row, MSB-first).
  const int rowBytes = 32;
  const int bmpHdr   = 14 + 40 + 8;
  const int imgBytes = rowBytes * SCREEN_H;
  const int total    = bmpHdr + imgBytes;

  uint8_t fileHeader[14] = {
    0x42, 0x4D,
    (uint8_t)(total & 0xFF), (uint8_t)((total >> 8) & 0xFF),
    (uint8_t)((total >> 16) & 0xFF), (uint8_t)((total >> 24) & 0xFF),
    0, 0, 0, 0,
    (uint8_t)(bmpHdr & 0xFF), (uint8_t)((bmpHdr >> 8) & 0xFF),
    (uint8_t)((bmpHdr >> 16) & 0xFF), (uint8_t)((bmpHdr >> 24) & 0xFF)
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
  // BMP palette: index 0 = black, index 1 = white. The eink panel draws
  // 1 = white pixel, so flip the palette so the preview matches.
  uint8_t palette[8] = { 0, 0, 0, 0,  255, 255, 255, 0 };

  server.setContentLength(total);
  server.send(200, "image/bmp", "");
  WiFiClient client = server.client();
  client.write(fileHeader, sizeof(fileHeader));
  client.write(infoHeader, sizeof(infoHeader));
  client.write(palette,    sizeof(palette));

  uint8_t row[rowBytes];
  for (int y = SCREEN_H - 1; y >= 0; y--) {
    const uint8_t* src = &buf[y * rowBytes];
    for (int i = 0; i < rowBytes; i++) row[i] = reverseBits8(src[i]);
    client.write(row, rowBytes);
  }
}

// ============================================================================
//  GET /screensavers/download — serve raw 3904-byte .bin for sharing.
// ============================================================================
static void handleSleepDownload() {
  uint8_t buf[Screensavers::SCREENSAVER_BYTES];
  bool gotBytes = false;
  String filename = "screensaver.bin";

  if (server.hasArg("single")) {
    File f = FS.open("/sleep.bin", "r");
    if (f && f.size() >= (size_t)Screensavers::SCREENSAVER_BYTES) {
      gotBytes = (f.read(buf, Screensavers::SCREENSAVER_BYTES) ==
                  (size_t)Screensavers::SCREENSAVER_BYTES);
    }
    if (f) f.close();
    filename = "sleep.bin";
  } else if (server.hasArg("slot")) {
    int slot = server.arg("slot").toInt();
    if (slot >= 0 && slot < Screensavers::MAX_SLOTS) {
      gotBytes = Screensavers::readSlot(slot, buf);
      filename = "screensaver-slot-" + String(slot) + ".bin";
    }
  }

  if (!gotBytes) {
    server.send(404, "text/plain; charset=utf-8", "Screensaver not found");
    return;
  }

  server.setContentLength(Screensavers::SCREENSAVER_BYTES);
  server.sendHeader(
    "Content-Disposition",
    String("attachment; filename=\"") + filename + "\""
  );
  server.send(200, "application/octet-stream", "");
  WiFiClient client = server.client();
  client.write(buf, Screensavers::SCREENSAVER_BYTES);
}

// ============================================================================
//  POST /screensavers/upload — multipart upload to a specific slot or to
//  /sleep.bin. Streaming, atomic via .tmp rename.
// ============================================================================

static void handleScreensaverUploadDone() {
  if (!s_up.ok) {
    server.send(400, "text/plain; charset=utf-8",
                s_up.error.length() ? s_up.error : "Upload failed");
    return;
  }
  // The legacy handler 302'd back to /screensavers; the SPA polls the
  // state endpoint after every upload anyway, so a tiny JSON is plenty.
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true}");
}

static void handleScreensaverUploadStream() {
  HTTPUpload& up = server.upload();

  if (up.status == UPLOAD_FILE_START) {
    s_up = SlotUpload{};
    s_up.legacy = server.hasArg("single");
    if (!s_up.legacy) {
      // No slot arg → auto-pick first free.
      int requested = -1;
      if (server.hasArg("slot")) requested = server.arg("slot").toInt();
      if (requested < 0 || requested >= Screensavers::MAX_SLOTS) {
        requested = Screensavers::firstFreeSlot();
      }
      if (requested < 0) {
        s_up.error = "All rotation slots are full";
        return;
      }
      s_up.slotTarget = requested;
      s_up.tmpPath = "/screensavers/upload.tmp";
    } else {
      s_up.tmpPath = "/sleep.bin.tmp";
    }
    if (FS.exists(s_up.tmpPath)) FS.remove(s_up.tmpPath);
    s_up.tmpFile = FS.open(s_up.tmpPath, "w");
    if (!s_up.tmpFile) s_up.error = "Cannot create temp file";
  }
  else if (up.status == UPLOAD_FILE_WRITE) {
    if (s_up.tmpFile) s_up.tmpFile.write(up.buf, up.currentSize);
  }
  else if (up.status == UPLOAD_FILE_END) {
    if (s_up.tmpFile) s_up.tmpFile.close();
    File f = FS.open(s_up.tmpPath, "r");
    size_t sz = f ? f.size() : 0;
    if (f) f.close();

    if (sz != (size_t)Screensavers::SCREENSAVER_BYTES) {
      if (FS.exists(s_up.tmpPath)) FS.remove(s_up.tmpPath);
      s_up.error = (sz == 0)
        ? "Please choose an image first."
        : "Image must be exactly 3904 bytes";
      s_up.ok = false;
    } else if (s_up.legacy) {
      if (FS.exists("/sleep.bin")) FS.remove("/sleep.bin");
      if (FS.rename(s_up.tmpPath, "/sleep.bin")) {
        s_up.ok = true;
      } else {
        if (FS.exists(s_up.tmpPath)) FS.remove(s_up.tmpPath);
        s_up.error = "Failed to save sleep image";
      }
    } else {
      if (Screensavers::installFromTemp(s_up.slotTarget, s_up.tmpPath)) {
        s_up.ok = true;
      } else {
        s_up.error = "Failed to save rotation slot";
      }
    }
    s_up.tmpPath = "";
  }
  else if (up.status == UPLOAD_FILE_ABORTED) {
    if (s_up.tmpFile) s_up.tmpFile.close();
    if (s_up.tmpPath.length() > 0 && FS.exists(s_up.tmpPath)) FS.remove(s_up.tmpPath);
    s_up.tmpPath = "";
    s_up.ok = false;
    s_up.error = "Upload aborted";
  }
}

void registerScreensaverRoutes() {
  server.on("/screensavers/thumb",    HTTP_GET,  handleSleepThumb);
  server.on("/screensavers/download", HTTP_GET,  handleSleepDownload);
  server.on("/screensavers/upload",   HTTP_POST,
            handleScreensaverUploadDone, handleScreensaverUploadStream);
}
