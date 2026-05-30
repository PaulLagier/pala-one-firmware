// Bookmarks screen — two modes selected by URL params:
//   #/bookmarks                       -> list mode (all books + bookmarks)
//   #/bookmarks?book=N&idx=M          -> detail mode (page text for one bookmark)
//
// Hash changes are reflected in the URL so detail views are shareable and
// the browser back button does the right thing.

(function () {
  var html = window.palaHtml.html;

  // -- List mode -------------------------------------------------------------
  async function renderList(ctx) {
    var t = ctx.t;
    ctx.header({
      title:    t.bookmarks.title,
      subtitle: t.bookmarks.subtitle,
      navKey:   "bookmarks"
    });
    ctx.container.innerHTML =
      html`<div class="card"><p class="muted">${t.bookmarks.loading}</p></div>`;

    var data;
    try {
      data = await window.palaApi.get("/api/bookmarks");
    } catch (e) {
      ctx.container.innerHTML =
        html`<div class="banner-warn">${t.errors.server}: ${e.message || e}</div>`;
      return;
    }

    var books = (data && data.books) || [];
    if (books.length === 0) {
      ctx.container.innerHTML =
        html`<div class="card"><p class="muted">${t.bookmarks.noBooks}</p></div>`;
      return;
    }

    // Hide books that have no bookmarks AND no error — the list of files lives
    // on the files screen, no reason to repeat zero-bookmark cards here.
    var cards = books.map(function (book) {
      var bms = book.bookmarks || [];
      if (bms.length === 0 && !book.error) return "";  // resolve() drops ""

      if (book.error) {
        return html`<div class="card">
          <h2>${book.name}</h2>
          <p class="muted">${t.bookmarks.openFailed}</p>
        </div>`;
      }

      var items = bms.map(function (bm) {
        var viewHref = "#/bookmarks?book=" + encodeURIComponent(book.id) +
                       "&idx="             + encodeURIComponent(bm.idx);
        return html`<li><div class="row">
          <div>
            <div class="pill">${t.bookmarks.pillPrefix}${bm.idx + 1}</div>
            <p class="meta" style="margin-top:8px">${bm.label}</p>
          </div>
          <div style="white-space:nowrap">
            <a class="link" href="${viewHref}">${t.bookmarks.view}</a> |
            <button type="button" class="btn secondary small"
                    data-book="${book.id}" data-idx="${bm.idx}">${t.bookmarks.del}</button>
          </div>
        </div></li>`;
      });

      var exportHref = "/api/bookmarks/export?book=" + encodeURIComponent(book.id);
      return html`<div class="card">
        <h2>${book.name}</h2>
        <ul class="list">${items}</ul>
        <div class="actions">
          <a class="btn secondary" href="${exportHref}">${t.bookmarks.downloadAll}</a>
        </div>
      </div>`;
    });

    var anyShown = books.some(function (b) {
      return (b.bookmarks || []).length > 0 || b.error;
    });
    ctx.container.innerHTML = anyShown
      ? html`${cards}`
      : html`<div class="card"><p class="muted">${t.bookmarks.noneAcrossLibrary}</p></div>`;
  }

  // Delete-click handler. Installed once by render() — `ctx.container`
  // outlives every innerHTML reassignment, so re-binding inside
  // renderList() (as we used to) stacks a new listener on every refresh
  // and ends up firing N confirm dialogs after N–1 deletes.
  function wireDeleteHandler(ctx) {
    var t = ctx.t;
    ctx.container.addEventListener("click", function (ev) {
      var btn = ev.target.closest('button[data-book]');
      if (!btn) return;
      var book = parseInt(btn.dataset.book, 10);
      var idx  = parseInt(btn.dataset.idx,  10);
      if (!window.confirm(t.bookmarks.confirmDelete)) return;
      btn.disabled = true;
      btn.textContent = "…";
      window.palaApi.post("/api/bookmarks/delete", { book: book, idx: idx })
        .then(function () { renderList(ctx); })  // re-fetch + redraw
        .catch(function (e) {
          window.alert(t.errors.server + ": " + (e.message || e));
          btn.disabled = false;
          btn.textContent = t.bookmarks.del;
        });
    });
  }

  // -- Detail mode -----------------------------------------------------------
  async function renderDetail(ctx, book, idx) {
    var t = ctx.t;
    ctx.header({
      title:    t.bookmarks.viewTitle,
      subtitle: t.bookmarks.viewSubtitle,
      navKey:   "bookmarks"
    });
    ctx.container.innerHTML =
      html`<div class="card"><p class="muted">${t.bookmarks.loading}</p></div>`;

    var data;
    try {
      data = await window.palaApi.get(
        "/api/bookmarks/view?book=" + encodeURIComponent(book) +
        "&idx="                     + encodeURIComponent(idx)
      );
    } catch (e) {
      ctx.container.innerHTML = html`
        <div class="banner-warn">${t.errors.server}: ${e.message || e}</div>
        <div class="actions">
          <a class="btn secondary" href="#/bookmarks">${t.bookmarks.back}</a>
        </div>`;
      return;
    }

    var b  = data.book || {};
    var bm = data.bookmark || {};
    var exportHref = "/api/bookmarks/export?book=" + encodeURIComponent(b.id);

    ctx.container.innerHTML = html`<div class="card">
      <h2>${b.name}</h2>
      <p class="muted">${t.bookmarks.pillPrefix}${(bm.idx || 0) + 1}</p>
      <pre class="pre" style="margin-top:10px">${bm.text}</pre>
      <div class="actions">
        <a class="btn secondary" href="#/bookmarks">${t.bookmarks.back}</a>
        <a class="btn secondary" href="${exportHref}">${t.bookmarks.downloadAll}</a>
      </div>
    </div>`;
  }

  function render(ctx) {
    // Wire the delete-button delegate ONCE per screen entry. List mode
    // re-renders into ctx.container on every delete; the container element
    // itself is reused, so binding inside renderList() leaks listeners.
    wireDeleteHandler(ctx);

    var p   = ctx.params || {};
    var bk  = p.book;
    var idx = p.idx;
    if (bk != null && idx != null && bk !== "" && idx !== "") {
      renderDetail(ctx, bk, idx);
    } else {
      renderList(ctx);
    }
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.bookmarks = { render: render };
})();
