// Read screen — in-browser book reader with find + jump.
//
// Hash: #/read?book=N
//
// Pulls the book's metadata from /api/files (one fetch, finds by id) and
// streams the raw text from /readbook-text. Search runs client-side over
// the loaded string — case-insensitive substring, every keystroke. Active
// match has its byte offset POSTed to /jumpoffset for the "Jump to here"
// action; jump-by-page goes through /api/books/jumppage like the rest of
// the SPA.

(function () {
  var html = window.palaHtml.html;

  // -- local mutable state (per-screen instance) ----------------------------
  var rawText = "";
  var hits    = [];
  var curHit  = -1;
  var book    = null;   // { id, name, size, savedPage }
  var refs    = {};
  var t_;

  // -- top-level render ------------------------------------------------------
  async function render(ctx) {
    t_ = ctx.t;
    var p = ctx.params || {};
    var bookId = parseInt(p.book, 10);
    if (isNaN(bookId)) {
      ctx.header({ title: t_.read.title, subtitle: "", navKey: "files" });
      ctx.container.innerHTML = html`<div class="card"><p class="muted">${t_.read.errNoBook}</p>
        <div class="actions"><a class="btn secondary" href="#/files">${t_.bookmarks.back}</a></div>
      </div>`;
      return;
    }

    ctx.header({ title: t_.read.title, subtitle: t_.read.subtitle, navKey: "files" });
    ctx.container.innerHTML =
      html`<div class="card"><p class="muted">${t_.read.loading}</p></div>`;

    // Fetch the books list and find ours. Cheaper than a dedicated
    // /api/books/:id endpoint and the SPA tends to have this hot in cache.
    var meta;
    try {
      meta = await window.palaApi.get("/api/files");
    } catch (e) {
      ctx.container.innerHTML =
        html`<div class="banner-warn">${t_.errors.server}: ${e.message || e}</div>`;
      return;
    }
    book = (meta.books || []).filter(function (b) { return b.id === bookId; })[0];
    if (!book) {
      ctx.container.innerHTML = html`<div class="card"><p class="muted">${t_.read.errBookNotFound}</p>
        <div class="actions"><a class="btn secondary" href="#/files">${t_.bookmarks.back}</a></div>
      </div>`;
      return;
    }

    renderShell(ctx);
    loadBookText();
  }

  function renderShell(ctx) {
    var r = t_.read;
    // Find UI, then jump-by-page form (mirrors the files screen's jump form).
    ctx.container.innerHTML = html`<div class="card">
      <h2>${book.name}</h2>
      <div class="meta">${book.size} ${r.bytesLabel} · ${r.currentPageLabel} ${book.savedPage}</div>

      <div class="bv-find">
        <input id="bvFind" type="search" placeholder="${r.findPlaceholder}" autocomplete="off">
        <button type="button" class="btn small"           id="bvFindBtn">${r.findAll}</button>
        <button type="button" class="btn secondary small" id="bvFindPrev">▲ ${r.findPrev}</button>
        <button type="button" class="btn secondary small" id="bvFindNext">▼ ${r.findNext}</button>
        <button type="button" class="btn small"           id="bvJumpBtn">${r.jumpHere}</button>
      </div>
      <div class="bv-status" id="bvFindStat">${r.statLoading}</div>
      <div class="bv-status" id="bvJumpStat"></div>

      <form id="bvJumpPage" class="bv-find" style="margin-top:14px;border-top:1px solid var(--line-soft);padding-top:12px">
        <input type="text" name="page" inputmode="numeric" placeholder="${r.pagePlaceholder}" value="${book.savedPage}" style="max-width:140px">
        <button type="submit" class="btn small">${r.jumpPage}</button>
        <span class="muted small">${r.jumpHint}</span>
      </form>

      <div id="bvText" class="bv-text"></div>
    </div>`;

    refs = {
      text:      ctx.container.querySelector("#bvText"),
      query:     ctx.container.querySelector("#bvFind"),
      btnFind:   ctx.container.querySelector("#bvFindBtn"),
      btnPrev:   ctx.container.querySelector("#bvFindPrev"),
      btnNext:   ctx.container.querySelector("#bvFindNext"),
      btnJump:   ctx.container.querySelector("#bvJumpBtn"),
      stat:      ctx.container.querySelector("#bvFindStat"),
      jumpStat:  ctx.container.querySelector("#bvJumpStat"),
      jumpForm:  ctx.container.querySelector("#bvJumpPage")
    };
    wire(ctx);
  }

  // -- search + highlight ----------------------------------------------------
  function renderText(highlight) {
    if (!highlight) {
      refs.text.textContent = rawText;
      return;
    }
    // Build the highlighted innerHTML by walking hits in order. Plain-string
    // chunks are escaped by the html helper (resolve()), so we never inject
    // markup from book content; the <span> wrappers are the only live HTML.
    var parts = [], last = 0;
    for (var i = 0; i < hits.length; i++) {
      var h = hits[i];
      parts.push(rawText.slice(last, h.start));
      parts.push(html`<span class="${i === curHit ? "find-hit find-cur" : "find-hit"}" data-i="${i}">${rawText.slice(h.start, h.end)}</span>`);
      last = h.end;
    }
    parts.push(rawText.slice(last));
    refs.text.innerHTML = html`${parts}`;
  }

  function gotoHit(i) {
    if (hits.length === 0) {
      refs.stat.textContent = t_.read.statNoMatches;
      return;
    }
    // Cycle with negative-safe modulo so Prev from match 0 wraps to last.
    curHit = ((i % hits.length) + hits.length) % hits.length;
    renderText(true);
    var n = refs.text.querySelector(".find-cur");
    if (n) n.scrollIntoView({ block: "center", behavior: "smooth" });
    refs.stat.textContent =
      t_.read.statMatch
        .replace("{n}",     curHit + 1)
        .replace("{total}", hits.length)
        .replace("{byte}",  hits[curHit].start);
  }

  function search() {
    var q = refs.query.value;
    if (!q) {
      hits = []; curHit = -1;
      renderText(false);
      refs.stat.textContent = t_.read.statEnterPhrase;
      return;
    }
    hits = [];
    var qLow = q.toLowerCase();
    var rLow = rawText.toLowerCase();
    var i = 0;
    while (true) {
      var p = rLow.indexOf(qLow, i);
      if (p < 0) break;
      hits.push({ start: p, end: p + q.length });
      i = p + q.length;
    }
    curHit = hits.length > 0 ? 0 : -1;
    renderText(true);
    if (hits.length === 0) refs.stat.textContent = t_.read.statNoMatches;
    else                   gotoHit(0);
  }

  // -- wiring ---------------------------------------------------------------
  function wire(ctx) {
    refs.btnFind.addEventListener("click", search);
    refs.query.addEventListener("keydown", function (e) {
      if (e.key === "Enter") { e.preventDefault(); search(); }
    });
    refs.btnPrev.addEventListener("click", function () {
      if (hits.length) gotoHit(curHit - 1);
    });
    refs.btnNext.addEventListener("click", function () {
      if (hits.length) gotoHit(curHit + 1);
    });

    // Click directly on a highlighted span to jump to that match.
    refs.text.addEventListener("click", function (e) {
      var tgt = e.target;
      if (tgt && tgt.classList && tgt.classList.contains("find-hit")) {
        var idx = parseInt(tgt.dataset.i, 10);
        if (!isNaN(idx)) gotoHit(idx);
      }
    });

    refs.btnJump.addEventListener("click", function () {
      if (curHit < 0) {
        refs.jumpStat.textContent = t_.read.errFindFirst;
        return;
      }
      var off = hits[curHit].start;
      refs.jumpStat.textContent = t_.read.savingJump;
      refs.btnJump.disabled = true;
      // /jumpoffset accepts urlencoded form bodies.
      var fd = new FormData();
      fd.append("id",     String(book.id));
      fd.append("offset", String(off));
      fetch("/jumpoffset", { method: "POST", body: fd, redirect: "follow" })
        .then(function (r) {
          refs.jumpStat.textContent = r.ok
            ? t_.read.jumpSaved.replace("{byte}", off)
            : (t_.read.errSaveFailed + ": HTTP " + r.status);
        })
        .catch(function (e) {
          refs.jumpStat.textContent =
            t_.read.errSaveFailed + ": " + (e && e.message ? e.message : "error");
        })
        .finally(function () { refs.btnJump.disabled = false; });
    });

    refs.jumpForm.addEventListener("submit", async function (ev) {
      ev.preventDefault();
      var page = parseInt(refs.jumpForm.querySelector('[name=page]').value, 10);
      if (isNaN(page) || page < 1) page = 1;
      refs.jumpStat.textContent = t_.read.savingJump;
      try {
        await window.palaApi.post("/api/books/jumppage", { id: book.id, page: page });
        book.savedPage = page;
        refs.jumpStat.textContent = t_.read.jumpPageSaved.replace("{page}", page);
      } catch (e) {
        refs.jumpStat.textContent =
          t_.read.errSaveFailed + ": " + (e.message || e);
      }
    });
  }

  // -- book load ------------------------------------------------------------
  function loadBookText() {
    fetch("/readbook-text?id=" + encodeURIComponent(book.id))
      .then(function (r) {
        if (!r.ok) throw new Error("HTTP " + r.status);
        return r.text();
      })
      .then(function (text) {
        rawText = text;
        renderText(false);
        refs.stat.textContent =
          t_.read.statLoaded.replace("{bytes}", rawText.length);
      })
      .catch(function (e) {
        refs.stat.textContent =
          t_.read.errLoadFailed + ": " + (e && e.message ? e.message : "error");
      });
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.read = { render: render };
})();
