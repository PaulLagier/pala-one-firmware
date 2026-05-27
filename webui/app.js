// SPA boot + hash router + theme toggle.
//
// Screens register themselves on window.palaScreens via screens/*.js. Routes
// come from location.hash: '' / '#/' -> home, '#/reset' -> reset, etc.
// Each screen exports `render({ container, header, t, info })`:
//   - container: the <main> element to render into
//   - header({ title, subtitle, navKey }): updates the page chrome
//   - t: the resolved-language i18n dictionary
//   - info: the /api/info payload (lang + fw + build)
//
// Locale resolution (also used by the placeholder during Phase 0):
//   explicit user override > device language > browser language > 'en'.

(function () {
  // --- Pre-paint theme (matches the legacy chrome.cpp behaviour so users'
  //     existing localStorage.palaTheme preference carries over). -----------
  (function applyThemeEarly() {
    var k = "palaTheme";
    var html = document.documentElement;
    var v = null;
    try { v = localStorage.getItem(k); } catch (_) { /* private mode */ }
    html.dataset.theme = (v === "dark" || v === "light") ? v : "light";
  })();

  function toggleTheme() {
    var html = document.documentElement;
    var next = (html.dataset.theme === "dark") ? "light" : "dark";
    html.dataset.theme = next;
    try { localStorage.setItem("palaTheme", next); } catch (_) { /* ignore */ }
  }
  window.palaToggleTheme = toggleTheme;

  // --- Locale resolution ----------------------------------------------------
  function pickLang(info) {
    var dict = window.__pala_i18n || {};
    var userOverride = null;
    try { userOverride = localStorage.getItem("palaWebLang"); } catch (_) {}
    var browserLang = (navigator.language || "en").slice(0, 2);

    if (userOverride && dict[userOverride]) return userOverride;
    if (info && info.lang && dict[info.lang]) return info.lang;
    if (dict[browserLang]) return browserLang;
    return "en";
  }

  // --- Header chrome --------------------------------------------------------
  // Rebuilt by each screen's render() via the header({...}) callback. Kept
  // outside the screen so the toggle button keeps focus / state across nav.
  function setHeader(els, t, opts) {
    opts = opts || {};
    els.title.textContent    = opts.title    || t.brand || "Pala One";
    els.subtitle.textContent = opts.subtitle || "";
    // Highlight the current nav entry — purely cosmetic, helps orient.
    var key = opts.navKey || "";
    Array.prototype.forEach.call(els.nav.children, function (a) {
      if (a.dataset.navKey === key) a.classList.add("active");
      else                          a.classList.remove("active");
    });
  }

  function buildNav(els, t) {
    // Phase 3+: list the SPA-ported screens here. Legacy screens still link
    // out to their full-page URLs (rendered by the screen's body, not nav)
    // until each one is ported.
    var items = [
      { key: "home",         href: "#/",             label: t.nav.home         },
      { key: "files",        href: "#/files",        label: t.nav.files        },
      { key: "bookmarks",    href: "#/bookmarks",    label: t.nav.bookmarks    },
      { key: "list",         href: "#/list",         label: t.nav.list         },
      { key: "screensavers", href: "#/screensavers", label: t.nav.screensavers },
      { key: "settings",     href: "#/settings",     label: t.nav.settings     },
      { key: "reset",        href: "#/reset",        label: t.nav.reset        }
    ];
    els.nav.innerHTML = "";
    items.forEach(function (it) {
      var a = document.createElement("a");
      a.href = it.href;
      a.textContent = it.label;
      a.dataset.navKey = it.key;
      els.nav.appendChild(a);
    });
  }

  // --- Router ---------------------------------------------------------------
  // Hash layout: '#/<name>(?<query>)?'. Examples:
  //   '#/'                        -> { name: 'home',      params: {} }
  //   '#/list'                    -> { name: 'list',      params: {} }
  //   '#/bookmarks?book=0&idx=2'  -> { name: 'bookmarks', params: { book: '0', idx: '2' } }
  //
  // Params arrive at screens via render({ params }). Screens that don't care
  // (home, list, reset) just ignore them.
  function currentRoute() {
    var raw = (location.hash || "").replace(/^#\/?/, "");
    var qPos = raw.indexOf("?");
    var name = (qPos < 0 ? raw : raw.slice(0, qPos)) || "home";
    var params = {};
    if (qPos >= 0) {
      new URLSearchParams(raw.slice(qPos + 1)).forEach(function (v, k) {
        params[k] = v;
      });
    }
    return { name: name, params: params };
  }

  function dispatch(els, t, info) {
    var route  = currentRoute();
    var screen = (window.palaScreens || {})[route.name];
    if (!screen) {
      els.content.innerHTML =
        '<div class="card"><h2>Not found</h2>' +
        '<p class="muted">No screen registered for #/' + route.name + '</p></div>';
      setHeader(els, t, { title: t.brand });
      return;
    }
    var header = function (opts) { setHeader(els, t, opts); };
    try {
      screen.render({
        container: els.content,
        header:    header,
        t:         t,
        info:      info,
        params:    route.params
      });
    } catch (e) {
      els.content.innerHTML =
        '<div class="banner-warn">Screen crashed: ' + (e && e.message || e) + '</div>';
    }
  }

  // --- Boot -----------------------------------------------------------------
  async function boot() {
    var els = {
      title:    document.getElementById("page-title"),
      subtitle: document.getElementById("page-subtitle"),
      nav:      document.getElementById("page-nav"),
      content:  document.getElementById("content")
    };

    var info = null;
    try {
      info = await window.palaApi.get("/api/info");
    } catch (_) {
      // Boot continues — i18n falls back to browser/en, header just shows
      // brand. Individual screens may still fail their own API calls.
      info = {};
    }

    var lang = pickLang(info);
    var dict = window.__pala_i18n || {};
    var t = dict[lang] || dict.en || {};

    buildNav(els, t);
    dispatch(els, t, info);
    window.addEventListener("hashchange", function () { dispatch(els, t, info); });
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", boot);
  } else {
    boot();
  }
})();
