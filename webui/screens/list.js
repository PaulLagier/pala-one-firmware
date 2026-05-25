// List screen — fetch /api/list, edit inline, POST the result.
//
// The on-disk list is up to N text items (N = info.max from the API) with
// per-item done flags. We render one row per item: checkbox, text input,
// delete (X) button. "Add item" appends a blank row until we hit max.
//
// "Save" POSTs the whole list back. "Delete checked" filters locally and
// POSTs — same single round-trip either way. Blank items are dropped
// server-side too, so a half-typed row that the user abandons just falls
// off on save.

(function () {
  // -- local mutable model --------------------------------------------------
  // Each entry: { text: string, done: boolean }. Mirrors the wire shape.
  var state = { items: [], max: 16 };
  var refs  = {};
  var t_;

  function setStatus(kind, msg) {
    refs.status.className   = "status " + kind;
    refs.status.textContent = msg;
  }
  function clearStatus() {
    refs.status.className   = "";
    refs.status.textContent = "";
  }

  function renderRows() {
    if (state.items.length === 0) {
      refs.rows.innerHTML = '<div class="empty-hint">' + t_.list.empty + '</div>';
    } else {
      var html = "";
      for (var i = 0; i < state.items.length; i++) {
        var it = state.items[i];
        html +=
          '<div class="row list-row" data-idx="' + i + '">' +
            '<div><input type="checkbox" data-role="done"' + (it.done ? " checked" : "") + '></div>' +
            '<div class="grow"><input type="text" data-role="text" maxlength="64" ' +
              'placeholder="' + t_.list.placeholder + '" value="' + escapeAttr(it.text) + '"></div>' +
            '<div><button type="button" class="btn secondary small" data-role="delete" ' +
              'aria-label="' + t_.list.deleteOne + '">×</button></div>' +
          '</div>';
      }
      refs.rows.innerHTML = html;
    }
    refs.add.disabled = state.items.length >= state.max;
    refs.counter.textContent = state.items.length + " / " + state.max;
  }

  function escapeAttr(s) {
    return String(s).replace(/&/g, "&amp;")
                    .replace(/"/g, "&quot;")
                    .replace(/</g, "&lt;")
                    .replace(/>/g, "&gt;");
  }

  // Read DOM back into state. Called before any save / mutation so live
  // edits aren't lost. Whitespace-only rows stay in state (they'll be
  // filtered server-side); we only drop them when explicitly compacting.
  function syncFromDom() {
    var nodes = refs.rows.querySelectorAll(".list-row");
    state.items = Array.prototype.map.call(nodes, function (n) {
      return {
        text: n.querySelector('[data-role="text"]').value,
        done: n.querySelector('[data-role="done"]').checked
      };
    });
  }

  function onRowsClick(ev) {
    var btn = ev.target.closest('[data-role="delete"]');
    if (!btn) return;
    var rowEl = btn.closest(".list-row");
    var idx   = parseInt(rowEl.dataset.idx, 10);
    syncFromDom();
    state.items.splice(idx, 1);
    renderRows();
  }

  function onAdd() {
    syncFromDom();
    if (state.items.length >= state.max) return;
    state.items.push({ text: "", done: false });
    renderRows();
    // Focus the freshly added input so typing flows.
    var last = refs.rows.querySelector(".list-row:last-child input[data-role=text]");
    if (last) last.focus();
  }

  async function save(filterChecked) {
    syncFromDom();
    var toSend = state.items.map(function (it) {
      return { text: (it.text || "").trim(), done: !!it.done };
    });
    if (filterChecked) toSend = toSend.filter(function (it) { return !it.done; });
    toSend = toSend.filter(function (it) { return it.text.length > 0; });

    setStatus("busy", t_.list.saving);
    refs.save.disabled = refs.clear.disabled = refs.add.disabled = true;
    try {
      await window.palaApi.post("/api/list", { items: toSend });
      // Apply the same client-side filtering to the local model so the UI
      // reflects what was persisted.
      state.items = toSend;
      renderRows();
      setStatus("ok", t_.list.saved);
    } catch (e) {
      setStatus("err", (t_.errors.server || "Server error") + ": " + (e.message || e));
    } finally {
      refs.save.disabled = refs.clear.disabled = false;
      refs.add.disabled = state.items.length >= state.max;
    }
  }

  async function load() {
    setStatus("busy", t_.list.loading);
    try {
      var data = await window.palaApi.get("/api/list");
      state.items = (data && data.items) || [];
      state.max   = (data && data.max)   || 16;
      renderRows();
      clearStatus();
    } catch (e) {
      setStatus("err", (t_.errors.server || "Server error") + ": " + (e.message || e));
    }
  }

  function render(ctx) {
    t_ = ctx.t;
    ctx.header({
      title:    t_.list.title,
      subtitle: t_.list.subtitle,
      navKey:   "list"
    });
    ctx.container.innerHTML =
      '<div class="card">' +
        '<h2>' + t_.list.editHeading + '</h2>' +
        '<p class="muted">' + t_.list.editDesc + '</p>' +
        '<div id="list-rows" style="margin-top:10px"></div>' +
        '<div class="actions">' +
          '<button type="button" class="btn secondary" id="list-add">' + t_.list.add + '</button>' +
          '<span class="muted" id="list-counter"></span>' +
        '</div>' +
        '<div class="actions">' +
          '<button type="button" class="btn"           id="list-save">'  + t_.list.save  + '</button>' +
          '<button type="button" class="btn secondary" id="list-clear">' + t_.list.clear + '</button>' +
        '</div>' +
        '<p class="muted" style="margin-top:8px">' + t_.list.hint + '</p>' +
        '<div id="list-status"></div>' +
      '</div>';

    refs = {
      rows:    ctx.container.querySelector("#list-rows"),
      add:     ctx.container.querySelector("#list-add"),
      save:    ctx.container.querySelector("#list-save"),
      clear:   ctx.container.querySelector("#list-clear"),
      counter: ctx.container.querySelector("#list-counter"),
      status:  ctx.container.querySelector("#list-status")
    };

    refs.rows.addEventListener("click", onRowsClick);
    refs.add.addEventListener("click",  onAdd);
    refs.save.addEventListener("click", function () { save(false); });
    refs.clear.addEventListener("click", function () {
      if (!window.confirm(t_.list.confirmClear)) return;
      save(true);
    });

    load();
  }

  window.palaScreens = window.palaScreens || {};
  window.palaScreens.list = { render: render };
})();
