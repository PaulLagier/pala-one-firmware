#include "src/web/screensavers.h"

#include <WiFi.h>   // WiFiClient

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
  File   tmpFile;
  String tmpPath;
  int    slotTarget = -1;
  bool   legacy     = false;
  bool   ok         = false;
  String error;
};

SlotUpload s_up;

}  // namespace

void resetScreensaverUpload() {
  if (s_up.tmpFile) s_up.tmpFile.close();
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
//  POST /screensavers/delete — remove a slot or the legacy single image.
// ============================================================================
static void handleSleepDelete() {
  if (server.hasArg("single")) {
    if (FS.exists("/sleep.bin")) FS.remove("/sleep.bin");
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
    if      (m == "cycle")   Screensavers::setMode(Screensavers::Mode::Cycle);
    else if (m == "shuffle") Screensavers::setMode(Screensavers::Mode::Shuffle);
    else                     Screensavers::setMode(Screensavers::Mode::Single);
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
                s_up.error.length() ? s_up.error : "Upload failed");
    return;
  }
  server.sendHeader("Location", "/screensavers");
  server.send(302, "text/plain", "");
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
    if (s_up.error.length() > 0) return;
    if (s_up.tmpFile && up.currentSize > 0) {
      if (s_up.tmpFile.size() + up.currentSize > 8192) {
        s_up.tmpFile.close();
        if (FS.exists(s_up.tmpPath)) FS.remove(s_up.tmpPath);
        s_up.error = "Image file is too large";
        return;
      }
      size_t wrote = s_up.tmpFile.write(up.buf, up.currentSize);
      if (wrote != up.currentSize) {
        s_up.tmpFile.close();
        if (FS.exists(s_up.tmpPath)) FS.remove(s_up.tmpPath);
        s_up.error = "Write failed (disk full?)";
        return;
      }
    }
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

// ============================================================================
//  Editor HTML + JS. PROGMEM strings keep the heavy strings out of RAM.
//  See Kevin's fork (Pala_One_2_1_kevinst1r/Pala_One_2_1_kevinst1r.ino,
//  ~line 4723) for the original; the JS is a faithful port with the upload
//  endpoint rewired to /screensavers/upload.
// ============================================================================

static const char kEditorStyle[] PROGMEM =
  "<style>"
  ".ss-wrap{display:grid;gap:12px}"
  ".ss-card{border:1px solid var(--line-soft);border-radius:12px;padding:10px 11px;background:var(--stat-bg)}"
  ".ss-grid{display:grid;gap:10px;grid-template-columns:repeat(2,minmax(0,1fr))}"
  ".ss-grid .full{grid-column:1/-1}"
  ".ss-grid > div{min-width:0}"
  ".ss-adv{border:1px solid var(--line);border-radius:10px;padding:8px 10px;background:var(--card)}"
  ".ss-adv summary{cursor:pointer;font-weight:600;list-style:none}"
  ".ss-adv summary::-webkit-details-marker{display:none}"
  ".ss-adv summary:after{content:'\\25BE';float:right;color:var(--muted)}"
  ".ss-adv[open] summary:after{content:'\\25B4'}"
  ".ss-adv-body{margin-top:10px}"
  ".ss-label-row{display:flex;justify-content:space-between;align-items:center;gap:8px;margin:0 0 6px}"
  ".ss-value{font-size:12px;color:var(--muted);white-space:nowrap}"
  ".ss-preview-wrap{display:flex;flex-direction:column;gap:8px;min-height:240px}"
  ".ss-preview-stage{flex:1;display:flex;align-items:center;justify-content:center;min-height:140px}"
  ".ss-preview-stage canvas{width:100%;max-width:520px;border:1px solid var(--line);border-radius:10px;background:#fff;image-rendering:pixelated;touch-action:none}"
  ".ss-meta,.ss-status{font-size:12px;color:var(--muted)}"
  ".ss-slots{display:grid;grid-template-columns:repeat(auto-fill,minmax(120px,1fr));gap:10px}"
  ".ss-slot{border:1px solid var(--line-soft);border-radius:10px;padding:8px;background:var(--stat-bg);display:flex;flex-direction:column;gap:6px;align-items:center}"
  ".ss-slot-actions{display:flex;gap:8px;align-items:center;justify-content:center;width:100%;margin-top:4px}"
  ".btn-icon{display:inline-flex;align-items:center;justify-content:center;width:36px;height:36px;margin:0;border:1px solid var(--line);border-radius:10px;background:var(--card);color:inherit;text-decoration:none;cursor:pointer;transition:background .2s ease}"
  ".btn-icon svg{width:18px;height:18px;stroke:currentColor;fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}"
  ".btn-icon:hover{background:var(--line-soft); }"
  ".btn-icon.danger{color:var(--danger);border-color:var(--danger)}"
  ".ss-slot img{width:100%;height:auto;border:1px solid var(--line);border-radius:6px;background:#fff;image-rendering:pixelated}"
  ".ss-slot .ss-slot-empty{width:100%;aspect-ratio:250/122;display:flex;align-items:center;justify-content:center;border:1px dashed var(--line);border-radius:6px;color:var(--muted);font-size:12px}"
  "@media(max-width:560px){.ss-grid{grid-template-columns:1fr}}"
  "</style>";

static const char kIconDownload[] PROGMEM =
  "<svg viewBox='0 0 24 24' aria-hidden='true'><path d='M12 4v10m0 0l4-4m-4 4l-4-4M5 20h14'/></svg>";
static const char kIconTrash[] PROGMEM =
  "<svg viewBox='0 0 24 24' aria-hidden='true'>"
  "<path d='M4 7h16'/>"
  "<path d='M8 7V5h8v2'/>"
  "<path d='M7 7v12h10V7'/>"
  "<path d='M10 11v5M14 11v5'/>"
  "</svg>";

static const char kEditorScript[] PROGMEM =
  "<script>(function(){"
  "if(window.__palaSleepEditorInit)return;"
  "window.__palaSleepEditorInit=1;"
  "var W=250,H=122,ROW=32,TOTAL=H*ROW;"
  "var fileInput=document.getElementById('ssEditFile');"
  "var tol=document.getElementById('ssTolerance');"
  "var tolLbl=document.getElementById('ssToleranceLabel');"
  "var zoom=document.getElementById('ssZoom');"
  "var zoomLbl=document.getElementById('ssZoomLabel');"
  "var panX=document.getElementById('ssPanX');"
  "var panY=document.getElementById('ssPanY');"
  "var panXLbl=document.getElementById('ssPanXLabel');"
  "var panYLbl=document.getElementById('ssPanYLabel');"
  "var inv=document.getElementById('ssInvert');"
  "var canvas=document.getElementById('ssPreview');"
  "var meta=document.getElementById('ssMeta');"
  "var status=document.getElementById('ssUploadStatus');"
  "var resetBtn=document.getElementById('ssResetBtn');"
  "var uploadBtn=document.getElementById('ssUploadBtn');"
  "var dstSel=document.getElementById('ssDestination');"
  "if(!fileInput||!tol||!zoom||!panX||!panY||!inv||!canvas||!meta||!status||!resetBtn||!uploadBtn)return;"
  "var ctx=canvas.getContext('2d',{willReadFrequently:true});"
  "var work=document.createElement('canvas');work.width=W;work.height=H;"
  "var workCtx=work.getContext('2d',{willReadFrequently:true});"
  "var sourceImage=null;"
  "var isDragging=false,dragStartX=0,dragStartY=0,dragPanX=0,dragPanY=0;"
  "var pointers={};"
  "var pinch={active:false,startDist:0,startZoom:100,startPanX:0,startPanY:0,startMidX:0,startMidY:0};"
  "function clamp(v,a,b){return v<a?a:(v>b?b:v)}"
  "function thr(o){o=clamp(parseInt(o||0,10)||0,-100,100);return clamp(128+Math.round(o*(255-128)/100),0,255)}"
  "function setLbls(){var t=parseInt(tol.value,10)||0;tolLbl.textContent=(t>0?'+':'')+t+'%';zoomLbl.textContent=zoom.value+'%';panXLbl.textContent=panX.value+' px';panYLbl.textContent=panY.value+' px';}"
  "function fit(){if(!sourceImage)return;zoom.value='100';panX.value='0';panY.value='0';setLbls();render();}"
  "function drawToWork(){workCtx.fillStyle='#fff';workCtx.fillRect(0,0,W,H);if(!sourceImage)return;var base=Math.min(W/sourceImage.width,H/sourceImage.height);var s=base*((parseInt(zoom.value,10)||100)/100);if(!isFinite(s)||s<=0)s=base;var dw=Math.max(1,Math.round(sourceImage.width*s));var dh=Math.max(1,Math.round(sourceImage.height*s));var x=((W-dw)/2)+(parseInt(panX.value,10)||0);var y=((H-dh)/2)+(parseInt(panY.value,10)||0);workCtx.drawImage(sourceImage,x,y,dw,dh);}"
  "function toOneBit(){var img=workCtx.getImageData(0,0,W,H),d=img.data,t=thr(tol.value),iv=!!inv.checked;for(var i=0;i<d.length;i+=4){var L=((d[i]*299)+(d[i+1]*587)+(d[i+2]*114))/1000;var w=(L>=t);if(iv)w=!w;var c=w?255:0;d[i]=c;d[i+1]=c;d[i+2]=c;d[i+3]=255;}ctx.putImageData(img,0,0);return img;}"
  "function pack(img){var d=img.data,o=new Uint8Array(TOTAL);for(var y=0;y<H;y++)for(var x=0;x<W;x++){var i=(y*W+x)*4;if(d[i]>=128){var b=(y*ROW)+(x>>3);o[b]=o[b]|(1<<(x&7));}}return o;}"
  "function render(){setLbls();drawToWork();var i=toOneBit();if(!sourceImage){meta.textContent='No image loaded';return null;}meta.textContent='Preview: '+W+'x'+H+'  threshold '+thr(tol.value)+'  bytes '+TOTAL;return i;}"
  "function pts(){var a=[];for(var k in pointers){a.push(pointers[k]);}return a;}"
  "function dist(a,b){var dx=a.x-b.x,dy=a.y-b.y;return Math.sqrt(dx*dx+dy*dy);}"
  "function mid(a,b){return{x:(a.x+b.x)/2,y:(a.y+b.y)/2};}"
  "function startPinch(){var p=pts();if(p.length===2){pinch.active=true;pinch.startDist=Math.max(8,dist(p[0],p[1]));pinch.startZoom=parseInt(zoom.value,10)||100;pinch.startPanX=parseInt(panX.value,10)||0;pinch.startPanY=parseInt(panY.value,10)||0;var m=mid(p[0],p[1]);pinch.startMidX=m.x;pinch.startMidY=m.y;}else pinch.active=false;}"
  "fileInput.addEventListener('change',function(){var f=fileInput.files&&fileInput.files[0];if(!f){sourceImage=null;render();return;}status.textContent='';var r=new FileReader();r.onload=function(){var im=new Image();im.onload=function(){sourceImage=im;fit();};im.onerror=function(){status.textContent='Could not decode image.';};im.src=r.result;};r.onerror=function(){status.textContent='Could not read image.';};r.readAsDataURL(f);});"
  "[tol,zoom,panX,panY,inv].forEach(function(el){el.addEventListener('input',render);el.addEventListener('change',render);});"
  "resetBtn.addEventListener('click',function(){fit();status.textContent='';});"
  "canvas.addEventListener('pointerdown',function(e){pointers[e.pointerId]={x:e.clientX,y:e.clientY};canvas.setPointerCapture(e.pointerId);var p=pts();if(p.length===1){isDragging=true;dragStartX=e.clientX;dragStartY=e.clientY;dragPanX=parseInt(panX.value,10)||0;dragPanY=parseInt(panY.value,10)||0;}startPinch();});"
  "canvas.addEventListener('pointermove',function(e){if(!pointers[e.pointerId])return;pointers[e.pointerId].x=e.clientX;pointers[e.pointerId].y=e.clientY;var p=pts();if(pinch.active&&p.length===2){var d=Math.max(8,dist(p[0],p[1])),r=d/pinch.startDist;zoom.value=String(clamp(Math.round(pinch.startZoom*r),10,400));var m=mid(p[0],p[1]);panX.value=String(clamp(pinch.startPanX+Math.round(m.x-pinch.startMidX),-250,250));panY.value=String(clamp(pinch.startPanY+Math.round(m.y-pinch.startMidY),-180,180));render();return;}if(isDragging&&p.length===1){panX.value=String(clamp(dragPanX+Math.round(e.clientX-dragStartX),-250,250));panY.value=String(clamp(dragPanY+Math.round(e.clientY-dragStartY),-180,180));render();}});"
  "function endP(e){delete pointers[e.pointerId];var p=pts();if(p.length===1){isDragging=true;dragStartX=p[0].x;dragStartY=p[0].y;dragPanX=parseInt(panX.value,10)||0;dragPanY=parseInt(panY.value,10)||0;}else isDragging=false;startPinch();}"
  "canvas.addEventListener('pointerup',endP);canvas.addEventListener('pointercancel',endP);"
  "canvas.addEventListener('wheel',function(e){if(!sourceImage)return;e.preventDefault();var z=parseInt(zoom.value,10)||100,s=Math.max(2,Math.round(Math.abs(e.deltaY)/25));zoom.value=String(clamp(z-(e.deltaY>0?s:-s),10,400));render();},{passive:false});"
  "uploadBtn.addEventListener('click',function(){if(!sourceImage){alert('Please choose an image first.');return;}var img=render();if(!img){status.textContent='Preview is not ready yet.';return;}var bytes=pack(img);var fd=new FormData();fd.append('file',new Blob([bytes],{type:'application/octet-stream'}),'sleep-editor.bin');var url='/screensavers/upload';var sel=dstSel?dstSel.value:'auto';if(sel==='single')url+='?single=1';else if(sel!=='auto')url+='?slot='+encodeURIComponent(sel);status.textContent='Uploading...';uploadBtn.disabled=true;fetch(url,{method:'POST',body:fd}).then(function(r){if(!r.ok)return r.text().then(function(t){throw new Error(t||('HTTP '+r.status));});status.textContent='Upload complete. Refreshing...';setTimeout(function(){window.location.href='/screensavers';},600);}).catch(function(e){status.textContent='Upload failed: '+(e&&e.message?e.message:'error');}).finally(function(){uploadBtn.disabled=false;});});"
  "setLbls();render();"
  "})();</script>";

// Download + delete icon buttons for a populated slot or the legacy single image.
static String screensaverActionsHtml(bool single, int slot) {
  String out;
  out.reserve(420);
  out += "<div class='ss-slot-actions'>";
  out += "<a class='btn-icon' href='/screensavers/download?";
  if (single) out += "single=1";
  else out += "slot=" + String(slot);
  out += "' download title='" D_WEB_SS_DOWNLOAD_ARIA "' aria-label='" D_WEB_SS_DOWNLOAD_ARIA "'>";
  out += FPSTR(kIconDownload);
  out += "</a>";
  out += "<form method='POST' action='/screensavers/delete' style='margin:0'>";
  if (single) {
    out += "<input type='hidden' name='single' value='1'>";
    out += "<button type='submit' class='btn-icon danger' title='" D_WEB_SS_DELETE_ARIA "' "
           "aria-label='" D_WEB_SS_DELETE_ARIA "' "
           "onclick=\"return confirm('" D_WEB_SS_CONFIRM_DEL_SINGLE "')\">";
  } else {
    out += "<input type='hidden' name='slot' value='" + String(slot) + "'>";
    out += "<button type='submit' class='btn-icon danger' title='" D_WEB_SS_DELETE_ARIA "' "
           "aria-label='" D_WEB_SS_DELETE_ARIA "' "
           "onclick=\"return confirm('" D_WEB_SS_CONFIRM_DEL_SLOT "')\">";
  }
  out += FPSTR(kIconTrash);
  out += "</button></form></div>";
  return out;
}

// Build the slot grid HTML — one card per slot with thumbnail (when
// populated) and a delete form. Appended into a containing card by the
// editor handler.
static String slotGridHtml() {
  String out;
  out.reserve(2000);
  out += "<div class='ss-slots'>";
  for (int i = 0; i < Screensavers::MAX_SLOTS; i++) {
    out += "<div class='ss-slot'><div class='muted small'>" D_WEB_SS_SLOT_LABEL " ";
    out += String(i);
    out += "</div>";
    if (Screensavers::slotExists(i)) {
      out += "<img src='/screensavers/thumb?slot=" + String(i) +
             "' alt='" D_WEB_SS_SLOT_LABEL " " + String(i) + "'>";
      out += screensaverActionsHtml(false, i);
    } else {
      out += "<div class='ss-slot-empty'>" D_WEB_SS_SLOT_EMPTY "</div>";
    }
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
  out += FPSTR(kEditorStyle);
  out += "<div class='card'><h2>" D_WEB_SS_EDITOR_HEADING "</h2>"
         "<p class='muted'>" D_WEB_SS_EDITOR_INTRO "</p>"
         "<div class='ss-wrap'>"
         "<div class='ss-card ss-grid'>"
         "<div class='full'><div class='ss-label-row'><label for='ssEditFile'>" D_WEB_SS_SOURCE_IMAGE "</label></div>"
         "<input id='ssEditFile' type='file' accept='image/*'></div>"
         "<div class='full'><div class='ss-label-row'><label for='ssTolerance'>" D_WEB_SS_TOLERANCE "</label>"
         "<span class='ss-value' id='ssToleranceLabel'>0%</span></div>"
         "<input id='ssTolerance' type='range' min='-100' max='100' value='0'></div>"
         "<div class='full'><label style='display:flex;gap:10px;align-items:center;font-weight:500'>"
         "<input id='ssInvert' type='checkbox'><span>" D_WEB_SS_INVERT "</span></label></div>"
         "<div class='full'><details class='ss-adv'><summary>" D_WEB_SS_PRECISE_CONTROL "</summary><div class='ss-adv-body ss-grid'>"
         "<div><div class='ss-label-row'><label for='ssZoom'>" D_WEB_SS_ZOOM "</label><span class='ss-value' id='ssZoomLabel'>100%</span></div>"
         "<input id='ssZoom' type='range' min='10' max='400' value='100'></div>"
         "<div><div class='ss-label-row'><label for='ssPanX'>" D_WEB_SS_MOVE_X "</label><span class='ss-value' id='ssPanXLabel'>0 px</span></div>"
         "<input id='ssPanX' type='range' min='-250' max='250' value='0'></div>"
         "<div class='full'><div class='ss-label-row'><label for='ssPanY'>" D_WEB_SS_MOVE_Y "</label><span class='ss-value' id='ssPanYLabel'>0 px</span></div>"
         "<input id='ssPanY' type='range' min='-180' max='180' value='0'></div>"
         "</div></details></div>"
         "</div>"
         "<div class='ss-card ss-preview-wrap'>"
         "<label>" D_WEB_SS_PREVIEW_LABEL "</label>"
         "<div class='ss-preview-stage'><canvas id='ssPreview' width='250' height='122'></canvas></div>"
         "<button type='button' class='btn secondary' id='ssResetBtn' style='align-self:flex-start;padding:6px 12px;font-size:13px'>" D_WEB_SS_RESET_FIT "</button>"
         "<div class='ss-meta' id='ssMeta'>" D_WEB_SS_NO_IMAGE "</div>"
         "</div>"
         "<div class='ss-card'>"
         "<div class='ss-label-row'><label for='ssDestination'>" D_WEB_SS_SAVE_TO "</label></div>"
         "<select id='ssDestination'>"
         "<option value='single'>" D_WEB_SS_DST_SINGLE "</option>";
  if (nextFreeSlot >= 0) {
    out += "<option value='auto' selected>" D_WEB_SS_DST_AUTO_PREFIX;
    out += String(nextFreeSlot);
    out += D_WEB_SS_DST_AUTO_SUFFIX "</option>";
  } else {
    out += "<option value='auto' disabled>" D_WEB_SS_DST_FULL "</option>";
  }
  for (int i = 0; i < Screensavers::MAX_SLOTS; i++) {
    out += "<option value='" + String(i) + "'>" D_WEB_SS_DST_SLOT_PREFIX + String(i);
    if (Screensavers::slotExists(i)) out += D_WEB_SS_DST_OVERWRITE;
    out += "</option>";
  }
  out += "</select></div>"
         "<div class='actions'>"
         "<button type='button' class='btn' id='ssUploadBtn'>" D_WEB_SS_UPLOAD_EDITED "</button>"
         "<span class='ss-status' id='ssUploadStatus'></span>"
         "</div></div></div>";
  out += FPSTR(kEditorScript);
  return out;
}

static void handleSleepEditorPage() {
  Screensavers::Mode m = Screensavers::currentMode();
  const char* selSingle  = (m == Screensavers::Mode::Single)  ? " selected" : "";
  const char* selCycle   = (m == Screensavers::Mode::Cycle)   ? " selected" : "";
  const char* selShuffle = (m == Screensavers::Mode::Shuffle) ? " selected" : "";

  int populated = Screensavers::populatedCount();
  bool hasLegacy = FS.exists("/sleep.bin");
  int nextFree = Screensavers::firstFreeSlot();

  String out = webPageStart(
    D_WEB_SS_TITLE,
    D_WEB_SS_SUBTITLE,
    "<a href='/'>" D_WEB_SETTINGS_BACK_NAV "</a><a href='/settings'>" D_WEB_NAV_SETTINGS "</a>",
    true
  );
  out.reserve(out.length() + 8000);

  // Editor.
  out += editorCardHtml(nextFree);

  // Mode picker.
  out += "<div class='card'><h2>" D_WEB_SS_ROTATION_HEADING "</h2>"
         "<p class='muted'>" D_WEB_SS_ROTATION_INTRO "</p>"
         "<form method='POST' action='/screensavers/mode' class='stack' style='margin-top:12px'>"
         "<div class='grid cols-2'>"
         "<div><label for='ssMode'>" D_WEB_SS_MODE_LABEL "</label><select id='ssMode' name='mode'>"
         "<option value='single'";  out += selSingle;  out += ">" D_WEB_SS_MODE_SINGLE  "</option>"
         "<option value='cycle'";   out += selCycle;   out += ">" D_WEB_SS_MODE_CYCLE   "</option>"
         "<option value='shuffle'"; out += selShuffle; out += ">" D_WEB_SS_MODE_SHUFFLE "</option>"
         "</select><div class='hint'>" D_WEB_SS_SLOTS_POPULATED;
  out += String(populated);
  out += "/";
  out += String(Screensavers::MAX_SLOTS);
  out += "</div></div></div>"
         "<div class='actions'><button type='submit'>" D_WEB_SS_SAVE_MODE "</button></div>"
         "</form></div>";

  // Slot grid.
  out += "<div class='card'><h2>" D_WEB_SS_SLOTS_HEADING "</h2>";
  out += slotGridHtml();
  out += "</div>";

  // Legacy single image card.
  out += "<div class='card'><h2>" D_WEB_SS_SINGLE_HEADING "</h2>";
  if (hasLegacy) {
    out += "<div class='row' style='align-items:center;gap:12px'>"
           "<img src='/screensavers/thumb?single=1' alt='" D_WEB_SS_SINGLE_ALT "' "
           "style='width:180px;border:1px solid var(--line);border-radius:8px;background:#fff;image-rendering:pixelated'>"
           "<form method='POST' action='/screensavers/delete'>"
           "<input type='hidden' name='single' value='1'>"
           "<button type='submit' class='btn secondary' "
           "onclick=\"return confirm('" D_WEB_SS_CONFIRM_DEL_SINGLE "')\">" D_WEB_DELETE_BUTTON "</button>"
           "</form></div>";
  } else {
    out += "<p class='muted'>" D_WEB_SS_NO_SINGLE "</p>";
  }
  out += "</div>";

  out += webPageEnd();
  server.send(200, "text/html; charset=utf-8", out);
}

// ============================================================================
//  Route registration.
// ============================================================================
void registerScreensaverRoutes() {
  server.on("/screensavers",         HTTP_GET,  handleSleepEditorPage);
  server.on("/screensavers/thumb",   HTTP_GET,  handleSleepThumb);
  server.on("/screensavers/download", HTTP_GET,  handleSleepDownload);
  server.on("/screensavers/delete",  HTTP_POST, handleSleepDelete);
  server.on("/screensavers/mode",    HTTP_POST, handleSleepModePost);
  server.on("/screensavers/upload",  HTTP_POST,
            handleScreensaverUploadDone, handleScreensaverUploadStream);
}
