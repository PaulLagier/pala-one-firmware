// Home screen — SPA landing page. Simple nav grid into every other screen.
// Subtitle shows the running firmware version (from /api/info).

(function () {
  function render(ctx) {
    var t = ctx.t;
    var info = ctx.info || {};

    ctx.header({
      title:    t.home.title,
      subtitle: (t.home.subtitleFw || "Firmware") + " " + (info.fw || "?"),
      navKey:   "home"
    });

    ctx.container.innerHTML =
      '<div class="card">' +
        '<h2>' + t.home.portedHeading + '</h2>' +
        '<ul class="list">' +
          '<li><a class="link" href="#/files">'        + t.nav.files        + '</a></li>' +
          '<li><a class="link" href="#/bookmarks">'    + t.nav.bookmarks    + '</a></li>' +
          '<li><a class="link" href="#/list">'         + t.nav.list         + '</a></li>' +
          '<li><a class="link" href="#/screensavers">' + t.nav.screensavers + '</a></li>' +
          '<li><a class="link" href="#/settings">'     + t.nav.settings     + '</a></li>' +
          '<li><a class="link" href="#/reset">'        + t.nav.reset        + '</a></li>' +
        '</ul>' +
      '</div>';
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.home = { render: render };
})();
