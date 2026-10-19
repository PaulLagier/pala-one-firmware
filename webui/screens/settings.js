// Settings screen — GET /api/settings, edit form, POST full state.
//
// Reading-related settings (font size/family/line-gap/bionic/paragraph
// gaps) can shift the
// active reader's page table; the backend handles cursor re-location, the
// SPA just sends the new state and trusts the device. The response carries
// the resulting state so we can refresh the form without a second GET.

(function () {
  var html = window.palaHtml.html, raw = window.palaHtml.raw;

  // Static option lists for the selects — values are the wire shape the
  // backend expects, labels come from i18n at render time.
  var FONT_VALUES    = [8, 10, 12, 14];
  var SLEEP_VALUES   = [30, 60, 120, 300, 600, 1800];
  var LGAP_VALUES    = [0, 1, 2, 3];
  var FAMILY_VALUES  = ["helv", "dys"];
  // Gesture-bindable actions. Order matches the legacy UI; "none" first so
  // users can blank out a binding from the keyboard with one arrow press.
  var ACTION_VALUES  = ["none", "next", "prev", "menu", "lock", "home", "bookmark", "rotate"];
  // Remappable gestures: [select id, wire key, i18n label key].
  var GESTURES = [
    ["btnS",  "short",     "buttonsShort"],
    ["btnD",  "double",    "buttonsDouble"],
    ["btnT",  "triple",    "buttonsTriple"],
    ["btnL",  "long",      "buttonsLong"],
    ["btnXL", "extraLong", "buttonsExtraLong"],
    ["btnCH", "clickHold", "buttonsClickHold"]
  ];
  // Without these the device can't be navigated; the backend enforces the
  // same rule.
  var REQUIRED_ACTIONS = ["next", "home", "menu"];
  // System entries the library menu can show, in dropdown order. Wire names
  // match the backend; "" means the slot is hidden.
  var LIBRARY_ENTRIES = ["bookmarks", "list", "apps", "statistics", "device", "upload", "update", "settings"];

  function selectHtml(id, name, options, currentValue, hint) {
    var opts = options.map(function (o) {
      var sel = (String(o.value) === String(currentValue)) ? raw(" selected") : "";
      return html`<option value="${o.value}"${sel}>${o.label}</option>`;
    });
    return html`<div>
      <label for="${id}">${name}</label>
      <select id="${id}">${opts}</select>
      <div class="hint">${hint}</div>
    </div>`;
  }

  function renderForm(ctx, state) {
    var t   = ctx.t;
    var s   = t.settings;
    var fontOpts   = FONT_VALUES.map(function (v)  { return { value: v, label: s.fontSizes[v]   }; });
    var sleepOpts  = SLEEP_VALUES.map(function (v) { return { value: v, label: s.sleepTimes[v]  }; });
    var lgapOpts   = LGAP_VALUES.map(function (v)  { return { value: v, label: s.lineGaps[v]    }; });
    var familyOpts = FAMILY_VALUES.map(function (v){ return { value: v, label: s.families[v]    }; });

    // Cards top to bottom: reading settings, gesture bindings (own form so a
    // hold can be rebound without touching the sliders), sleep-image
    // management (hidden when no custom image), and a link to the editor.
    ctx.container.innerHTML = html`<div class="card">
      <h2>${s.readingHeading}</h2>
      <p class="muted">${s.readingIntro}</p>
      <form id="settings-form" style="margin-top:12px">
        <div class="grid cols-2">
          ${selectHtml("font",   s.fontSize,    fontOpts,   state.font,   s.fontSizeHint)}
          ${selectHtml("family", s.fontFamily,  familyOpts, state.family, s.familyHint)}
          ${selectHtml("sleep",  s.sleepAfter,  sleepOpts,  state.sleep,  s.sleepHint)}
          ${selectHtml("lgap",   s.lineSpacing, lgapOpts,   state.lgap,   s.lineSpacingHint)}
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="bionic"${state.bionic ? raw(" checked") : ""}>
              <span>${s.bionicLabel}</span>
            </label>
            <div class="hint">${s.bionicHint}</div>
          </div>
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="halfParagraphGaps"${state.halfParagraphGaps ? raw(" checked") : ""}>
              <span>${s.paraGapLabel}</span>
            </label>
            <div class="hint">${s.paraGapHint}</div>
          </div>
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="noScreensaver"${state.noScreensaver ? raw(" checked") : ""}>
              <span>${s.noScreensaverLabel}</span>
            </label>
            <div class="hint">${s.noScreensaverHint}</div>
          </div>
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="lockOnSleep"${state.lockOnSleep ? raw(" checked") : ""}>
              <span>${s.lockOnSleepLabel}</span>
            </label>
            <div class="hint">${s.lockOnSleepHint}</div>
          </div>
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="screenRotation"${state.screenRotation ? raw(" checked") : ""}>
              <span>${s.screenRotationLabel}</span>
            </label>
            <div class="hint">${s.screenRotationHint}</div>
          </div>
        </div>
        <div class="actions">
          <button type="submit" id="save">${s.save}</button>
          <span class="muted">${s.applyHint}</span>
        </div>
        <div id="settings-status"></div>
      </form>
    </div>
    ${iconsCardHtml(s, state)}
    ${gesturesCardHtml(s, state.gestures || {})}
    ${timingsCardHtml(s, state.timings || [])}
    ${wifiCardHtml(s, state.wifi || {})}
    ${libraryCardHtml(s, state.headerTitle || "", state.headerTitleDefault || "")}
    ${libraryMenuCardHtml(s, state.libraryMenu || [], state.libraryMenuSlots || LIBRARY_ENTRIES.length)}
    <div class="card" id="sleep-image-card"${state.hasSleepImage ? "" : raw(" hidden")}>
      <h2>${s.sleepImageHeading}</h2>
      <p class="muted">${s.sleepImagePresent}</p>
      <div class="actions">
        <button type="button" class="btn secondary" id="delete-sleep">${s.deleteSleepImage}</button>
      </div>
    </div>
    <div class="card">
      <h2>${s.screensaverHeading}</h2>
      <p class="muted">${s.screensaverCardDesc}</p>
      <div class="actions">
        <a class="btn" href="#/screensavers">${s.screensaverEditorLink}</a>
        <span class="muted">${s.screensaverEditorHint}</span>
      </div>
    </div>`;

    var form = ctx.container.querySelector("#settings-form");
    form.addEventListener("submit", function (ev) {
      ev.preventDefault();
      save(ctx);
    });
    var gForm = ctx.container.querySelector("#gestures-form");
    if (gForm) {
      gForm.addEventListener("submit", function (ev) {
        ev.preventDefault();
        saveGestures(ctx);
      });
    }
    var iForm = ctx.container.querySelector("#icons-form");
    if (iForm) {
      iForm.addEventListener("submit", function (ev) {
        ev.preventDefault();
        saveIcons(ctx);
      });
    }
    var tForm = ctx.container.querySelector("#timings-form");
    if (tForm) {
      tForm.addEventListener("submit", function (ev) {
        ev.preventDefault();
        saveTimings(ctx);
      });
      tForm.querySelectorAll("[data-timing-reset]").forEach(function (b) {
        b.addEventListener("click", function () {
          resetTiming(ctx, b.getAttribute("data-timing-reset"));
        });
      });
    }
    var wForm = ctx.container.querySelector("#wifi-form");
    if (wForm) {
      wForm.addEventListener("submit", function (ev) {
        ev.preventDefault();
        saveWifi(ctx);
      });
      var pw = ctx.container.querySelector("#wifiPass");
      pw.addEventListener("input", function () { pw.dataset.dirty = "1"; });
      var eye = ctx.container.querySelector("#wifi-pw-toggle");
      eye.addEventListener("click", function () {
        var show = pw.type === "password";
        pw.type = show ? "text" : "password";
        eye.classList.toggle("shown", show);
        eye.setAttribute("aria-pressed", show ? "true" : "false");
      });
    }
    var lmForm = ctx.container.querySelector("#library-menu-form");
    if (lmForm) {
      lmForm.addEventListener("submit", function (ev) {
        ev.preventDefault();
        saveLibraryMenu(ctx, { libraryMenu: readLibraryMenuForm(ctx) });
      });
      ctx.container.querySelector("#library-menu-reset")
        .addEventListener("click", function () {
          saveLibraryMenu(ctx, { libraryMenuReset: true });
        });
    }
    var hForm = ctx.container.querySelector("#header-title-form");
    if (hForm) {
      hForm.addEventListener("submit", function (ev) {
        ev.preventDefault();
        saveHeaderTitle(ctx);
      });
      ctx.container.querySelector("#header-title-reset")
        .addEventListener("click", function () { resetHeaderTitle(ctx); });
    }
    ctx.container.querySelector("#delete-sleep").addEventListener("click", function () {
      deleteSleepImage(ctx);
    });
  }

  function gesturesCardHtml(s, gestures) {
    var actionOpts = ACTION_VALUES.map(function (v) {
      return { value: v, label: s.actions[v] };
    });
    return html`<div class="card">
      <h2>${s.buttonsHeading}</h2>
      <p class="muted">${s.buttonsHint}</p>
      <form id="gestures-form" style="margin-top:12px">
        <div class="grid cols-2">
          ${GESTURES.map(function (g) {
            return selectHtml(g[0], s[g[2]], actionOpts, gestures[g[1]] || "none", "");
          })}
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="legacyControls"${gestures.legacyControls ? raw(" checked") : ""}>
              <span>${s.legacyControlsLabel}</span>
            </label>
            <div class="hint">${s.legacyControlsHint}</div>
          </div>
        </div>
        <div class="actions">
          <button type="submit" id="save-gestures">${s.buttonsSave}</button>
          <span class="muted">${s.buttonsLockHint}</span>
        </div>
        <div id="gestures-status"></div>
      </form>
    </div>`;
  }

  // Header status icons + battery indicators. The device stores "battery
  // indicators enabled"; the checkbox is phrased as "hide", so it's inverted.
  function iconsCardHtml(s, state) {
    return html`<div class="card">
      <h2>${s.iconsHeading}</h2>
      <p class="muted">${s.iconsIntro}</p>
      <form id="icons-form" style="margin-top:12px">
        <div class="grid cols-2">
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="sleepIcon"${state.sleepIcon ? raw(" checked") : ""}>
              <span>${s.sleepIconLabel}</span>
            </label>
            <div class="hint">${s.sleepIconHint}</div>
          </div>
          <div class="span-2">
            <label class="check-row">
              <input type="checkbox" id="hideBattery"${state.batteryIndicators === false ? raw(" checked") : ""}>
              <span>${s.hideBatteryLabel}</span>
            </label>
            <div class="hint">${s.hideBatteryHint}</div>
          </div>
        </div>
        <div class="actions">
          <button type="submit" id="save-icons">${s.save}</button>
        </div>
        <div id="icons-status"></div>
      </form>
    </div>`;
  }

  // Click-timing card. The backend sends the list in its own order with live
  // min/max (some floors depend on other timings), so render it as-is.
  function timingsCardHtml(s, timings) {
    var fields = timings.map(function (tm) {
      var label = s.timingLabels[tm.key] || tm.key;
      var hint  = s.timingHints[tm.key]  || "";
      var range = s.timingRange
        .replace("{default}", tm["default"])
        .replace("{min}", tm.min)
        .replace("{max}", tm.max);
      return html`<div>
        <label for="tim-${tm.key}">${label}</label>
        <div style="display:flex;gap:8px;align-items:center;margin-top:6px">
          <input type="number" id="tim-${tm.key}" data-timing="${tm.key}"
                 min="${tm.min}" max="${tm.max}" step="1" value="${tm.value}"
                 style="max-width:120px">
          <button type="button" class="btn secondary" data-timing-reset="${tm.key}">${s.timingsReset}</button>
        </div>
        <div class="hint">${hint} <span class="muted">${range}</span></div>
      </div>`;
    });
    return html`<div class="card">
      <h2>${s.timingsHeading}</h2>
      <p class="muted">${s.timingsIntro}</p>
      <form id="timings-form" style="margin-top:12px">
        <div class="grid cols-2">${fields}</div>
        <div class="actions">
          <button type="submit" id="save-timings">${s.timingsSave}</button>
          <span class="muted">${s.applyHint}</span>
        </div>
        <div id="timings-status"></div>
      </form>
    </div>`;
  }

  // Wi-Fi card. The device never returns the stored password; the field
  // starts empty with a "<saved>" placeholder, and `pass` is only sent once
  // the user has typed in it (so clearing it on purpose saves an open
  // network, while editing just the SSID keeps the stored password).
  function wifiCardHtml(s, wifi) {
    var pwPlaceholder = wifi.hasPassword ? s.wifiPasswordSaved : s.wifiPasswordPlaceholder;
    return html`<div class="card">
      <h2>${s.wifiHeading}</h2>
      <p class="muted">${s.wifiIntro}</p>
      <form id="wifi-form" style="margin-top:12px" autocomplete="off">
        <div class="grid cols-2">
          <div>
            <label for="wifiSsid">${s.wifiSsidLabel}</label>
            <input type="text" id="wifiSsid" maxlength="32"
                   placeholder="${s.wifiSsidPlaceholder}" value="${wifi.ssid || ""}">
          </div>
          <div>
            <label for="wifiPass">${s.wifiPasswordLabel}</label>
            <div class="pwwrap">
              <input type="password" id="wifiPass" maxlength="64"
                     autocomplete="new-password" placeholder="${pwPlaceholder}">
              <button type="button" class="pweye" id="wifi-pw-toggle"
                      aria-label="${s.wifiShowPassword}" aria-pressed="false">
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor"
                     stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
                  <path d="M1 12s4-7 11-7 11 7 11 7-4 7-11 7S1 12 1 12z"/><circle cx="12" cy="12" r="3"/>
                  <line class="pwh" x1="3" y1="21" x2="21" y2="3"/>
                </svg>
              </button>
            </div>
          </div>
        </div>
        <div class="actions">
          <button type="submit" id="save-wifi">${s.wifiSave}</button>
          <span class="muted">${s.wifiHint}</span>
        </div>
        <div id="wifi-status"></div>
      </form>
    </div>`;
  }

  function setWifiStatus(ctx, kind, msg) {
    var el = ctx.container.querySelector("#wifi-status");
    if (!el) return;
    el.className   = "status " + kind;
    el.textContent = msg;
  }

  async function saveWifi(ctx) {
    var t = ctx.t;
    var pw = ctx.container.querySelector("#wifiPass");
    var wifi = { ssid: ctx.container.querySelector("#wifiSsid").value };
    if (pw.dataset.dirty) wifi.pass = pw.value;
    var btn = ctx.container.querySelector("#save-wifi");
    btn.disabled = true;
    var prevLabel = btn.textContent;
    btn.textContent = t.settings.saving;
    setWifiStatus(ctx, "busy", t.settings.saving);
    try {
      var state = await window.palaApi.post("/api/settings", { wifi: wifi });
      renderForm(ctx, state);
      setWifiStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      btn.disabled    = false;
      btn.textContent = prevLabel;
      setWifiStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  // Library menu order: one select per position, each offering "Hidden" plus
  // every entry. Duplicates are dropped by the device (first one wins), and
  // Upload is always kept so the device stays reachable.
  function libraryMenuCardHtml(s, order, slots) {
    var opts = [{ value: "", label: s.libraryMenuHidden }].concat(
      LIBRARY_ENTRIES.map(function (v) { return { value: v, label: s.libraryEntries[v] }; })
    );
    var selects = [];
    for (var i = 0; i < slots; i++) {
      selects.push(selectHtml("lib" + i, s.libraryMenuSlot + " " + (i + 1), opts, order[i] || "", ""));
    }
    return html`<div class="card">
      <h2>${s.libraryMenuHeading}</h2>
      <p class="muted">${s.libraryMenuIntro}</p>
      <p class="muted">${s.libraryMenuRequired}</p>
      <form id="library-menu-form" style="margin-top:12px">
        <div class="grid cols-2">${selects}</div>
        <div class="actions">
          <button type="submit" id="save-library-menu">${s.save}</button>
          <button type="button" class="btn secondary" id="library-menu-reset">${s.libraryMenuReset}</button>
          <span class="muted">${s.libraryMenuHint}</span>
        </div>
        <div id="library-menu-status"></div>
      </form>
    </div>`;
  }

  function readLibraryMenuForm(ctx) {
    var out = [];
    ctx.container.querySelectorAll("#library-menu-form select").forEach(function (sel) {
      if (sel.value && out.indexOf(sel.value) < 0) out.push(sel.value);
    });
    return out;
  }

  function setLibraryMenuStatus(ctx, kind, msg) {
    var el = ctx.container.querySelector("#library-menu-status");
    if (!el) return;
    el.className   = "status " + kind;
    el.textContent = msg;
  }

  async function saveLibraryMenu(ctx, payload) {
    var t = ctx.t;
    setLibraryMenuStatus(ctx, "busy", t.settings.saving);
    try {
      // Re-render with the device's normalised order (dedupe, Upload kept).
      var state = await window.palaApi.post("/api/settings", payload);
      renderForm(ctx, state);
      setLibraryMenuStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      setLibraryMenuStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  function libraryCardHtml(s, current, defaultTitle) {
    var placeholder = (s.headerTitlePlaceholder || "")
      .replace("{default}", defaultTitle || "");
    return html`<div class="card">
      <h2>${s.libraryHeading}</h2>
      <p class="muted">${s.libraryIntro}</p>
      <form id="header-title-form" style="margin-top:12px">
        <div>
          <label for="headerTitle">${s.headerTitleLabel}</label>
          <input type="text" id="headerTitle" maxlength="31"
                 value="${current}" placeholder="${placeholder}">
          <div class="hint">${s.headerTitleHint}</div>
        </div>
        <div class="actions">
          <button type="submit" id="save-header-title">${s.headerTitleSave}</button>
          <button type="button" class="btn secondary" id="header-title-reset">${s.headerTitleReset}</button>
        </div>
        <div id="header-title-status"></div>
      </form>
    </div>`;
  }

  function readForm(ctx) {
    return {
      font:          parseInt(ctx.container.querySelector("#font").value,  10),
      family:        ctx.container.querySelector("#family").value,
      sleep:         parseInt(ctx.container.querySelector("#sleep").value, 10),
      lgap:          parseInt(ctx.container.querySelector("#lgap").value,  10),
      bionic:        ctx.container.querySelector("#bionic").checked,
      halfParagraphGaps: ctx.container.querySelector("#halfParagraphGaps").checked,
      noScreensaver: ctx.container.querySelector("#noScreensaver").checked,
      lockOnSleep:    ctx.container.querySelector("#lockOnSleep").checked,
      screenRotation: ctx.container.querySelector("#screenRotation").checked
    };
  }

  function setStatus(ctx, kind, msg) {
    var el = ctx.container.querySelector("#settings-status");
    if (!el) return;
    el.className   = "status " + kind;
    el.textContent = msg;
  }

  function setGesturesStatus(ctx, kind, msg) {
    var el = ctx.container.querySelector("#gestures-status");
    if (!el) return;
    el.className   = "status " + kind;
    el.textContent = msg;
  }

  function readGesturesForm(ctx) {
    var gestures = {
      legacyControls: ctx.container.querySelector("#legacyControls").checked
    };
    GESTURES.forEach(function (g) {
      gestures[g[1]] = ctx.container.querySelector("#" + g[0]).value;
    });
    return { gestures: gestures };
  }

  function missingRequiredActions(gestures) {
    var bound = GESTURES.map(function (g) { return gestures[g[1]]; });
    return REQUIRED_ACTIONS.filter(function (a) { return bound.indexOf(a) < 0; });
  }

  function setHeaderTitleStatus(ctx, kind, msg) {
    var el = ctx.container.querySelector("#header-title-status");
    if (!el) return;
    el.className   = "status " + kind;
    el.textContent = msg;
  }

  async function saveHeaderTitle(ctx) {
    var t = ctx.t;
    var btn = ctx.container.querySelector("#save-header-title");
    btn.disabled = true;
    var prevLabel = btn.textContent;
    btn.textContent = t.settings.saving;
    setHeaderTitleStatus(ctx, "busy", t.settings.saving);
    try {
      var val = ctx.container.querySelector("#headerTitle").value;
      var state = await window.palaApi.post("/api/settings", { headerTitle: val });
      // Re-render with the server's resulting state (e.g. trimmed value).
      renderForm(ctx, state);
      setHeaderTitleStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      btn.disabled    = false;
      btn.textContent = prevLabel;
      setHeaderTitleStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  async function resetHeaderTitle(ctx) {
    var t = ctx.t;
    if (!window.confirm(t.settings.headerTitleResetConfirm)) return;
    setHeaderTitleStatus(ctx, "busy", t.settings.saving);
    try {
      var state = await window.palaApi.post("/api/settings", { headerTitleReset: true });
      renderForm(ctx, state);
      setHeaderTitleStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      setHeaderTitleStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  function setIconsStatus(ctx, kind, msg) {
    var el = ctx.container.querySelector("#icons-status");
    if (!el) return;
    el.className   = "status " + kind;
    el.textContent = msg;
  }

  async function saveIcons(ctx) {
    var t = ctx.t;
    var btn = ctx.container.querySelector("#save-icons");
    btn.disabled = true;
    var prevLabel = btn.textContent;
    btn.textContent = t.settings.saving;
    setIconsStatus(ctx, "busy", t.settings.saving);
    try {
      var state = await window.palaApi.post("/api/settings", {
        sleepIcon:         ctx.container.querySelector("#sleepIcon").checked,
        batteryIndicators: !ctx.container.querySelector("#hideBattery").checked
      });
      renderForm(ctx, state);
      setIconsStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      btn.disabled    = false;
      btn.textContent = prevLabel;
      setIconsStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  function setTimingsStatus(ctx, kind, msg) {
    var el = ctx.container.querySelector("#timings-status");
    if (!el) return;
    el.className   = "status " + kind;
    el.textContent = msg;
  }

  function readTimingsForm(ctx) {
    var timings = {};
    ctx.container.querySelectorAll("[data-timing]").forEach(function (inp) {
      var v = parseInt(inp.value, 10);
      if (!isNaN(v)) timings[inp.getAttribute("data-timing")] = v;
    });
    return { timings: timings };
  }

  async function saveTimings(ctx) {
    var t = ctx.t;
    var btn = ctx.container.querySelector("#save-timings");
    btn.disabled = true;
    var prevLabel = btn.textContent;
    btn.textContent = t.settings.saving;
    setTimingsStatus(ctx, "busy", t.settings.saving);
    try {
      // The device clamps out-of-range values; re-rendering shows what stuck.
      var state = await window.palaApi.post("/api/settings", readTimingsForm(ctx));
      renderForm(ctx, state);
      setTimingsStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      btn.disabled    = false;
      btn.textContent = prevLabel;
      setTimingsStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  async function resetTiming(ctx, key) {
    var t = ctx.t;
    setTimingsStatus(ctx, "busy", t.settings.saving);
    try {
      var state = await window.palaApi.post("/api/settings", { timingsReset: [key] });
      renderForm(ctx, state);
      setTimingsStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      setTimingsStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  async function saveGestures(ctx) {
    var t = ctx.t;
    var payload = readGesturesForm(ctx);
    var missing = missingRequiredActions(payload.gestures);
    if (missing.length) {
      var names = missing.map(function (a) { return t.settings.actions[a]; });
      setGesturesStatus(ctx, "err", t.settings.buttonsMissingRequired + " " + names.join(", "));
      return;
    }
    var btn = ctx.container.querySelector("#save-gestures");
    btn.disabled = true;
    var prevLabel = btn.textContent;
    btn.textContent = t.settings.saving;
    setGesturesStatus(ctx, "busy", t.settings.saving);
    try {
      var state = await window.palaApi.post("/api/settings", payload);
      // Whole-screen redraw so the new bindings stick (and any clamping
      // the device did is reflected back). Cheaper than threading the
      // gestures sub-object through renderForm by hand.
      renderForm(ctx, state);
      setGesturesStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      btn.disabled    = false;
      btn.textContent = prevLabel;
      setGesturesStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  async function save(ctx) {
    var t = ctx.t;
    var btn = ctx.container.querySelector("#save");
    btn.disabled = true;
    var prevLabel = btn.textContent;
    btn.textContent = t.settings.saving;
    setStatus(ctx, "busy", t.settings.saving);
    try {
      var state = await window.palaApi.post("/api/settings", readForm(ctx));
      // Server returns the resulting state. Re-render so any clamping it
      // did (e.g. value out of range) is reflected back in the form.
      renderForm(ctx, state);
      setStatus(ctx, "ok", t.settings.saved);
    } catch (e) {
      btn.disabled    = false;
      btn.textContent = prevLabel;
      setStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  async function deleteSleepImage(ctx) {
    var t = ctx.t;
    if (!window.confirm(t.settings.deleteSleepConfirm)) return;
    try {
      var res = await window.palaApi.post("/api/sleep-image/delete");
      // Backend includes the resulting hasSleepImage flag.
      if (res && !res.hasSleepImage) {
        ctx.container.querySelector("#sleep-image-card").setAttribute("hidden", "");
      }
      setStatus(ctx, "ok", t.settings.sleepImageDeleted);
    } catch (e) {
      setStatus(ctx, "err", (t.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  async function render(ctx) {
    ctx.header({
      title:    ctx.t.settings.title,
      subtitle: ctx.t.settings.subtitle,
      navKey:   "settings"
    });
    ctx.container.innerHTML =
      html`<div class="card"><p class="muted">${ctx.t.settings.loading}</p></div>`;
    try {
      var state = await window.palaApi.get("/api/settings");
      renderForm(ctx, state);
    } catch (e) {
      ctx.container.innerHTML =
        html`<div class="banner-warn">${ctx.t.errors.server}: ${e.message || e}</div>`;
    }
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.settings = { render: render };
})();
