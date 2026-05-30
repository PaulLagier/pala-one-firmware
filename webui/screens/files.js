// Files screen — storage stats, folder management, book library, and apps,
// plus the book + app upload forms.

(function () {
  var html = window.palaHtml.html;

  function humanBytes(n) {
    if (n < 1024) return n + " B";
    if (n < 1024 * 1024) return (n / 1024).toFixed(1) + " KB";
    return (n / 1024 / 1024).toFixed(2) + " MB";
  }

  // Strip the leading "/books/" so the user sees folder paths the same way
  // they enter them. Empty folder = library root.
  function prettyFolder(folder, rootLabel) {
    var f = String(folder || "").replace(/^\/+/, "").replace(/^books\/?/, "");
    return f.length ? f : rootLabel;
  }

  async function load(ctx) {
    var t = ctx.t;
    ctx.header({
      title:    t.files.title,
      subtitle: t.files.subtitle,
      navKey:   "files"
    });
    ctx.container.innerHTML =
      html`<div class="card"><p class="muted">${t.files.loading}</p></div>`;
    try {
      var data = await window.palaApi.get("/api/files");
      renderAll(ctx, data);
    } catch (e) {
      ctx.container.innerHTML =
        html`<div class="banner-warn">${t.errors.server}: ${e.message || e}</div>`;
    }
  }

  function renderAll(ctx, data) {
    var t  = ctx.t;
    var fs = t.files;

    // -- Storage card --------------------------------------------------------
    var s = data.storage || { total: 0, used: 0, free: 0, pct: 0 };
    var booksCount = (data.books || []).length;
    var lowStorage = (s.total === 0 || s.free < 8192);

    var storageCard = html`<div class="card">
      <h2>${fs.storageHeading}</h2>
      <div class="stats">
        <div class="stat"><span class="muted">${fs.storageBooks}</span><b>${booksCount}</b></div>
        <div class="stat"><span class="muted">${fs.storageUsed}</span><b>${humanBytes(s.used)}</b></div>
        <div class="stat"><span class="muted">${fs.storageFree}</span><b>${humanBytes(s.free)}</b></div>
        <div class="stat"><span class="muted">${fs.storageTotal}</span><b>${humanBytes(s.total)}</b></div>
      </div>
      <div class="bar"><span style="width:${s.pct || 0}%"></span></div>
      <div class="muted" style="margin-top:8px">${s.pct || 0}${fs.storagePctSuffix}</div>
    </div>`;

    // -- Folder management ---------------------------------------------------
    var folderCreateCard = html`<div class="card">
      <h2>${fs.createFolderHeading}</h2>
      <form id="folder-create" data-act="folder-create" class="stack" style="margin-top:12px">
        <input type="text" name="folder" placeholder="${fs.createFolderPlaceholder}" maxlength="64">
        <div class="actions">
          <button type="submit">${fs.createFolderButton}</button>
          <span class="muted">${fs.createFolderHint}</span>
        </div>
      </form>
    </div>`;

    var folders = data.folders || [];
    var foldersCard = html`<div class="card">
      <h2>${fs.foldersHeading}</h2>
      ${folders.length === 0
        ? html`<p class="muted">${fs.noFolders}</p>`
        : html`<ul class="list">${folders.map(function (folder) {
            return html`<li><div class="row">
              <div><span class="pill">${prettyFolder(folder, fs.rootLabel)}</span></div>
              <div>
                <button type="button" class="btn secondary small"
                        data-act="folder-delete" data-folder="${folder}">${fs.deleteButton}</button>
              </div>
            </div></li>`;
          })}</ul>`}
    </div>`;

    // -- Library books -------------------------------------------------------
    var books  = data.books  || [];
    var limits = data.limits || { maxBooks: 0, maxFolders: 0 };
    var booksCard = html`<div class="card">
      <h2>${fs.libraryHeading}</h2>
      ${books.length >= limits.maxBooks
        ? html`<p style="color:#b91c1c;font-weight:600">${fs.libraryFullWarn}</p>` : ""}
      ${folders.length >= limits.maxFolders
        ? html`<p style="color:#b91c1c;font-weight:600">${fs.folderLimitWarn}</p>` : ""}
      ${books.length === 0
        ? html`<p class="muted">${fs.noBooks}</p>`
        : html`<ul class="list">${books.map(function (b) {
            var folderLabel = prettyFolder(b.folder, fs.rootLabel);
            return html`<li><div class="row">
              <div style="flex:1">
                <h3>${b.name}</h3>
                <div class="meta">${b.size}${fs.bytesLabel} &middot; ${fs.folderLabel}${folderLabel} &middot; ${fs.currentPage}${b.savedPage}</div>

                <div class="actions" style="margin-top:10px">
                  <a class="btn secondary small" href="#/read?book=${b.id}">${fs.readAndFind}</a>
                </div>

                <form data-act="book-jumppage" data-id="${b.id}" class="stack small" style="margin-top:10px">
                  <div class="row" style="align-items:end;gap:10px">
                    <div style="flex:1">
                      <input type="text" name="page" value="${b.savedPage}" inputmode="numeric" placeholder="${fs.pagePlaceholder}">
                    </div>
                    <div><button type="submit">${fs.jumpButton}</button></div>
                  </div>
                  <div class="muted">${fs.jumpHint}<br><span class="muted">${fs.jumpHint2}</span></div>
                </form>

                <form data-act="book-move" data-id="${b.id}" class="stack small" style="margin-top:10px">
                  <input type="text" name="folder" value="${b.folder}" placeholder="${fs.movePlaceholder}" maxlength="64">
                  <div class="actions">
                    <button type="submit">${fs.moveButton}</button>
                    <span class="muted">${fs.moveHint}</span>
                  </div>
                </form>
              </div>

              <div>
                <button type="button" class="btn secondary small" data-act="book-delete" data-id="${b.id}">${fs.deleteButton}</button>
              </div>
            </div></li>`;
          })}</ul>`}
    </div>`;

    // -- Apps ---------------------------------------------------------------
    var apps = data.apps || [];
    var appsCard = html`<div class="card">
      <h2>${fs.appsHeading}</h2>
      ${apps.length === 0
        ? html`<p class="muted">${fs.noApps}</p>`
        : html`<ul class="list">${apps.map(function (a) {
            return html`<li><div class="row">
              <div>
                <h3>${a.name}</h3>
                <div class="meta">${a.size}${fs.bytesLabel} &middot; ${a.fileName}</div>
              </div>
              <div>
                <button type="button" class="btn secondary small" data-act="app-delete" data-name="${a.fileName}">${fs.deleteButton}</button>
              </div>
            </div></li>`;
          })}</ul>`}
    </div>`;

    // -- Upload cards (book + app) ------------------------------------------
    // Both POST multipart to /upload and /upload-app respectively. Success
    // returns a tiny JSON; errors come back as plain text and we surface
    // them in the status line.
    var warnBanner = lowStorage
      ? html`<div class="banner-warn">${fs.storageWarn}</div>` : "";

    ctx.container.innerHTML = html`${warnBanner}${storageCard}${folderCreateCard}${foldersCard}${booksCard}${appsCard}${uploadCardHtml(t, "book")}${uploadCardHtml(t, "app")}`;
    // Container-level submit/click delegates are wired ONCE in render(),
    // not on each refresh — see wireActionHandlers().
    wireUploads(ctx);
  }

  function uploadCardHtml(t, kind) {
    var fs = t.files;
    var headings = {
      book: { h: fs.uploadBookHeading,   d: fs.uploadBookDesc,   b: fs.uploadBookButton,   accept: ".txt,text/plain", name: "file" },
      app:  { h: fs.uploadAppHeading,    d: fs.uploadAppDesc,    b: fs.uploadAppButton,    accept: ".bin",            name: "file" }
    };
    var c = headings[kind];
    return html`<div class="card" data-upload="${kind}">
      <h2>${c.h}</h2>
      <p class="muted">${c.d}</p>
      <form data-upload-form style="margin-top:14px" enctype="multipart/form-data" accept-charset="UTF-8">
        <input type="file" name="${c.name}" accept="${c.accept}" required>
        <div class="actions">
          <button type="submit">${c.b}</button>
          <span class="muted" data-progress></span>
        </div>
        <div class="bar" data-bar hidden style="margin-top:10px">
          <span style="width:0%"></span>
        </div>
        <div data-status></div>
      </form>
    </div>`;
  }

  // ----------------------------------------------------------------------
  //  Event wiring — container-level delegated handlers. Installed ONCE per
  //  screen entry by render(); the container element outlives every
  //  innerHTML reassignment, so re-wiring on each refresh would stack a
  //  fresh listener every time and end up firing N confirm dialogs after
  //  N–1 deletes.
  // ----------------------------------------------------------------------
  function wireActionHandlers(ctx) {
    var t = ctx.t;
    var fs = t.files;

    async function call(path, body) {
      try {
        await window.palaApi.post(path, body);
        await load(ctx);  // refresh on success
      } catch (e) {
        window.alert((t.errors.server || "Server error") + ": " + (e.message || e));
      }
    }

    ctx.container.addEventListener("submit", function (ev) {
      var f = ev.target.closest("form[data-act]");
      if (!f) return;
      ev.preventDefault();
      var act = f.dataset.act;
      if (act === "folder-create") {
        var folder = f.querySelector('[name=folder]').value.trim();
        if (!folder) return;
        call("/api/folders/create", { folder: folder });
      } else if (act === "book-jumppage") {
        var id   = parseInt(f.dataset.id, 10);
        var page = parseInt(f.querySelector('[name=page]').value, 10);
        if (isNaN(page) || page < 1) page = 1;
        call("/api/books/jumppage", { id: id, page: page });
      } else if (act === "book-move") {
        var id2   = parseInt(f.dataset.id, 10);
        var dest  = f.querySelector('[name=folder]').value;
        call("/api/books/move", { id: id2, folder: dest });
      }
    });

    ctx.container.addEventListener("click", function (ev) {
      var btn = ev.target.closest("button[data-act]");
      if (!btn) return;
      var act = btn.dataset.act;
      if (act === "folder-delete") {
        if (!window.confirm(fs.confirmDeleteFolder)) return;
        call("/api/folders/delete", { folder: btn.dataset.folder });
      } else if (act === "book-delete") {
        if (!window.confirm(fs.confirmDeleteFile)) return;
        call("/api/books/delete", { id: parseInt(btn.dataset.id, 10) });
      } else if (act === "app-delete") {
        if (!window.confirm(fs.confirmDeleteApp)) return;
        call("/api/apps/delete", { name: btn.dataset.name });
      }
    });
  }

  // ----------------------------------------------------------------------
  //  Upload form wiring. Each card has a `<form data-upload-form>` inside a
  //  `<div data-upload="book|app">`. We POST multipart to the matching
  //  endpoint and refresh the screen on success.
  // ----------------------------------------------------------------------
  function wireUploads(ctx) {
    var t  = ctx.t;
    var fs = t.files;
    var ENDPOINTS = { book: "/upload", app: "/upload-app" };

    ctx.container.querySelectorAll('[data-upload]').forEach(function (card) {
      var kind = card.dataset.upload;
      var form = card.querySelector('[data-upload-form]');
      var btn  = card.querySelector('button[type=submit]');
      var prog = card.querySelector('[data-progress]');
      var bar  = card.querySelector('[data-bar]');
      var barFill = bar.querySelector('span');
      var status  = card.querySelector('[data-status]');

      form.addEventListener('submit', async function (ev) {
        ev.preventDefault();
        var file = form.querySelector('input[type=file]').files[0];
        if (!file) return;

        btn.disabled    = true;
        status.className = "";
        status.textContent = "";
        prog.textContent = "";
        bar.hidden = false;
        barFill.style.width = "0%";

        var fd = new FormData(form);
        try {
          await window.palaApi.upload(ENDPOINTS[kind], fd, function (frac, loaded, total) {
            var pct = Math.round(frac * 100);
            barFill.style.width = pct + "%";
            prog.textContent = pct + "%";
          });
          status.className   = "status ok";
          status.textContent = (kind === "book") ? fs.uploadBookOk : fs.uploadAppOk;
          // Reset the input so the user can immediately do another upload.
          form.reset();
          await load(ctx);  // re-fetch storage + library + apps
        } catch (e) {
          status.className   = "status err";
          status.textContent = (t.errors.server || "Server error") + ": " + (e.message || e);
        } finally {
          btn.disabled = false;
          bar.hidden   = true;
          prog.textContent = "";
        }
      });
    });
  }

  function render(ctx) {
    wireActionHandlers(ctx);   // once per screen entry, see comment above
    load(ctx);
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.files = { render: render };
})();
