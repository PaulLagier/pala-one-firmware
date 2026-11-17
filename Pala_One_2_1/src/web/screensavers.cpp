#include "src/web/web_assets.h"
#include "src/web/localized_scripts.h"
#include "src/lang/join.h"
#include "src/web/screensavers.h"

#include <WiFi.h> // WiFiClient

#include "src/config.h"
#include "src/state.h"
#include "src/ui/screensavers.h"
#include "src/web/chrome.h"

// ============================================================================
//  Screensaver editor + slot manager — WebUI side.
//
//  The actual rotation logic lives in src/ui/screensavers.{h,cpp}; this file
//  is purely HTTP glue: it renders the editor page, accepts multipart
//  uploads, serves slot thumbnails (as BMP so any browser can render them),
//  and accepts mode + delete commands.
// ============================================================================

namespace {

// Per-session upload state. `slotTarget == -1` and `legacy == true` means
// the upload is destined for /sleep.bin (legacy single image); otherwise
// it goes into a Screensavers slot.
struct SlotUpload {
  File tmpFile;
  String tmpPath;
  int slotTarget = -1;
  bool legacy = false;
  bool ok = false;
  String error;
};

SlotUpload s_up;

} // namespace

void resetScreensaverUpload() {
  if (s_up.tmpFile)
    s_up.tmpFile.close();
  s_up = SlotUpload{};
}

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
//  Used by the slot grid + the "current single image" preview.
// ============================================================================
static void handleSleepThumb() {
  uint8_t buf[Screensavers::SCREENSAVER_BYTES];
  bool gotBytes = false;

  if (server.hasArg("single")) {
    File f = FS.open("/sleep.bin", "r");
    if (f && f.size() >= (size_t)Screensavers::SCREENSAVER_BYTES) {
      gotBytes =
          (f.read(buf, Screensavers::SCREENSAVER_BYTES) == (size_t)Screensavers::SCREENSAVER_BYTES);
    }
    if (f)
      f.close();
  } else if (server.hasArg("slot")) {
    int slot = server.arg("slot").toInt();
    gotBytes = Screensavers::readSlot(slot, buf);
  }

  if (!gotBytes) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_SS_ERR_THUMB_NOT_FOUND);
    return;
  }

  // Minimal 1-bit BMP — 14-byte file header, 40-byte info header, 8-byte
  // 2-color palette, then bottom-up row data (32 bytes per row, MSB-first).
  const int rowBytes = 32;
  const int bmpHdr = 14 + 40 + 8;
  const int imgBytes = rowBytes * SCREEN_H;
  const int total = bmpHdr + imgBytes;

  uint8_t fileHeader[14] = {0x42,
                            0x4D,
                            (uint8_t)(total & 0xFF),
                            (uint8_t)((total >> 8) & 0xFF),
                            (uint8_t)((total >> 16) & 0xFF),
                            (uint8_t)((total >> 24) & 0xFF),
                            0,
                            0,
                            0,
                            0,
                            (uint8_t)(bmpHdr & 0xFF),
                            (uint8_t)((bmpHdr >> 8) & 0xFF),
                            (uint8_t)((bmpHdr >> 16) & 0xFF),
                            (uint8_t)((bmpHdr >> 24) & 0xFF)};
  uint8_t infoHeader[40] = {40,
                            0,
                            0,
                            0,
                            (uint8_t)(SCREEN_W & 0xFF),
                            (uint8_t)((SCREEN_W >> 8) & 0xFF),
                            0,
                            0,
                            (uint8_t)(SCREEN_H & 0xFF),
                            (uint8_t)((SCREEN_H >> 8) & 0xFF),
                            0,
                            0,
                            1,
                            0,
                            1,
                            0,
                            0,
                            0,
                            0,
                            0,
                            (uint8_t)(imgBytes & 0xFF),
                            (uint8_t)((imgBytes >> 8) & 0xFF),
                            (uint8_t)((imgBytes >> 16) & 0xFF),
                            (uint8_t)((imgBytes >> 24) & 0xFF),
                            0x13,
                            0x0B,
                            0,
                            0,
                            0x13,
                            0x0B,
                            0,
                            0,
                            2,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0};
  // BMP palette: index 0 = black, index 1 = white. The eink panel draws
  // 1 = white pixel, so flip the palette so the preview matches.
  uint8_t palette[8] = {0, 0, 0, 0, 255, 255, 255, 0};

  server.setContentLength(total);
  server.send(200, "image/bmp", "");
  WiFiClient client = server.client();
  client.write(fileHeader, sizeof(fileHeader));
  client.write(infoHeader, sizeof(infoHeader));
  client.write(palette, sizeof(palette));

  uint8_t row[rowBytes];
  for (int y = SCREEN_H - 1; y >= 0; y--) {
    const uint8_t *src = &buf[y * rowBytes];
    for (int i = 0; i < rowBytes; i++)
      row[i] = reverseBits8(src[i]);
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
      gotBytes =
          (f.read(buf, Screensavers::SCREENSAVER_BYTES) == (size_t)Screensavers::SCREENSAVER_BYTES);
    }
    if (f)
      f.close();
    filename = "sleep.bin";
  } else if (server.hasArg("slot")) {
    int slot = server.arg("slot").toInt();
    if (slot >= 0 && slot < Screensavers::MAX_SLOTS) {
      gotBytes = Screensavers::readSlot(slot, buf);
      filename = "screensaver-slot-" + String(slot) + ".bin";
    }
  }

  if (!gotBytes) {
    server.send(404, "text/plain; charset=utf-8", D_WEB_SS_ERR_NOT_FOUND);
    return;
  }

  server.setContentLength(Screensavers::SCREENSAVER_BYTES);
  server.sendHeader("Content-Disposition", String("attachment; filename=\"") + filename + "\"");
  server.send(200, "application/octet-stream", "");
  WiFiClient client = server.client();
  client.write(buf, Screensavers::SCREENSAVER_BYTES);
}

// ============================================================================
//  POST /screensavers/delete — remove a slot or the legacy single image.
// ============================================================================
static void handleSleepDelete() {
  if (server.hasArg("single")) {
    if (FS.exists("/sleep.bin"))
      FS.remove("/sleep.bin");
  } else if (server.hasArg("slot")) {
    int slot = server.arg("slot").toInt();
    Screensavers::deleteSlot(slot);
  }
  server.sendHeader("Location", "/screensavers");
  server.send(302, "text/plain", "");
}

// ============================================================================
//  POST /screensavers/mode — switch between Single / Cycle / Shuffle.
// ============================================================================
static void handleSleepModePost() {
  if (server.hasArg("mode")) {
    String m = server.arg("mode");
    if (m == "cycle")
      Screensavers::setMode(Screensavers::Mode::Cycle);
    else if (m == "shuffle")
      Screensavers::setMode(Screensavers::Mode::Shuffle);
    else
      Screensavers::setMode(Screensavers::Mode::Single);
  }
  server.sendHeader("Location", "/screensavers");
  server.send(302, "text/plain", "");
}

// ============================================================================
//  POST /screensavers/upload — multipart upload to a specific slot or to
//  the legacy /sleep.bin. Streaming, atomic via .tmp rename.
// ============================================================================

static void handleScreensaverUploadDone() {
  if (!s_up.ok) {
    server.send(400, "text/plain; charset=utf-8",
                s_up.error.length() ? s_up.error : D_WEB_SS_ERR_UPLOAD_FAILED);
    return;
  }
  server.sendHeader("Location", "/screensavers");
  server.send(302, "text/plain", "");
}

static void handleScreensaverUploadStream() {
  HTTPUpload &up = server.upload();

  if (up.status == UPLOAD_FILE_START) {
    s_up = SlotUpload{};
    s_up.legacy = server.hasArg("single");
    if (!s_up.legacy) {
      // No slot arg → auto-pick first free.
      int requested = -1;
      if (server.hasArg("slot"))
        requested = server.arg("slot").toInt();
      if (requested < 0 || requested >= Screensavers::MAX_SLOTS) {
        requested = Screensavers::firstFreeSlot();
      }
      if (requested < 0) {
        s_up.error = D_WEB_SS_ERR_SLOTS_FULL;
        return;
      }
      s_up.slotTarget = requested;
      s_up.tmpPath = "/screensavers/upload.tmp";
    } else {
      s_up.tmpPath = "/sleep.bin.tmp";
    }
    if (FS.exists(s_up.tmpPath))
      FS.remove(s_up.tmpPath);
    s_up.tmpFile = FS.open(s_up.tmpPath, "w");
    if (!s_up.tmpFile)
      s_up.error = D_WEB_SS_ERR_CANT_CREATE_TMP;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (s_up.error.length() > 0)
      return;
    if (s_up.tmpFile && up.currentSize > 0) {
      if (s_up.tmpFile.size() + up.currentSize > 8192) {
        s_up.tmpFile.close();
        if (FS.exists(s_up.tmpPath))
          FS.remove(s_up.tmpPath);
        s_up.error = D_WEB_SS_ERR_IMAGE_TOO_LARGE;
        return;
      }
      size_t wrote = s_up.tmpFile.write(up.buf, up.currentSize);
      if (wrote != up.currentSize) {
        s_up.tmpFile.close();
        if (FS.exists(s_up.tmpPath))
          FS.remove(s_up.tmpPath);
        s_up.error = D_WEB_ERR_WRITE_FAILED;
        return;
      }
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (s_up.tmpFile)
      s_up.tmpFile.close();
    File f = FS.open(s_up.tmpPath, "r");
    size_t sz = f ? f.size() : 0;
    if (f)
      f.close();

    if (sz != (size_t)Screensavers::SCREENSAVER_BYTES) {
      if (FS.exists(s_up.tmpPath))
        FS.remove(s_up.tmpPath);
      if (sz == 0) {
        s_up.error = D_WEB_SS_ERR_CHOOSE_IMAGE;
      } else {
        char msg[80];
        snprintf(msg, sizeof(msg), D_WEB_SS_ERR_EXACT_BYTES_FMT, Screensavers::SCREENSAVER_BYTES);
        s_up.error = msg;
      }
      s_up.ok = false;
    } else if (s_up.legacy) {
      if (FS.exists("/sleep.bin"))
        FS.remove("/sleep.bin");
      if (FS.rename(s_up.tmpPath, "/sleep.bin")) {
        s_up.ok = true;
      } else {
        if (FS.exists(s_up.tmpPath))
          FS.remove(s_up.tmpPath);
        s_up.error = D_WEB_SS_ERR_SAVE_SLEEP;
      }
    } else {
      if (Screensavers::installFromTemp(s_up.slotTarget, s_up.tmpPath)) {
        s_up.ok = true;
      } else {
        s_up.error = D_WEB_SS_ERR_SAVE_SLOT;
      }
    }
    s_up.tmpPath = "";
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (s_up.tmpFile)
      s_up.tmpFile.close();
    if (s_up.tmpPath.length() > 0 && FS.exists(s_up.tmpPath))
      FS.remove(s_up.tmpPath);
    s_up.tmpPath = "";
    s_up.ok = false;
    s_up.error = D_WEB_ERR_UPLOAD_ABORTED;
  }
}

// ============================================================================
//  Editor HTML + JS. PROGMEM strings keep the heavy strings out of RAM.
//  See Kevin's fork (Pala_One_2_1_kevinst1r/Pala_One_2_1_kevinst1r.ino,
//  ~line 4723) for the original; the JS is a faithful port with the upload
//  endpoint rewired to /screensavers/upload.
// ============================================================================

static const char kIconDownload[] PROGMEM = "<svg viewBox='0 0 24 24' aria-hidden='true'><path "
                                            "d='M12 4v10m0 0l4-4m-4 4l-4-4M5 20h14'/></svg>";
static const char kIconUpload[] PROGMEM = "<svg viewBox='0 0 24 24' aria-hidden='true'><path "
                                          "d='M12 20v-10m0 0l4 4m-4-4l-4 4M5 4h14'/></svg>";
static const char kIconTrash[] PROGMEM = "<svg viewBox='0 0 24 24' aria-hidden='true'>"
                                         "<path d='M4 7h16'/>"
                                         "<path d='M8 7V5h8v2'/>"
                                         "<path d='M7 7v12h10V7'/>"
                                         "<path d='M10 11v5M14 11v5'/>"
                                         "</svg>";

// Upload + (when populated) download + delete icon buttons for a slot or the
// legacy single image. Upload is always shown so an empty slot can be filled
// from a previously downloaded .bin without going through the image editor.
static String screensaverActionsHtml(bool single, int slot, bool populated) {
  String out;
  out.reserve(560);
  out += "<div class='ss-slot-actions'>";

  // Upload: label wraps a hidden file input so clicking the icon opens the
  // file picker; onchange auto-submits the form to the existing upload route.
  String uploadUrl =
      String("/screensavers/upload?") + (single ? "single=1" : "slot=" + String(slot));
  out += "<form method='POST' action='" + uploadUrl +
         "' enctype='multipart/form-data' style='margin:0'>";
  out += Language::join({"<label class='btn-icon' title='", D_WEB_SS_UPLOAD_ARIA, "' aria-label='",
                         D_WEB_SS_UPLOAD_ARIA, "'>"})
             .c_str();
  out += "<input type='file' name='file' accept='.bin' style='display:none' "
         "onchange='this.form.submit()'>";
  out += FPSTR(kIconUpload);
  out += "</label></form>";

  if (populated) {
    out += "<a class='btn-icon' href='/screensavers/download?";
    if (single)
      out += "single=1";
    else
      out += "slot=" + String(slot);
    out += Language::join({"' download title='", D_WEB_SS_DOWNLOAD_ARIA, "' aria-label='",
                           D_WEB_SS_DOWNLOAD_ARIA, "'>"})
               .c_str();
    out += FPSTR(kIconDownload);
    out += "</a>";
    out += "<form method='POST' action='/screensavers/delete' style='margin:0'>";
    if (single) {
      out += "<input type='hidden' name='single' value='1'>";
      out += Language::join({"<button type='submit' class='btn-icon danger' title='",
                             D_WEB_SS_DELETE_ARIA, "' ", "aria-label='", D_WEB_SS_DELETE_ARIA, "' ",
                             "onclick=\"return confirm('", D_WEB_SS_CONFIRM_DEL_SINGLE, "')\">"})
                 .c_str();
    } else {
      out += "<input type='hidden' name='slot' value='" + String(slot) + "'>";
      out += Language::join({"<button type='submit' class='btn-icon danger' title='",
                             D_WEB_SS_DELETE_ARIA, "' ", "aria-label='", D_WEB_SS_DELETE_ARIA, "' ",
                             "onclick=\"return confirm('", D_WEB_SS_CONFIRM_DEL_SLOT, "')\">"})
                 .c_str();
    }
    out += FPSTR(kIconTrash);
    out += "</button></form>";
  }

  out += "</div>";
  return out;
}

// Build the slot grid HTML — one card per slot with thumbnail (when
// populated) and a delete form. Appended into a containing card by the
// editor handler.
static String slotGridHtml() {
  String out;
  out.reserve(6000); // 8 slots × ~700 bytes each
  out += "<div class='ss-slots'>";
  for (int i = 0; i < Screensavers::MAX_SLOTS; i++) {
    bool exists = Screensavers::slotExists(i);
    out +=
        Language::join({"<div class='ss-slot'><div class='muted small'>", D_WEB_SS_SLOT_LABEL, " "})
            .c_str();
    out += String(i);
    out += "</div>";
    if (exists) {
      out += "<img src='/screensavers/thumb?slot=" + String(i) +
             Language::join({"' alt='", D_WEB_SS_SLOT_LABEL, " "}).c_str() + String(i) + "'>";
    } else {
      out += Language::join({"<div class='ss-slot-empty'>", D_WEB_SS_SLOT_EMPTY, "</div>"}).c_str();
    }
    out += screensaverActionsHtml(false, i, exists);
    out += "</div>";
  }
  out += "</div>";
  return out;
}

// Editor card HTML — canvas + sliders + destination dropdown + upload button.
// Heavy bits (CSS + JS) come from PROGMEM strings above.
static String editorCardHtml(int nextFreeSlot) {
  String out;
  out.reserve(3500);
  WebAssets::appendStyle(out, WebAssets::Id::EditorCss);
  out +=
      Language::join({"<div class='card'><h2>",
                      D_WEB_SS_EDITOR_HEADING,
                      "</h2>",
                      "<p class='muted'>",
                      D_WEB_SS_EDITOR_INTRO,
                      "</p>",
                      "<div class='ss-wrap'>",
                      "<div class='ss-card ss-grid'>",
                      "<div class='full'><div class='ss-label-row'><label for='ssEditFile'>",
                      D_WEB_SS_SOURCE_IMAGE,
                      "</label></div>",
                      "<input id='ssEditFile' type='file' accept='image/*,.bin'></div>",
                      "<div class='full'><div class='ss-label-row'><label for='ssTolerance'>",
                      D_WEB_SS_TOLERANCE,
                      "</label>",
                      "<span class='ss-value' id='ssToleranceLabel'>0%</span></div>",
                      "<input id='ssTolerance' type='range' min='-100' max='100' value='0'></div>",
                      "<div class='full'><label "
                      "style='display:flex;gap:10px;align-items:center;font-weight:500'>",
                      "<input id='ssInvert' type='checkbox'><span>",
                      D_WEB_SS_INVERT,
                      "</span></label></div>",
                      "<div class='full'><details class='ss-adv'><summary>",
                      D_WEB_SS_PRECISE_CONTROL,
                      "</summary><div class='ss-adv-body ss-grid'>",
                      "<div><div class='ss-label-row'><label for='ssZoom'>",
                      D_WEB_SS_ZOOM,
                      "</label><span class='ss-value' id='ssZoomLabel'>100%</span></div>",
                      "<input id='ssZoom' type='range' min='10' max='400' value='100'></div>",
                      "<div><div class='ss-label-row'><label for='ssPanX'>",
                      D_WEB_SS_MOVE_X,
                      "</label><span class='ss-value' id='ssPanXLabel'>0 px</span></div>",
                      "<input id='ssPanX' type='range' min='-250' max='250' value='0'></div>",
                      "<div class='full'><div class='ss-label-row'><label for='ssPanY'>",
                      D_WEB_SS_MOVE_Y,
                      "</label><span class='ss-value' id='ssPanYLabel'>0 px</span></div>",
                      "<input id='ssPanY' type='range' min='-180' max='180' value='0'></div>",
                      "</div></details></div>",
                      "</div>",
                      "<div class='ss-card ss-preview-wrap'>",
                      "<label>",
                      D_WEB_SS_PREVIEW_LABEL,
                      "</label>",
                      "<div class='ss-preview-stage'><canvas id='ssPreview' width='250' "
                      "height='122'></canvas></div>",
                      "<button type='button' class='btn secondary' id='ssResetBtn' "
                      "style='align-self:flex-start;padding:6px 12px;font-size:13px'>",
                      D_WEB_SS_RESET_FIT,
                      "</button><button type='button' class='btn secondary' id='ssRotateBtn'>",
                      D_WEB_SS_ROTATE,
                      "</button>",
                      "<div class='ss-meta' id='ssMeta'>",
                      D_WEB_SS_NO_IMAGE,
                      "</div>",
                      "</div>",
                      "<div class='ss-card'>",
                      "<div class='ss-label-row'><label for='ssDestination'>",
                      D_WEB_SS_SAVE_TO,
                      "</label></div>",
                      "<select id='ssDestination'>",
                      "<option value='single'>",
                      D_WEB_SS_DST_SINGLE,
                      "</option>"})
          .c_str();
  if (nextFreeSlot >= 0) {
    out += Language::join({"<option value='auto' selected>", D_WEB_SS_DST_AUTO_PREFIX}).c_str();
    out += String(nextFreeSlot);
    out += Language::join({D_WEB_SS_DST_AUTO_SUFFIX, "</option>"}).c_str();
  } else {
    out +=
        Language::join({"<option value='auto' disabled>", D_WEB_SS_DST_FULL, "</option>"}).c_str();
  }
  for (int i = 0; i < Screensavers::MAX_SLOTS; i++) {
    out += "<option value='" + String(i) +
           Language::join({"'>", D_WEB_SS_DST_SLOT_PREFIX}).c_str() + String(i);
    if (Screensavers::slotExists(i))
      out += D_WEB_SS_DST_OVERWRITE;
    out += "</option>";
  }
  out +=
      Language::join({"</select></div>", "<div class='actions'>",
                      "<button type='button' class='btn' id='ssUploadBtn'>", D_WEB_SS_UPLOAD_EDITED,
                      "</button>", "<span class='ss-status' id='ssUploadStatus'></span>",
                      "</div></div></div>"})
          .c_str();
  appendEditorScript(out);
  return out;
}

static void handleSleepEditorPage() {
  Screensavers::Mode m = Screensavers::currentMode();
  const char *selSingle = (m == Screensavers::Mode::Single) ? " selected" : "";
  const char *selCycle = (m == Screensavers::Mode::Cycle) ? " selected" : "";
  const char *selShuffle = (m == Screensavers::Mode::Shuffle) ? " selected" : "";

  int populated = Screensavers::populatedCount();
  bool hasLegacy = FS.exists("/sleep.bin");
  int nextFree = Screensavers::firstFreeSlot();

  String out = webPageStart(D_WEB_SS_TITLE, D_WEB_SS_SUBTITLE,
                            Language::join({"<a href='/'>", D_WEB_NAV_HOME,
                                            "</a><a href='/settings'>", D_WEB_NAV_SETTINGS, "</a>"})
                                .c_str(),
                            true);
  out.reserve(out.length() + 8000);

  // Editor.
  out += editorCardHtml(nextFree);

  // Mode picker.
  out +=
      Language::join(
          {"<div class='card'><h2>", D_WEB_SS_ROTATION_HEADING, "</h2>", "<p class='muted'>",
           D_WEB_SS_ROTATION_INTRO, "</p>",
           "<form method='POST' action='/screensavers/mode' class='stack' style='margin-top:12px'>",
           "<div class='grid cols-2'>", "<div><label for='ssMode'>", D_WEB_SS_MODE_LABEL,
           "</label><select id='ssMode' name='mode'>", "<option value='single'"})
          .c_str();
  out += selSingle;
  out += Language::join({">", D_WEB_SS_MODE_SINGLE, "</option>", "<option value='cycle'"}).c_str();
  out += selCycle;
  out += Language::join({">", D_WEB_SS_MODE_CYCLE, "</option>", "<option value='shuffle'"}).c_str();
  out += selShuffle;
  out += Language::join({">", D_WEB_SS_MODE_SHUFFLE, "</option>", "</select><div class='hint'>",
                         D_WEB_SS_SLOTS_POPULATED})
             .c_str();
  out += String(populated);
  out += "/";
  out += String(Screensavers::MAX_SLOTS);
  out += Language::join({"</div></div></div>", "<div class='actions'><button type='submit'>",
                         D_WEB_SS_SAVE_MODE, "</button></div>", "</form></div>"})
             .c_str();

  // Slot grid.
  out += Language::join({"<div class='card'><h2>", D_WEB_SS_SLOTS_HEADING, "</h2>"}).c_str();
  out += slotGridHtml();
  out += "</div>";

  // Legacy single image card.
  out += Language::join({"<div class='card'><h2>", D_WEB_SS_SINGLE_HEADING, "</h2>"}).c_str();
  if (hasLegacy) {
    out += Language::join(
               {"<div class='row' style='align-items:center;gap:12px'>",
                "<img src='/screensavers/thumb?single=1' alt='", D_WEB_SS_SINGLE_ALT, "' ",
                "style='width:180px;border:1px solid "
                "var(--line);border-radius:8px;background:#fff;image-rendering:pixelated'>"})
               .c_str();
    out += screensaverActionsHtml(true, -1, true);
    out += "</div>";
  } else {
    out += Language::join({"<p class='muted'>", D_WEB_SS_NO_SINGLE, "</p>"}).c_str();
    out += screensaverActionsHtml(true, -1, false);
  }
  out += "</div>";

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

// ============================================================================
//  Route registration.
// ============================================================================
void registerScreensaverRoutes() {
  server.on("/screensavers", HTTP_GET, handleSleepEditorPage);
  server.on("/screensavers/thumb", HTTP_GET, handleSleepThumb);
  server.on("/screensavers/download", HTTP_GET, handleSleepDownload);
  server.on("/screensavers/delete", HTTP_POST, handleSleepDelete);
  server.on("/screensavers/mode", HTTP_POST, handleSleepModePost);
  server.on("/screensavers/upload", HTTP_POST, handleScreensaverUploadDone,
            handleScreensaverUploadStream);
}
