// Screensavers screen — bitmap editor + rotation manager.
//
// Four sections from top to bottom:
//   1. Bitmap editor (file picker, sliders, 250x122 preview, destination, upload).
//   2. Rotation mode (single / cycle / shuffle) + populated count.
//   3. Slot grid (8 cards with thumbnails, download + delete buttons).
//   4. Single image (/sleep.bin), the legacy "one screensaver" slot.
//
// The editor's "1-bit pack" pipeline: load image, draw to a 250x122 work
// canvas at user-selected zoom+pan, threshold to 1 bit, and pack to 3904
// bytes. The byte format matches what the device's e-ink driver consumes
// (250x122 px, 1-bit, LSB-first, 32 bytes per row).

(function () {
  // --- Constants tied to the device's e-ink panel + bitmap format. ----------
  var W = 250, H = 122, ROW = 32, TOTAL = H * ROW;   // 3904 bytes total

  // Inline icon SVGs (download arrow / trash) used by the slot grid.
  var ICON_DOWNLOAD =
    "<svg viewBox='0 0 24 24' aria-hidden='true'>" +
      "<path d='M12 4v10m0 0l4-4m-4 4l-4-4M5 20h14'/>" +
    "</svg>";
  var ICON_TRASH =
    "<svg viewBox='0 0 24 24' aria-hidden='true'>" +
      "<path d='M4 7h16'/>" +
      "<path d='M8 7V5h8v2'/>" +
      "<path d='M7 7v12h10V7'/>" +
      "<path d='M10 11v5M14 11v5'/>" +
    "</svg>";

  function esc(s) {
    return String(s == null ? "" : s)
      .replace(/&/g, "&amp;").replace(/</g, "&lt;")
      .replace(/>/g, "&gt;").replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  // Cache-buster appended to thumbnail URLs so a fresh upload doesn't show
  // a stale image. Different per render call.
  function nextBust() { return Date.now().toString(36); }

  // ---------------------------------------------------------------------
  //  HTML builders — pure string assembly, no state.
  // ---------------------------------------------------------------------
  function editorCardHtml(t, state) {
    var ss = t.screensavers;

    // Destination select options:
    //   - "single"        /sleep.bin (single-image legacy slot)
    //   - "auto"          first free slot (label inserts the slot number)
    //   - "0".."MAX-1"    explicit slot (mark with " (overwrite)" when used)
    var dstOpts = '';
    dstOpts += '<option value="single">' + esc(ss.dstSingle) + '</option>';
    if (state.firstFree >= 0) {
      dstOpts += '<option value="auto" selected>' +
                 esc(ss.dstAutoPrefix) + state.firstFree + esc(ss.dstAutoSuffix) +
                 '</option>';
    } else {
      dstOpts += '<option value="auto" disabled>' + esc(ss.dstFull) + '</option>';
    }
    (state.slots || []).forEach(function (s) {
      dstOpts += '<option value="' + s.id + '">' +
                 esc(ss.dstSlotPrefix) + s.id +
                 (s.exists ? esc(ss.dstOverwrite) : '') +
                 '</option>';
    });

    return (
      '<div class="card">' +
        '<h2>' + esc(ss.editorHeading) + '</h2>' +
        '<p class="muted">' + esc(ss.editorIntro) + '</p>' +
        '<div class="ss-wrap">' +

          '<div class="ss-card ss-grid">' +
            '<div class="full">' +
              '<div class="ss-label-row"><label for="ssEditFile">' + esc(ss.sourceImage) + '</label></div>' +
              '<input id="ssEditFile" type="file" accept="image/*">' +
            '</div>' +
            '<div class="full">' +
              '<div class="ss-label-row">' +
                '<label for="ssTolerance">' + esc(ss.tolerance) + '</label>' +
                '<span class="ss-value" id="ssToleranceLabel">0%</span>' +
              '</div>' +
              '<input id="ssTolerance" type="range" min="-100" max="100" value="0">' +
            '</div>' +
            '<div class="full">' +
              '<label class="check-row" style="font-weight:500">' +
                '<input id="ssInvert" type="checkbox">' +
                '<span>' + esc(ss.invert) + '</span>' +
              '</label>' +
            '</div>' +
            '<div class="full">' +
              '<details class="ss-adv">' +
                '<summary>' + esc(ss.preciseControl) + '</summary>' +
                '<div class="ss-adv-body ss-grid">' +
                  '<div>' +
                    '<div class="ss-label-row">' +
                      '<label for="ssZoom">' + esc(ss.zoom) + '</label>' +
                      '<span class="ss-value" id="ssZoomLabel">100%</span>' +
                    '</div>' +
                    '<input id="ssZoom" type="range" min="10" max="400" value="100">' +
                  '</div>' +
                  '<div>' +
                    '<div class="ss-label-row">' +
                      '<label for="ssPanX">' + esc(ss.moveX) + '</label>' +
                      '<span class="ss-value" id="ssPanXLabel">0 px</span>' +
                    '</div>' +
                    '<input id="ssPanX" type="range" min="-250" max="250" value="0">' +
                  '</div>' +
                  '<div class="full">' +
                    '<div class="ss-label-row">' +
                      '<label for="ssPanY">' + esc(ss.moveY) + '</label>' +
                      '<span class="ss-value" id="ssPanYLabel">0 px</span>' +
                    '</div>' +
                    '<input id="ssPanY" type="range" min="-180" max="180" value="0">' +
                  '</div>' +
                '</div>' +
              '</details>' +
            '</div>' +
          '</div>' +

          '<div class="ss-card ss-preview-wrap">' +
            '<label>' + esc(ss.previewLabel) + '</label>' +
            '<div class="ss-preview-stage">' +
              '<canvas id="ssPreview" width="250" height="122"></canvas>' +
            '</div>' +
            '<button type="button" class="btn secondary" id="ssResetBtn" ' +
              'style="align-self:flex-start;padding:6px 12px;font-size:13px">' +
              esc(ss.resetFit) +
            '</button>' +
            '<div class="ss-meta" id="ssMeta">' + esc(ss.noImage) + '</div>' +
          '</div>' +

          '<div class="ss-card">' +
            '<div class="ss-label-row"><label for="ssDestination">' + esc(ss.saveTo) + '</label></div>' +
            '<select id="ssDestination">' + dstOpts + '</select>' +
          '</div>' +

          '<div class="actions">' +
            '<button type="button" class="btn" id="ssUploadBtn">' + esc(ss.uploadEdited) + '</button>' +
            '<span class="ss-status" id="ssUploadStatus"></span>' +
          '</div>' +

        '</div>' +
      '</div>'
    );
  }

  function modeCardHtml(t, state) {
    var ss = t.screensavers;
    function opt(v, label) {
      return '<option value="' + v + '"' + (state.mode === v ? ' selected' : '') + '>' +
             esc(label) + '</option>';
    }
    return (
      '<div class="card">' +
        '<h2>' + esc(ss.rotationHeading) + '</h2>' +
        '<p class="muted">' + esc(ss.rotationIntro) + '</p>' +
        '<form id="ssModeForm" class="stack" style="margin-top:12px">' +
          '<div class="grid cols-2">' +
            '<div>' +
              '<label for="ssMode">' + esc(ss.modeLabel) + '</label>' +
              '<select id="ssMode">' +
                opt('single',  ss.modeSingle) +
                opt('cycle',   ss.modeCycle)  +
                opt('shuffle', ss.modeShuffle) +
              '</select>' +
              '<div class="hint">' + esc(ss.slotsPopulated) + state.populated + '/' + state.max + '</div>' +
            '</div>' +
          '</div>' +
          '<div class="actions">' +
            '<button type="submit">' + esc(ss.saveMode) + '</button>' +
          '</div>' +
        '</form>' +
      '</div>'
    );
  }

  function slotActionsHtml(t, isSingle, slot, bust) {
    var ss = t.screensavers;
    var dlHref = '/screensavers/download?' +
                 (isSingle ? 'single=1' : ('slot=' + slot));
    return (
      '<div class="ss-slot-actions">' +
        '<a class="btn-icon" href="' + dlHref + '" download ' +
          'title="' + esc(ss.downloadAria) + '" aria-label="' + esc(ss.downloadAria) + '">' +
          ICON_DOWNLOAD +
        '</a>' +
        '<button type="button" class="btn-icon danger" ' +
          'data-act="ss-delete"' +
          (isSingle ? ' data-single="1"' : ' data-slot="' + slot + '"') +
          ' title="' + esc(ss.deleteAria) + '" aria-label="' + esc(ss.deleteAria) + '">' +
          ICON_TRASH +
        '</button>' +
      '</div>'
    );
  }

  function slotsCardHtml(t, state, bust) {
    var ss = t.screensavers;
    var html = '<div class="card"><h2>' + esc(ss.slotsHeading) + '</h2><div class="ss-slots">';
    (state.slots || []).forEach(function (s) {
      html += '<div class="ss-slot">';
      html += '<div class="muted small">' + esc(ss.slotLabel) + ' ' + s.id + '</div>';
      if (s.exists) {
        html +=
          '<img src="/screensavers/thumb?slot=' + s.id + '&_=' + bust + '" ' +
               'alt="' + esc(ss.slotLabel) + ' ' + s.id + '">' +
          slotActionsHtml(t, false, s.id, bust);
      } else {
        html += '<div class="ss-slot-empty">' + esc(ss.slotEmpty) + '</div>';
      }
      html += '</div>';
    });
    html += '</div></div>';
    return html;
  }

  function singleCardHtml(t, state, bust) {
    var ss = t.screensavers;
    var html = '<div class="card"><h2>' + esc(ss.singleHeading) + '</h2>';
    if (state.hasSingle) {
      html +=
        '<div class="row" style="align-items:center;gap:12px">' +
          '<img src="/screensavers/thumb?single=1&_=' + bust + '" alt="' + esc(ss.singleAlt) + '" ' +
               'style="width:180px;border:1px solid var(--line);border-radius:8px;background:#fff;image-rendering:pixelated">' +
          '<button type="button" class="btn secondary" data-act="ss-delete" data-single="1">' +
            esc(ss.deleteSingle) +
          '</button>' +
        '</div>';
    } else {
      html += '<p class="muted">' + esc(ss.noSingle) + '</p>';
    }
    html += '</div>';
    return html;
  }

  // ---------------------------------------------------------------------
  //  Editor state + handlers (image -> 1-bit -> upload).
  // ---------------------------------------------------------------------
  function wireEditor(ctx, state) {
    var t = ctx.t, ss = t.screensavers;
    var c = ctx.container;
    var $ = function (id) { return c.querySelector('#' + id); };

    var fileInput = $('ssEditFile');
    var tol       = $('ssTolerance');
    var tolLbl    = $('ssToleranceLabel');
    var zoom      = $('ssZoom');
    var zoomLbl   = $('ssZoomLabel');
    var panX      = $('ssPanX');
    var panY      = $('ssPanY');
    var panXLbl   = $('ssPanXLabel');
    var panYLbl   = $('ssPanYLabel');
    var inv       = $('ssInvert');
    var canvas    = $('ssPreview');
    var meta      = $('ssMeta');
    var status    = $('ssUploadStatus');
    var resetBtn  = $('ssResetBtn');
    var uploadBtn = $('ssUploadBtn');
    var dstSel    = $('ssDestination');

    var cctx = canvas.getContext('2d', { willReadFrequently: true });
    var work = document.createElement('canvas'); work.width = W; work.height = H;
    var wctx = work.getContext('2d', { willReadFrequently: true });

    var sourceImage = null;
    // Single-finger drag for pan, two-finger pinch for zoom.
    var dragging = false, dragStartX = 0, dragStartY = 0, dragPanX = 0, dragPanY = 0;
    var pointers = {};
    var pinch = { active: false, startDist: 0, startZoom: 100,
                  startPanX: 0, startPanY: 0, startMidX: 0, startMidY: 0 };

    function clamp(v, a, b) { return v < a ? a : (v > b ? b : v); }
    function thr(o) {
      o = clamp(parseInt(o || 0, 10) || 0, -100, 100);
      return clamp(128 + Math.round(o * (255 - 128) / 100), 0, 255);
    }
    function setLbls() {
      var v = parseInt(tol.value, 10) || 0;
      tolLbl.textContent  = (v > 0 ? '+' : '') + v + '%';
      zoomLbl.textContent = zoom.value + '%';
      panXLbl.textContent = panX.value + ' px';
      panYLbl.textContent = panY.value + ' px';
    }
    function fit() {
      if (!sourceImage) return;
      zoom.value = '100'; panX.value = '0'; panY.value = '0';
      setLbls(); render();
    }
    function drawToWork() {
      wctx.fillStyle = '#fff';
      wctx.fillRect(0, 0, W, H);
      if (!sourceImage) return;
      var base = Math.min(W / sourceImage.width, H / sourceImage.height);
      var s = base * ((parseInt(zoom.value, 10) || 100) / 100);
      if (!isFinite(s) || s <= 0) s = base;
      var dw = Math.max(1, Math.round(sourceImage.width  * s));
      var dh = Math.max(1, Math.round(sourceImage.height * s));
      var x = ((W - dw) / 2) + (parseInt(panX.value, 10) || 0);
      var y = ((H - dh) / 2) + (parseInt(panY.value, 10) || 0);
      wctx.drawImage(sourceImage, x, y, dw, dh);
    }
    function toOneBit() {
      var img = wctx.getImageData(0, 0, W, H);
      var d = img.data, t_ = thr(tol.value), iv = !!inv.checked;
      for (var i = 0; i < d.length; i += 4) {
        var L = ((d[i] * 299) + (d[i + 1] * 587) + (d[i + 2] * 114)) / 1000;
        var w = L >= t_;
        if (iv) w = !w;
        var c2 = w ? 255 : 0;
        d[i] = d[i + 1] = d[i + 2] = c2;
        d[i + 3] = 255;
      }
      cctx.putImageData(img, 0, 0);
      return img;
    }
    function pack(img) {
      var d = img.data;
      var o = new Uint8Array(TOTAL);
      for (var y = 0; y < H; y++) {
        for (var x = 0; x < W; x++) {
          var i = (y * W + x) * 4;
          if (d[i] >= 128) {
            var b = (y * ROW) + (x >> 3);
            o[b] = o[b] | (1 << (x & 7));
          }
        }
      }
      return o;
    }
    function render() {
      setLbls();
      drawToWork();
      var i = toOneBit();
      if (!sourceImage) { meta.textContent = ss.noImage; return null; }
      meta.textContent = ss.previewMeta
        .replace('{w}', W).replace('{h}', H)
        .replace('{threshold}', thr(tol.value))
        .replace('{bytes}', TOTAL);
      return i;
    }
    function pts() {
      var a = [];
      for (var k in pointers) a.push(pointers[k]);
      return a;
    }
    function dist(a, b) { var dx = a.x - b.x, dy = a.y - b.y; return Math.sqrt(dx * dx + dy * dy); }
    function mid(a, b)  { return { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 }; }
    function startPinch() {
      var p = pts();
      if (p.length === 2) {
        pinch.active    = true;
        pinch.startDist = Math.max(8, dist(p[0], p[1]));
        pinch.startZoom = parseInt(zoom.value, 10) || 100;
        pinch.startPanX = parseInt(panX.value, 10) || 0;
        pinch.startPanY = parseInt(panY.value, 10) || 0;
        var m = mid(p[0], p[1]);
        pinch.startMidX = m.x; pinch.startMidY = m.y;
      } else {
        pinch.active = false;
      }
    }

    fileInput.addEventListener('change', function () {
      var f = fileInput.files && fileInput.files[0];
      if (!f) { sourceImage = null; render(); return; }
      status.textContent = '';
      var r = new FileReader();
      r.onload  = function () {
        var im = new Image();
        im.onload  = function () { sourceImage = im; fit(); };
        im.onerror = function () { status.textContent = ss.errImageDecode; };
        im.src = r.result;
      };
      r.onerror = function () { status.textContent = ss.errImageRead; };
      r.readAsDataURL(f);
    });
    [tol, zoom, panX, panY, inv].forEach(function (el) {
      el.addEventListener('input',  render);
      el.addEventListener('change', render);
    });
    resetBtn.addEventListener('click', function () { fit(); status.textContent = ''; });

    canvas.addEventListener('pointerdown', function (e) {
      pointers[e.pointerId] = { x: e.clientX, y: e.clientY };
      canvas.setPointerCapture(e.pointerId);
      var p = pts();
      if (p.length === 1) {
        dragging = true;
        dragStartX = e.clientX; dragStartY = e.clientY;
        dragPanX = parseInt(panX.value, 10) || 0;
        dragPanY = parseInt(panY.value, 10) || 0;
      }
      startPinch();
    });
    canvas.addEventListener('pointermove', function (e) {
      if (!pointers[e.pointerId]) return;
      pointers[e.pointerId].x = e.clientX;
      pointers[e.pointerId].y = e.clientY;
      var p = pts();
      if (pinch.active && p.length === 2) {
        var d = Math.max(8, dist(p[0], p[1]));
        var r = d / pinch.startDist;
        zoom.value = String(clamp(Math.round(pinch.startZoom * r), 10, 400));
        var m = mid(p[0], p[1]);
        panX.value = String(clamp(pinch.startPanX + Math.round(m.x - pinch.startMidX), -250, 250));
        panY.value = String(clamp(pinch.startPanY + Math.round(m.y - pinch.startMidY), -180, 180));
        render();
        return;
      }
      if (dragging && p.length === 1) {
        panX.value = String(clamp(dragPanX + Math.round(e.clientX - dragStartX), -250, 250));
        panY.value = String(clamp(dragPanY + Math.round(e.clientY - dragStartY), -180, 180));
        render();
      }
    });
    function endP(e) {
      delete pointers[e.pointerId];
      var p = pts();
      if (p.length === 1) {
        dragging = true;
        dragStartX = p[0].x; dragStartY = p[0].y;
        dragPanX = parseInt(panX.value, 10) || 0;
        dragPanY = parseInt(panY.value, 10) || 0;
      } else {
        dragging = false;
      }
      startPinch();
    }
    canvas.addEventListener('pointerup',     endP);
    canvas.addEventListener('pointercancel', endP);
    canvas.addEventListener('wheel', function (e) {
      if (!sourceImage) return;
      e.preventDefault();
      var z = parseInt(zoom.value, 10) || 100;
      var step = Math.max(2, Math.round(Math.abs(e.deltaY) / 25));
      zoom.value = String(clamp(z - (e.deltaY > 0 ? step : -step), 10, 400));
      render();
    }, { passive: false });

    uploadBtn.addEventListener('click', async function () {
      if (!sourceImage) { window.alert(ss.errNoImage); return; }
      var img = render();
      if (!img) { status.textContent = ss.errPreviewNotReady; return; }
      var bytes = pack(img);
      var fd = new FormData();
      fd.append('file', new Blob([bytes], { type: 'application/octet-stream' }),
                       'sleep-editor.bin');
      var url = '/screensavers/upload';
      var sel = dstSel ? dstSel.value : 'auto';
      if (sel === 'single')      url += '?single=1';
      else if (sel !== 'auto')   url += '?slot=' + encodeURIComponent(sel);
      status.textContent = ss.uploading;
      uploadBtn.disabled = true;
      try {
        var r = await fetch(url, { method: 'POST', body: fd });
        if (!r.ok) {
          var msg = '';
          try { msg = await r.text(); } catch (_) {}
          throw new Error(msg || ('HTTP ' + r.status));
        }
        status.textContent = ss.uploadOk;
        // Re-fetch state + redraw so the new thumbnail appears.
        await load(ctx);
      } catch (e) {
        status.textContent = ss.uploadFailed + ': ' + (e.message || e);
      } finally {
        uploadBtn.disabled = false;
      }
    });

    setLbls();
    render();
  }

  // ---------------------------------------------------------------------
  //  Mode + delete handlers (delegated at the container).
  // ---------------------------------------------------------------------
  function wireModeAndDeletes(ctx) {
    var t = ctx.t, ss = t.screensavers;
    var c = ctx.container;

    var modeForm = c.querySelector('#ssModeForm');
    if (modeForm) {
      modeForm.addEventListener('submit', async function (ev) {
        ev.preventDefault();
        var val = c.querySelector('#ssMode').value;
        try {
          await window.palaApi.post('/api/screensavers/mode', { mode: val });
          await load(ctx);
        } catch (e) {
          window.alert((t.errors.server || 'Server error') + ': ' + (e.message || e));
        }
      });
    }

    c.addEventListener('click', async function (ev) {
      var btn = ev.target.closest('[data-act="ss-delete"]');
      if (!btn) return;
      var single = btn.dataset.single === '1';
      var msg    = single ? ss.confirmDeleteSingle : ss.confirmDeleteSlot;
      if (!window.confirm(msg)) return;
      var body   = single ? { single: true }
                          : { slot: parseInt(btn.dataset.slot, 10) };
      try {
        await window.palaApi.post('/api/screensavers/delete', body);
        await load(ctx);
      } catch (e) {
        window.alert((t.errors.server || 'Server error') + ': ' + (e.message || e));
      }
    });
  }

  // ---------------------------------------------------------------------
  //  Top-level render: fetch state, paint, wire.
  // ---------------------------------------------------------------------
  async function load(ctx) {
    var t = ctx.t;
    ctx.header({
      title:    t.screensavers.title,
      subtitle: t.screensavers.subtitle,
      navKey:   "screensavers"
    });
    ctx.container.innerHTML =
      '<div class="card"><p class="muted">' + esc(t.screensavers.loading) + '</p></div>';

    var state;
    try {
      state = await window.palaApi.get('/api/screensavers');
    } catch (e) {
      ctx.container.innerHTML =
        '<div class="banner-warn">' + esc(t.errors.server) + ': ' +
        esc(e.message || e) + '</div>';
      return;
    }

    var bust = nextBust();
    ctx.container.innerHTML =
      editorCardHtml(t, state) +
      modeCardHtml(t, state) +
      slotsCardHtml(t, state, bust) +
      singleCardHtml(t, state, bust);

    wireEditor(ctx, state);
    wireModeAndDeletes(ctx);
  }

  function render(ctx) { load(ctx); }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.screensavers = { render: render };
})();
