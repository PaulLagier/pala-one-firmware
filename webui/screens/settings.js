// Settings screen — GET /api/settings, edit form, POST full state.
//
// Reading-related settings (font size/family/line-gap/bionic) can shift the
// active reader's page table; the backend handles cursor re-location, the
// SPA just sends the new state and trusts the device. The response carries
// the resulting state so we can refresh the form without a second GET.

(function () {
  function escapeHtml(s) {
    return String(s == null ? "" : s)
      .replace(/&/g, "&amp;").replace(/</g, "&lt;")
      .replace(/>/g, "&gt;").replace(/"/g, "&quot;")
      .replace(/'/g, "&#39;");
  }

  // Static option lists for the selects — values are the wire shape the
  // backend expects, labels come from i18n at render time.
  var FONT_VALUES    = [8, 10, 12, 14];
  var SLEEP_VALUES   = [30, 60, 120, 300, 600, 1800];
  var LGAP_VALUES    = [0, 1, 2, 3];
  var FAMILY_VALUES  = ["helv", "dys"];
  // Gesture-bindable actions. Order matches the legacy UI; "none" first so
  // users can blank out a binding from the keyboard with one arrow press.
  var ACTION_VALUES  = ["none", "bookmark", "lock", "menu"];

  function selectHtml(id, name, options, currentValue, hint) {
    var opts = options.map(function (o) {
      var sel = (String(o.value) === String(currentValue)) ? " selected" : "";
      return '<option value="' + escapeHtml(o.value) + '"' + sel + '>' +
             escapeHtml(o.label) + '</option>';
    }).join("");
    return '<div>' +
      '<label for="' + id + '">' + escapeHtml(name) + '</label>' +
      '<select id="' + id + '">' + opts + '</select>' +
      '<div class="hint">' + escapeHtml(hint) + '</div>' +
    '</div>';
  }

  function renderForm(ctx, state) {
    var t   = ctx.t;
    var s   = t.settings;
    var fontOpts   = FONT_VALUES.map(function (v)  { return { value: v, label: s.fontSizes[v]   }; });
    var sleepOpts  = SLEEP_VALUES.map(function (v) { return { value: v, label: s.sleepTimes[v]  }; });
    var lgapOpts   = LGAP_VALUES.map(function (v)  { return { value: v, label: s.lineGaps[v]    }; });
    var familyOpts = FAMILY_VALUES.map(function (v){ return { value: v, label: s.families[v]    }; });

    ctx.container.innerHTML =
      '<div class="card">' +
        '<h2>' + s.readingHeading + '</h2>' +
        '<p class="muted">' + s.readingIntro + '</p>' +
        '<form id="settings-form" style="margin-top:12px">' +
          '<div class="grid cols-2">' +
            selectHtml("font",   s.fontSize,    fontOpts,   state.font,   s.fontSizeHint)   +
            selectHtml("family", s.fontFamily,  familyOpts, state.family, s.familyHint)     +
            selectHtml("sleep",  s.sleepAfter,  sleepOpts,  state.sleep,  s.sleepHint)      +
            selectHtml("lgap",   s.lineSpacing, lgapOpts,   state.lgap,   s.lineSpacingHint) +
            '<div class="span-2">' +
              '<label class="check-row">' +
                '<input type="checkbox" id="bionic"' + (state.bionic ? " checked" : "") + '>' +
                '<span>' + s.bionicLabel + '</span>' +
              '</label>' +
              '<div class="hint">' + s.bionicHint + '</div>' +
            '</div>' +
            '<div class="span-2">' +
              '<label class="check-row">' +
                '<input type="checkbox" id="noScreensaver"' + (state.noScreensaver ? " checked" : "") + '>' +
                '<span>' + s.noScreensaverLabel + '</span>' +
              '</label>' +
              '<div class="hint">' + s.noScreensaverHint + '</div>' +
            '</div>' +
          '</div>' +
          '<div class="actions">' +
            '<button type="submit" id="save">' + s.save + '</button>' +
            '<span class="muted">' + s.applyHint + '</span>' +
          '</div>' +
          '<div id="settings-status"></div>' +
        '</form>' +
      '</div>' +

      // Buttons (gesture bindings) — its own form + save button so users
      // can rebind a hold without touching the reading sliders.
      gesturesCardHtml(s, state.gestures || {}) +

      // Sleep-image management card. Hidden when no custom image exists.
      '<div class="card" id="sleep-image-card"' + (state.hasSleepImage ? '' : ' hidden') + '>' +
        '<h2>' + s.sleepImageHeading + '</h2>' +
        '<p class="muted">' + s.sleepImagePresent + '</p>' +
        '<div class="actions">' +
          '<button type="button" class="btn secondary" id="delete-sleep">' +
            s.deleteSleepImage +
          '</button>' +
        '</div>' +
      '</div>' +

      // Quick link to the screensaver editor on its dedicated SPA screen.
      '<div class="card">' +
        '<h2>' + s.screensaverHeading + '</h2>' +
        '<p class="muted">' + s.screensaverCardDesc + '</p>' +
        '<div class="actions">' +
          '<a class="btn" href="#/screensavers">' + s.screensaverEditorLink + '</a>' +
          '<span class="muted">' + s.screensaverEditorHint + '</span>' +
        '</div>' +
      '</div>';

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
    ctx.container.querySelector("#delete-sleep").addEventListener("click", function () {
      deleteSleepImage(ctx);
    });
  }

  function gesturesCardHtml(s, gestures) {
    var actionOpts = ACTION_VALUES.map(function (v) {
      return { value: v, label: s.actions[v] };
    });
    return (
      '<div class="card">' +
        '<h2>' + s.buttonsHeading + '</h2>' +
        '<p class="muted">' + s.buttonsHint + '</p>' +
        '<form id="gestures-form" style="margin-top:12px">' +
          '<div class="grid cols-2">' +
            selectHtml("btnL",  s.buttonsLong,      actionOpts, gestures.long      || "none", "") +
            selectHtml("btnXL", s.buttonsExtraLong, actionOpts, gestures.extraLong || "none", "") +
            selectHtml("btnCH", s.buttonsClickHold, actionOpts, gestures.clickHold || "none", "") +
          '</div>' +
          '<div class="actions">' +
            '<button type="submit" id="save-gestures">' + s.buttonsSave + '</button>' +
            '<span class="muted">' + s.buttonsLockHint + '</span>' +
          '</div>' +
          '<div id="gestures-status"></div>' +
        '</form>' +
      '</div>'
    );
  }

  function readForm(ctx) {
    return {
      font:          parseInt(ctx.container.querySelector("#font").value,  10),
      family:        ctx.container.querySelector("#family").value,
      sleep:         parseInt(ctx.container.querySelector("#sleep").value, 10),
      lgap:          parseInt(ctx.container.querySelector("#lgap").value,  10),
      bionic:        ctx.container.querySelector("#bionic").checked,
      noScreensaver: ctx.container.querySelector("#noScreensaver").checked
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
    return {
      gestures: {
        long:      ctx.container.querySelector("#btnL").value,
        extraLong: ctx.container.querySelector("#btnXL").value,
        clickHold: ctx.container.querySelector("#btnCH").value
      }
    };
  }

  async function saveGestures(ctx) {
    var t = ctx.t;
    var btn = ctx.container.querySelector("#save-gestures");
    btn.disabled = true;
    var prevLabel = btn.textContent;
    btn.textContent = t.settings.saving;
    setGesturesStatus(ctx, "busy", t.settings.saving);
    try {
      var state = await window.palaApi.post("/api/settings", readGesturesForm(ctx));
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
      '<div class="card"><p class="muted">' + ctx.t.settings.loading + '</p></div>';
    try {
      var state = await window.palaApi.get("/api/settings");
      renderForm(ctx, state);
    } catch (e) {
      ctx.container.innerHTML =
        '<div class="banner-warn">' + escapeHtml(ctx.t.errors.server) +
        ': ' + escapeHtml(e.message || e) + '</div>';
    }
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.settings = { render: render };
})();
