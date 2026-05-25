// Home screen — SPA landing page during the rewrite.
//
// Lists the screens that have been ported to the SPA (linked via the hash
// router) and the screens that still live at their legacy full-page URLs.
// As each screen lands in the SPA we move its entry from `legacy` to
// `ported` (here and in app.js' buildNav). Once everything is ported and
// Phase 4 swaps `/` to serve this SPA, the legacy card can be deleted.

(function () {
  function render(ctx) {
    var t = ctx.t;
    var info = ctx.info || {};

    ctx.header({
      title:    t.home.title,
      subtitle: (t.home.subtitleFw || "Firmware") + " " + (info.fw || "?"),
      navKey:   "home"
    });

    var html = "";

    html +=
      '<div class="card">' +
        '<h2>' + t.home.portedHeading + '</h2>' +
        '<ul class="list">' +
          '<li><a class="link" href="#/files">'     + t.nav.files     + '</a></li>' +
          '<li><a class="link" href="#/bookmarks">' + t.nav.bookmarks + '</a></li>' +
          '<li><a class="link" href="#/list">'      + t.nav.list      + '</a></li>' +
          '<li><a class="link" href="#/settings">'  + t.nav.settings  + '</a></li>' +
          '<li><a class="link" href="#/reset">'     + t.nav.reset     + '</a></li>' +
        '</ul>' +
      '</div>';

    var legacyLinks = [
      { href: "/screensavers", label: t.home.legacy.screensavers }
    ];
    html +=
      '<div class="card">' +
        '<h2>' + t.home.legacyHeading + '</h2>' +
        '<p class="muted">' + t.home.legacyNote + '</p>' +
        '<ul class="list">';
    legacyLinks.forEach(function (l) {
      html += '<li><a class="link" href="' + l.href + '">' + l.label + '</a></li>';
    });
    html += '</ul></div>';

    ctx.container.innerHTML = html;
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.home = { render: render };
})();
