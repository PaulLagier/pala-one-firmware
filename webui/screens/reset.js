// Reset screen — POSTs /api/reset after a confirm() prompt and shows the
// outcome inline. On success the device has wiped storage + NVS but stays
// running, so we swap the card for a success banner instead of redirecting.

(function () {
  function render(ctx) {
    var t = ctx.t;
    ctx.header({
      title:    t.reset.title,
      subtitle: t.reset.subtitle,
      navKey:   "reset"
    });

    ctx.container.innerHTML =
      '<div class="card">' +
        '<h2>' + t.reset.confirmHeading + '</h2>' +
        '<p><strong>' + t.reset.warn + '</strong></p>' +
        '<p class="muted">' + t.reset.detail + '</p>' +
        '<div class="actions">' +
          '<button class="btn danger" id="btn-reset" type="button">' +
            t.reset.button +
          '</button>' +
          '<a class="btn secondary" href="#/">' + t.reset.cancel + '</a>' +
        '</div>' +
        '<div id="reset-status" class="muted" style="margin-top:12px"></div>' +
      '</div>';

    var btn    = ctx.container.querySelector("#btn-reset");
    var status = ctx.container.querySelector("#reset-status");

    btn.addEventListener("click", async function () {
      // Browser confirm() is enough here — destructive action with a single
      // unambiguous outcome. If the styling ever feels insufficient, replace
      // with an in-page modal; the rest of this handler stays the same.
      if (!window.confirm(t.reset.confirmJs)) return;

      btn.disabled    = true;
      btn.textContent = t.reset.inProgress;
      status.textContent = "";
      status.className   = "muted";

      try {
        await window.palaApi.post("/api/reset");
        // On success the device has wiped storage + NVS but stayed running,
        // so swap the card for a success banner instead of redirecting.
        ctx.container.innerHTML =
          '<div class="banner-ok">' + t.reset.success + '</div>' +
          '<div class="card"><p class="muted">' + t.reset.successDetail + '</p>' +
            '<div class="actions">' +
              '<a class="btn" href="#/">' + t.reset.goHome + '</a>' +
            '</div>' +
          '</div>';
      } catch (e) {
        btn.disabled    = false;
        btn.textContent = t.reset.button;
        status.textContent = (t.errors.server || "Server error") +
                             ": " + (e.message || e);
        status.className   = "banner-warn";
      }
    });
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.reset = { render: render };
})();
